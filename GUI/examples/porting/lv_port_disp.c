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
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2, LCD_LOGICAL_WIDTH * LCD_LOGICAL_HEIGHT);

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/


    /*Set the resolution of the display*/
    disp_drv.hor_res = LCD_LOGICAL_WIDTH;
    disp_drv.ver_res = LCD_LOGICAL_HEIGHT;

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
    /* LTDC scans the native 640x480 frame buffer. */
    // LCD_Display_Dir(0U);
    LCD_EnableDoubleBuffer(1U);
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
    if (disp_flush_enabled == false) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    LCD_BlitLandscapeArea((const uint16_t *)color_p,
                          (uint16_t)area->x1,
                          (uint16_t)area->y1,
                          (uint16_t)area->x2,
                          (uint16_t)area->y2);

    lv_disp_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

typedef int keep_pedantic_happy;
#endif
