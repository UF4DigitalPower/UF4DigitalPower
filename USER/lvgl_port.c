#include "lvgl_port.h"

#include "demos/lv_demos.h"
#include "lcd.h"
#include "lvgl.h"
#include "ltdc.h"

#include <stdint.h>
#include <string.h>

/*
 * Stable baseline:
 *   LTDC physical framebuffer is 480 x 640 RGB565.
 *   LVGL renders directly in the same physical coordinate system.
 *   No software rotation, no DMA2D, no double framebuffer, no LTDC reload.
 *
 * This deliberately avoids the previous rotated per-pixel flush. Rotated writes
 * access SDRAM with a large stride and easily starve LTDC on a 16-bit SDRAM bus.
 */
#define LV_PORT_HOR_RES        LTDC_WIDTH
#define LV_PORT_VER_RES        LTDC_HEIGHT
#define LV_PORT_BUF_LINES      10U
#define LV_PORT_BUFFER_PIXELS  (LV_PORT_HOR_RES * LV_PORT_BUF_LINES)

static lv_disp_draw_buf_t s_disp_draw_buf;
static lv_disp_drv_t s_disp_drv;
static uint8_t s_is_initialized = 0U;
static lv_color_t s_draw_buf1[LV_PORT_BUFFER_PIXELS];
static lv_color_t s_draw_buf2[LV_PORT_BUFFER_PIXELS];

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x1 = area->x1;
    int32_t x2 = area->x2;
    int32_t y1 = area->y1;
    int32_t y2 = area->y2;
    uint32_t src_w;
    uint32_t copy_w;
    uint32_t copy_h;
    uint32_t y;
    lv_color_t *src;
    uint16_t *dst;

    if (x2 < 0 || y2 < 0 || x1 >= (int32_t)LV_PORT_HOR_RES || y1 >= (int32_t)LV_PORT_VER_RES) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= (int32_t)LV_PORT_HOR_RES) x2 = (int32_t)LV_PORT_HOR_RES - 1;
    if (y2 >= (int32_t)LV_PORT_VER_RES) y2 = (int32_t)LV_PORT_VER_RES - 1;

    src_w = (uint32_t)(area->x2 - area->x1 + 1);
    copy_w = (uint32_t)(x2 - x1 + 1);
    copy_h = (uint32_t)(y2 - y1 + 1);

    src = color_p + ((uint32_t)(y1 - area->y1) * src_w) + (uint32_t)(x1 - area->x1);
    dst = ((uint16_t *)LCD_FRAME_BUFFER) + ((uint32_t)y1 * LTDC_WIDTH) + (uint32_t)x1;

    for (y = 0U; y < copy_h; y++) {
        memcpy(dst, src, copy_w * sizeof(lv_color_t));
        src += src_w;
        dst += LTDC_WIDTH;
    }

    lv_disp_flush_ready(disp_drv);
}

void LVGL_Port_Init(void)
{
    if (s_is_initialized != 0U) {
        return;
    }

    lv_init();

    HAL_LTDC_SetAddress(&hltdc, (uint32_t)LCD_FRAME_BUFFER, 0);
    LCD_Clear(BLACK);

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
    lv_demo_benchmark_set_max_speed(false);
    lv_demo_benchmark();
}

uint32_t LVGL_Port_Task(void)
{
    return lv_timer_handler();
}
