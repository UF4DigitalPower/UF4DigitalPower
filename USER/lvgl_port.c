#include "lvgl_port.h"

#include "demos/lv_demos.h"
#include "lcd.h"
#include "lvgl.h"
#include "ltdc.h"

#include <stdint.h>
#include <string.h>

#define LV_PORT_HOR_RES        LTDC_WIDTH
#define LV_PORT_VER_RES        LTDC_HEIGHT
#define LV_PORT_BUF_LINES      20U
#define LV_PORT_BUFFER_PIXELS  (LV_PORT_HOR_RES * LV_PORT_BUF_LINES)
#define LV_PORT_FRAME_PIXELS   (LV_PORT_HOR_RES * LV_PORT_VER_RES)
#define LV_PORT_FB0            ((lv_color_t *)(LCD_FRAME_BUFFER))
#define LV_PORT_FB1            ((lv_color_t *)(LCD_FRAME_BUFFER + (LV_PORT_FRAME_PIXELS * sizeof(lv_color_t))))

/*
 * Current diagnostic mode:
 *   0 = single framebuffer, CPU row-copy directly into the LTDC framebuffer.
 *       This is the most conservative path and avoids the two-framebuffer
 *       alternating/flickering failure seen on the panel.
 *   1 = double framebuffer with VBlank address reload.
 */
#define LV_PORT_USE_DOUBLE_FB  0U

/* Keep DMA2D disabled until the single-framebuffer path is confirmed stable. */
#define LV_PORT_USE_DMA2D      0U

static lv_disp_draw_buf_t s_disp_draw_buf;
static lv_disp_drv_t s_disp_drv;
static uint8_t s_is_initialized = 0U;
static lv_color_t s_draw_buf1[LV_PORT_BUFFER_PIXELS];
static lv_color_t s_draw_buf2[LV_PORT_BUFFER_PIXELS];

#if LV_PORT_USE_DOUBLE_FB
static lv_color_t *s_front_fb = LV_PORT_FB0;
static lv_color_t *s_back_fb = LV_PORT_FB1;
static volatile uint8_t s_ltdc_reload_done = 0U;

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc)
{
    (void)hltdc;
    s_ltdc_reload_done = 1U;
}
#endif

static void lvgl_cpu_copy(const lv_color_t *src,
                          lv_color_t *dst,
                          uint32_t width,
                          uint32_t height,
                          uint32_t src_stride,
                          uint32_t dst_stride)
{
    uint32_t y;

    if (width == 0U || height == 0U) {
        return;
    }

    for (y = 0U; y < height; y++) {
        memcpy(dst, src, width * sizeof(lv_color_t));
        src += src_stride;
        dst += dst_stride;
    }
}

#if LV_PORT_USE_DMA2D
static void lvgl_clean_dcache_by_addr(const void *addr, uint32_t size)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    uintptr_t start_addr;
    uintptr_t end_addr;

    if ((SCB->CCR & SCB_CCR_DC_Msk) == 0U || size == 0U) {
        return;
    }

    start_addr = ((uintptr_t)addr) & ~(uintptr_t)31U;
    end_addr = (((uintptr_t)addr) + size + 31U) & ~(uintptr_t)31U;
    SCB_CleanDCache_by_Addr((uint32_t *)start_addr, (int32_t)(end_addr - start_addr));
#else
    (void)addr;
    (void)size;
#endif
}

