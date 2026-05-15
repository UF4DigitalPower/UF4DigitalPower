/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include <string.h>

#include "lcd.h"
#include "ltdc.h"
/*********************
 *      DEFINES
 *********************/

#define MY_DISP_HOR_RES LTDC_WIDTH
#define MY_DISP_VER_RES LTDC_HEIGHT
#define MY_DISP_DRAW_BUF_LINES 40
#define DISP_FLUSH_WAIT_TIMEOUT_MS 20U

//#ifndef MY_DISP_HOR_RES
//    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
//    #define MY_DISP_HOR_RES    320
//#endif
//
//#ifndef MY_DISP_VER_RES
//    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
//    #define MY_DISP_VER_RES    240
//#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void disp_wait_for_safe_flush_window(int32_t y2);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{

    disp_init();

    static lv_disp_draw_buf_t draw_buf_dsc;
    static LV_ATTRIBUTE_LARGE_RAM_ARRAY lv_color_t buf_1[MY_DISP_HOR_RES * MY_DISP_DRAW_BUF_LINES];
    static LV_ATTRIBUTE_LARGE_RAM_ARRAY lv_color_t buf_2[MY_DISP_HOR_RES * MY_DISP_DRAW_BUF_LINES];
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, MY_DISP_HOR_RES * MY_DISP_DRAW_BUF_LINES);

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/


    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc;

    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*初始化您的显示器和所需的外围设备.*/
static void disp_init(void)
{
    HAL_LTDC_SetAddress(&hltdc, (uint32_t) ltdc_lcd_framebuf, 0);
    LCD_Clear(BLACK);
}

volatile bool disp_flush_enabled = true;


void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static void disp_wait_for_safe_flush_window(int32_t y2)
{
    uint32_t start_tick = HAL_GetTick();

    while (1) {
        int32_t current_line = LTDC_GetCurrentVisibleLine();

        if (current_line < 0 || current_line > y2) {
            return;
        }

        if ((HAL_GetTick() - start_tick) >= DISP_FLUSH_WAIT_TIMEOUT_MS) {
            return;
        }
    }
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p){
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    if (disp_flush_enabled == false) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (x2 < 0 || y2 < 0 || x1 >= MY_DISP_HOR_RES || y1 >= MY_DISP_VER_RES) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= MY_DISP_HOR_RES) x2 = MY_DISP_HOR_RES - 1;
    if (y2 >= MY_DISP_VER_RES) y2 = MY_DISP_VER_RES - 1;

    disp_wait_for_safe_flush_window(y2);

    // Use DMA2D accelerated function after the LTDC scanner has passed the updated area.
    LCD_Color_Fill((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, (uint16_t*)color_p);

    lv_disp_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

/*这个虚拟的 typedef 纯粹是为了沉默 -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
