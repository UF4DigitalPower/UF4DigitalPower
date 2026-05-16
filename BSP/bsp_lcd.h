/**
 * @file    : bsp_lcd.h
 * @brief   : TODO: 请填写模块功能简介
 * @author  : UF4
 * @date    : 2026/5/16 11:20
 * @version : CLion
 * @project : STM32H743
 * @details : 
 * TODO: 请填写详细说明
 */

#ifndef STM32H743_BSP_LCD_H
#define STM32H743_BSP_LCD_H

#include "main.h"
#include "stdlib.h"

#define LTDC_WIDTH 480
#define LTDC_HEIGHT 640

#define LCD_LOGICAL_PORTRAIT_WIDTH   LTDC_WIDTH
#define LCD_LOGICAL_PORTRAIT_HEIGHT  LTDC_HEIGHT
#define LCD_LOGICAL_LANDSCAPE_WIDTH  LTDC_HEIGHT
#define LCD_LOGICAL_LANDSCAPE_HEIGHT LTDC_WIDTH

#define LCD_FRAMEBUFFER_ADDR      0xC0000000UL
#define LCD_FRAMEBUFFER_BACK_ADDR (LCD_FRAMEBUFFER_ADDR + (2UL * 1024UL * 1024UL))
#define LCD_FRAMEBUFFER_PIXELS    ((uint32_t)LTDC_WIDTH * (uint32_t)LTDC_HEIGHT)
#define LCD_FRAMEBUFFER_BYTES     (LCD_FRAMEBUFFER_PIXELS * 2UL)

extern LTDC_HandleTypeDef hltdc;
extern DMA2D_HandleTypeDef hdma2d;
extern uint16_t ltdc_lcd_framebuf[LTDC_HEIGHT][LTDC_WIDTH];


//LCD MPU保护参数
#define LCD_REGION_NUMBER MPU_REGION_NUMBER0  //LCD使用region0
#define LCD_ADDRESS_START (0xC0000000)		  //LCD区的首地址
#define LCD_REGION_SIZE MPU_REGION_SIZE_32MB //LCD区大小

//画笔颜色
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0XBC40 //棕色
#define BRRED 0XFC07 //棕红色
#define GRAY 0X8430	 //灰色
//GUI颜色

#define DARKBLUE 0X01CF	 //深蓝色
#define LIGHTBLUE 0X7D7C //浅蓝色
#define GRAYBLUE 0X5458	 //灰蓝色
//以上三色为PANEL的颜色

#define LIGHTGREEN 0X841F //浅绿色
//#define LIGHTGRAY        0XEF5B //浅灰色(PANNEL)
#define LGRAY 0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE 0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE 0X2B12	 //浅棕蓝色(选择条目的反色)

#define LCD_PANEL_INVERTED_COLORS 0U

static inline uint16_t LCD_EncodeColor(uint16_t color) {
#if LCD_PANEL_INVERTED_COLORS
	return (uint16_t) (color ^ 0xFFFFU);
#else
	return color;
#endif
}



#ifdef __cplusplus
extern "C" {
#endif

//LCD重要参数集
typedef struct {
    uint16_t width;	 //LCD 宽度
    uint16_t height;	 //LCD 高度
    uint16_t id;		 //LCD ID
    uint8_t dir;		 //横屏还是竖屏控制：0，竖屏；1，横屏。
    uint32_t pixsize;	//每个像素所占字节数
} lcd_dev;

#ifdef __cplusplus
}
#endif

extern lcd_dev LCD_DEV; //管理LCD重要参数

extern uint32_t POINT_COLOR; //默认红色 LCD的画笔颜色和背景色
extern uint32_t BACK_COLOR;	//背景颜色.默认为白色

void LCD_Init(void);													  //初始化
void LCD_Clear(uint32_t color);											  //清屏
void LCD_Rect_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y);										  //画点
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r);								  //画圆
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);						  //画线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);					  //画矩形
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);				  //填充单色
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);		  //填充指定颜色

void LCD_SetDisplayDir(uint8_t dir);
uint32_t LCD_GetDrawBufferAddress(void);
void LCD_CopyRectFromFrontToDraw(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void LCD_Present(void);
#endif /* STM32H743_BSP_LCD_H */
