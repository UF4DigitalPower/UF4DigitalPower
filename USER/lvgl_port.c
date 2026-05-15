#include "lvgl_port.h"

#include "demos/lv_demos.h"
#include "lcd.h"
#include "lvgl.h"
#include "ltdc.h"

#define LV_PORT_HOR_RES        LTDC_WIDTH
#define LV_PORT_VER_RES        LTDC_HEIGHT
#define LV_PORT_BUF_LINES      20U
#define LV_PORT_BUFFER_PIXELS  (LV_PORT_HOR_RES * LV_PORT_BUF_LINES)
#define LV_PORT_FRAME_PIXELS   (LV_PORT_HOR_RES * LV_PORT_VER_RES)
#define LV_PORT_FB0            ((lv_color_t *)(LCD_FRAME_BUFFER))
#define LV_PORT_FB1            ((lv_color_t *)(LCD_FRAME_BUFFER + (LV_PORT_FRAME_PIXELS * sizeof(lv_color_t))))

static lv_disp_draw_buf_t s_disp_draw_buf;
static lv_disp_drv_t s_disp_drv;
static uint8_t s_is_initialized = 0U;
static lv_color_t s_draw_buf1[LV_PORT_BUFFER_PIXELS];
static lv_color_t s_draw_buf2[LV_PORT_BUFFER_PIXELS];
static lv_color_t *s_front_fb = LV_PORT_FB0;
static lv_color_t *s_back_fb = LV_PORT_FB1;
static volatile uint8_t s_ltdc_reload_done = 0U;

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc)
{
    (void)hltdc;
    s_ltdc_reload_done = 1U;
}

static void lvgl_dma2d_copy(const lv_color_t *src,
                            lv_color_t *dst,
                            uint32_t width,
                            uint32_t height,
                            uint32_t src_stride,
                            uint32_t dst_stride)
{
    if (width == 0U || height == 0U) {
        return;
    }

    __HAL_RCC_DMA2D_CLK_ENABLE();

    DMA2D->CR = DMA2D_M2M;
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
    DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;
    DMA2D->FGMAR = (uint32_t)src;
    DMA2D->OMAR = (uint32_t)dst;
    DMA2D->FGOR = src_stride - width;
    DMA2D->OOR = dst_stride - width;
    DMA2D->NLR = (width << 16) | height;
    DMA2D->CR |= DMA2D_CR_START;

    while ((DMA2D->ISR & DMA2D_ISR_TCIF) == 0U) {
    }
    DMA2D->IFCR = DMA2D_IFCR_CTCIF;
}

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

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    const uint32_t area_w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t area_h = (uint32_t)(area->y2 - area->y1 + 1);
    lv_color_t *dst = s_back_fb + ((uint32_t)area->y1 * LV_PORT_HOR_RES) + (uint32_t)area->x1;

    lvgl_dma2d_copy(color_p, dst, area_w, area_h, area_w, LV_PORT_HOR_RES);

    if (lv_disp_flush_is_last(disp_drv)) {
        lvgl_swap_buffers_on_vblank();
    }

    lv_disp_flush_ready(disp_drv);
}

void LVGL_Port_Init(void)
{
    if (s_is_initialized != 0U) {
        return;
    }

    lv_init();

    /* Start from identical framebuffers to avoid a garbage first swap. */
    lvgl_dma2d_copy(s_front_fb, s_back_fb, LV_PORT_HOR_RES, LV_PORT_VER_RES, LV_PORT_HOR_RES, LV_PORT_HOR_RES);

    lv_disp_draw_buf_init(&s_disp_draw_buf, s_draw_buf1, s_draw_buf2, LV_PORT_BUFFER_PIXELS);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = LV_PORT_HOR_RES;
    s_disp_drv.ver_res = LV_PORT_VER_RES;
    s_disp_drv.draw_buf = &s_disp_draw_buf;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.direct_mode = 0;
    s_disp_drv.full_refresh = 1;

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

