#include "lvgl_port.h"

#include "lcd.h"
#include "lvgl.h"
#include "ltdc.h"

#include <stdint.h>
#include <string.h>

/*
 * Display smoke-test version.
 * Purpose: stop the heavy LVGL benchmark and prove whether tearing is caused
 * by SDRAM/LTDC bus pressure. This path uses LVGL only to draw a very small
 * moving object on the physical 480x640 framebuffer.
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

static lv_obj_t *s_box;
static int16_t s_box_x = 0;
static int8_t s_box_dx = 2;
static uint32_t s_last_anim_tick = 0;

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
    lv_obj_t *label;

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

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "LTDC SDRAM smoke test");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 8, 8);

    s_box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_box, 80, 80);
    lv_obj_set_style_bg_color(s_box, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(s_box, 0, 0);
    lv_obj_set_pos(s_box, 0, (LV_PORT_VER_RES - 80U) / 2U);

    s_is_initialized = 1U;
}

void LVGL_Port_RunBenchmark(void)
{
    LVGL_Port_Init();
}

uint32_t LVGL_Port_Task(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t wait;

    if ((now - s_last_anim_tick) >= 33U) {
        s_last_anim_tick = now;

        s_box_x = (int16_t)(s_box_x + s_box_dx);
        if (s_box_x < 0) {
            s_box_x = 0;
            s_box_dx = 2;
        } else if (s_box_x > (int16_t)(LV_PORT_HOR_RES - 80U)) {
            s_box_x = (int16_t)(LV_PORT_HOR_RES - 80U);
            s_box_dx = -2;
        }

        if (s_box != NULL) {
            lv_obj_set_pos(s_box, s_box_x, (int16_t)((LV_PORT_VER_RES - 80U) / 2U));
        }
    }

    wait = lv_timer_handler();
    if (wait > 10U) {
        wait = 10U;
    }
    return wait;
}
