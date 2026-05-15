#include "lvgl_port.h"

#include "demos/lv_demos.h"
#include "lcd.h"
#include "lvgl.h"
#include "ltdc.h"
#include "lv_demo_benchmark.h"
#include <stdint.h>

/*
 * The LTDC layer is the physical panel buffer: 480 x 640, RGB565.
 * The module is mounted/used as a 640 x 480 landscape display, and lcd.c maps
 * logical coordinates as:
 *   logical(x, y) -> physical(fb_x = y, fb_y = LTDC_HEIGHT - 1 - x)
 *
 * Therefore LVGL must render in the same logical coordinate system. Do not let
 * LVGL write a linear 480 x 640 framebuffer directly, or the output becomes
 * rotated/striped because the display orientation does not match the LTDC memory
 * layout.
 */
#define LV_PORT_HOR_RES        LCD_LOGICAL_WIDTH
#define LV_PORT_VER_RES        LCD_LOGICAL_HEIGHT
#define LV_PORT_BUF_LINES      20U
#define LV_PORT_BUFFER_PIXELS  (LV_PORT_HOR_RES * LV_PORT_BUF_LINES)

static lv_disp_draw_buf_t s_disp_draw_buf;
static lv_disp_drv_t s_disp_drv;
static uint8_t s_is_initialized = 0U;
static lv_color_t s_draw_buf1[LV_PORT_BUFFER_PIXELS];
static lv_color_t s_draw_buf2[LV_PORT_BUFFER_PIXELS];

static inline void lvgl_put_pixel_rotated(uint32_t x, uint32_t y, lv_color_t color)
{
    uint32_t fb_x;
    uint32_t fb_y;
    uint16_t *fb;

    if (x >= LCD_LOGICAL_WIDTH || y >= LCD_LOGICAL_HEIGHT) {
        return;
    }

    fb_x = y;
    fb_y = (uint32_t)LTDC_HEIGHT - 1U - x;
    fb = (uint16_t *)LCD_FRAME_BUFFER;
    fb[(fb_y * LTDC_WIDTH) + fb_x] = color.full;
}

static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x;
    int32_t y;
    int32_t x1;
    int32_t x2;
    int32_t y1;
    int32_t y2;
    uint32_t src_w;
    lv_color_t *src_row;

    x1 = area->x1;
    x2 = area->x2;
    y1 = area->y1;
    y2 = area->y2;

    if (x2 < 0 || y2 < 0 || x1 >= (int32_t)LV_PORT_HOR_RES || y1 >= (int32_t)LV_PORT_VER_RES) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= (int32_t)LV_PORT_HOR_RES) {
        x2 = (int32_t)LV_PORT_HOR_RES - 1;
    }
    if (y2 >= (int32_t)LV_PORT_VER_RES) {
        y2 = (int32_t)LV_PORT_VER_RES - 1;
    }

    src_w = (uint32_t)(area->x2 - area->x1 + 1);

    for (y = y1; y <= y2; y++) {
        src_row = color_p + ((uint32_t)(y - area->y1) * src_w) + (uint32_t)(x1 - area->x1);
        for (x = x1; x <= x2; x++) {
            lvgl_put_pixel_rotated((uint32_t)x, (uint32_t)y, *src_row++);
        }
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
    lv_demo_benchmark_set_max_speed(true);
    lv_demo_benchmark();
}

uint32_t LVGL_Port_Task(void)
{
    return lv_timer_handler();
}
