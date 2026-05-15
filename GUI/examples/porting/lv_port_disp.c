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
#include "st7701.h"
/*********************
 *      DEFINES
 *********************/

#define MY_DISP_HOR_RES LTDC_WIDTH
#define MY_DISP_VER_RES LTDC_HEIGHT

#define MY_DISP_FRAME_PIXELS ((uint32_t)MY_DISP_HOR_RES * (uint32_t)MY_DISP_VER_RES)

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
    static LV_ATTRIBUTE_LARGE_RAM_ARRAY lv_color_t *buf_1 = (lv_color_t *)SDRAM_LVGL_DRAW_BUF1;
    static LV_ATTRIBUTE_LARGE_RAM_ARRAY lv_color_t *buf_2 = (lv_color_t *)SDRAM_LVGL_DRAW_BUF2;
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, MY_DISP_FRAME_PIXELS);

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/


    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Full-frame rendering in SDRAM buffers, then scan out directly. */
    disp_drv.full_refresh = 1;

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
    LCD_Init();
    st7701Init();
    HAL_LTDC_SetAddress(&hltdc, (uint32_t) ltdc_lcd_framebuf, 0);
    /* LVGL uses panel-native portrait coordinates; disable extra rotate in LCD_Color_Fill. */
    LCD_Display_Dir(1U);
    LCD_EnableDoubleBuffer(0U);
    LCD_Clear(RED);
    LCD_ScanoutFrame((uint16_t *)ltdc_lcd_framebuf);
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

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p){
    (void)area;
    (void)color_p;

    if (disp_flush_enabled == false) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        LCD_ScanoutFrame((uint16_t *)disp_drv->draw_buf->buf_act);
    }

    lv_disp_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

typedef int keep_pedantic_happy;
#endif