static void lvgl_dma2d_copy(const lv_color_t *src,
                            lv_color_t *dst,
                            uint32_t width,
                            uint32_t height,
                            uint32_t src_stride,
                            uint32_t dst_stride)
{
    uint32_t timeout;

    if (width == 0U || height == 0U) {
        return;
    }

    __HAL_RCC_DMA2D_CLK_ENABLE();

    /* LVGL draw buffers are usually in AXI SRAM and may be D-Cacheable on H7. */
    lvgl_clean_dcache_by_addr(src, height * src_stride * sizeof(lv_color_t));

    DMA2D->CR &= ~DMA2D_CR_START;
    DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CCEIF;

    DMA2D->CR = DMA2D_M2M;
    DMA2D->FGPFCCR = DMA2D_INPUT_RGB565;
    DMA2D->OPFCCR = DMA2D_OUTPUT_RGB565;
    DMA2D->FGMAR = (uint32_t)src;
    DMA2D->OMAR = (uint32_t)dst;
    DMA2D->FGOR = src_stride - width;
    DMA2D->OOR = dst_stride - width;
    DMA2D->NLR = (width << 16) | height;
    DMA2D->CR |= DMA2D_CR_START;

    timeout = HAL_GetTick();
    while ((DMA2D->ISR & DMA2D_ISR_TCIF) == 0U) {
        if ((DMA2D->ISR & (DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) != 0U) {
            break;
        }
        if ((HAL_GetTick() - timeout) > 100U) {
            break;
        }
    }

    DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CCEIF;
}
#endif

static void lvgl_fb_copy(const lv_color_t *src,
                         lv_color_t *dst,
                         uint32_t width,
                         uint32_t height,
                         uint32_t src_stride,
                         uint32_t dst_stride)
{
#if LV_PORT_USE_DMA2D
    lvgl_dma2d_copy(src, dst, width, height, src_stride, dst_stride);
#else
    lvgl_cpu_copy(src, dst, width, height, src_stride, dst_stride);
#endif
}

#if LV_PORT_USE_DOUBLE_FB
static void lvgl_swap_buffers_on_vblank(void)
{
    lv_color_t *next_front = s_back_fb;

    if (next_front == s_front_fb) {
        return;
    }

    s_ltdc_reload_done = 0U;
    __HAL_LTDC_CLEAR_FLAG(&hltdc, LTDC_FLAG_RR);

    if (HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t)next_front, 0) != HAL_OK) {
        return;
    }

    if (HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK) {
        return;
    }

    {
        const uint32_t start = HAL_GetTick();
        while (s_ltdc_reload_done == 0U) {
            if ((HAL_GetTick() - start) > 100U) {
                return;
            }
        }
    }

    s_back_fb = s_front_fb;
    s_front_fb = next_front;
}
#endif

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    const uint32_t area_w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t area_h = (uint32_t)(area->y2 - area->y1 + 1);
#if LV_PORT_USE_DOUBLE_FB
    lv_color_t *dst = s_back_fb + ((uint32_t)area->y1 * LV_PORT_HOR_RES) + (uint32_t)area->x1;
#else
    lv_color_t *dst = LV_PORT_FB0 + ((uint32_t)area->y1 * LV_PORT_HOR_RES) + (uint32_t)area->x1;
#endif

    (void)disp_drv;
    lvgl_fb_copy(color_p, dst, area_w, area_h, area_w, LV_PORT_HOR_RES);

#if LV_PORT_USE_DOUBLE_FB
    if (lv_disp_flush_is_last(disp_drv)) {
        lvgl_swap_buffers_on_vblank();
    }
#endif

    lv_disp_flush_ready(disp_drv);
}

void LVGL_Port_Init(void)
{
    if (s_is_initialized != 0U) {
        return;
    }

    lv_init();

#if LV_PORT_USE_DOUBLE_FB
    /* Start from identical framebuffers to avoid a garbage first swap. */
    lvgl_fb_copy(s_front_fb, s_back_fb, LV_PORT_HOR_RES, LV_PORT_VER_RES, LV_PORT_HOR_RES, LV_PORT_HOR_RES);
#endif

    HAL_LTDC_SetAddress(&hltdc, (uint32_t)LV_PORT_FB0, 0);

    lv_disp_draw_buf_init(&s_disp_draw_buf, s_draw_buf1, s_draw_buf2, LV_PORT_BUFFER_PIXELS);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = LV_PORT_HOR_RES;
    s_disp_drv.ver_res = LV_PORT_VER_RES;
    s_disp_drv.draw_buf = &s_disp_draw_buf;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.direct_mode = 0;
    s_disp_drv.full_refresh = 0;

    lv_disp_drv_register(&s_disp_drv);

    s_is_initialized = 1U;
}

void LVGL_Port_RunBenchmark(void)
{
    LVGL_Port_Init();
    lv_demo_benchmark_set_max_speed(true);
    lv_demo_benchmark();
}

uint32_t LVGL_Port_Task(void)
{
    return lv_timer_handler();
}
