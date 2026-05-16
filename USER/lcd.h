//
// Created by UF4 on 2026/4/12.
//

#ifndef STM32H750_COM_LCD_H
#define STM32H750_COM_LCD_H

#include "main.h"
#include "stdlib.h"

extern LTDC_HandleTypeDef hltdc;
extern DMA2D_HandleTypeDef hdma2d;

//LCD重要参数集
typedef struct {
	uint16_t width;	 //LCD 宽度
	uint16_t height;	 //LCD 高度
	uint16_t id;		 //LCD ID
	uint8_t dir;		 //显示方向：0，物理竖屏；1，逻辑横屏。
	uint32_t pixsize;	//每个像素所占字节数
} _lcd_dev;

typedef struct { // Data stored PER GLYPH
	uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
	uint8_t  width, height;    // Bitmap dimensions in pixels
	uint8_t  xAdvance;         // Distance to advance cursor (x axis)
	int8_t   xOffset, yOffset; // Dist from cursor position to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
	uint8_t  *bitmap;      // Glyph bitmaps, concatenated
	GFXglyph *glyph;       // Glyph array
	uint8_t   first, last; // ASCII extents
	uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;

//LCD参数
extern _lcd_dev lcddev; //管理LCD重要参数
//LCD的画笔颜色和背景色
extern uint32_t POINT_COLOR; //默认红色
extern uint32_t BACK_COLOR;	//背景颜色.默认为白色

#define LTDC_WIDTH 640U
#define LTDC_HEIGHT 480U
#define LTDC_PIXSIZE 2U
#define LTDC_FRAME_BYTES (LTDC_WIDTH * LTDC_HEIGHT * LTDC_PIXSIZE)

#define LCD_LOGICAL_WIDTH 640U
#define LCD_LOGICAL_HEIGHT 480U

// SDRAM / LTDC / LVGL 内存规划
#define EXT_SDRAM_ADDR        ((uint32_t)0xC0000000)
#define EXT_SDRAM_SIZE        (32U * 1024U * 1024U)
#define SDRAM_LCD_SIZE        (2U * 1024U * 1024U)
#define SDRAM_LCD_LAYER       2U

#define SDRAM_LCD_BUF1        EXT_SDRAM_ADDR
#define SDRAM_LCD_BUF2        (EXT_SDRAM_ADDR + SDRAM_LCD_SIZE)
#define SDRAM_APP_BUF         (EXT_SDRAM_ADDR + SDRAM_LCD_SIZE * SDRAM_LCD_LAYER)
#define SDRAM_APP_SIZE        (EXT_SDRAM_SIZE - SDRAM_LCD_SIZE * SDRAM_LCD_LAYER)

/* LVGL 双全屏 draw buffer，放在 SDRAM 应用区起始位置 */
#define SDRAM_LVGL_DRAW_BUF1  SDRAM_APP_BUF
#define SDRAM_LVGL_DRAW_BUF2  (SDRAM_LVGL_DRAW_BUF1 + LTDC_FRAME_BYTES)
#define SDRAM_LVGL_HEAP_ADDR  (SDRAM_APP_BUF + 2U * LTDC_FRAME_BYTES)
#define SDRAM_LVGL_HEAP_SIZE  (EXT_SDRAM_ADDR + EXT_SDRAM_SIZE - SDRAM_LVGL_HEAP_ADDR)

//LCD MPU保护参数
#define LCD_REGION_NUMBER MPU_REGION_NUMBER0  //LCD使用region0
#define LCD_ADDRESS_START EXT_SDRAM_ADDR		  //LCD区的首地址
#define LCD_REGION_SIZE MPU_REGION_SIZE_32MB  //LCD区大小

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

#define LCD_FRAME_BUFFER SDRAM_LCD_BUF1
extern uint16_t *const ltdc_lcd_framebuf;


void LCD_Init(void);                                                    //初始化
void LCD_Clear(uint32_t color);                                         //清屏
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color); //按坐标范围填充单色
void LCD_Rect_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color); //按宽高填充
void LCD_Font(uint16_t x, uint16_t y, const char *text, const GFXfont *p_font, uint8_t size, uint32_t color24);
void LCD_Display_Dir(uint8_t dir);                                      //设置显示方向
void LCD_EnableDoubleBuffer(uint8_t enable);                             //启用/关闭裸屏双缓冲
void LCD_PresentFrame(void);                                             //提交后台帧到LTDC
void LCD_ScanoutFrame(uint16_t *framebuf);                              //直接切换LTDC扫描地址到指定帧缓冲
void LCD_BlitLandscapeArea(const uint16_t *src, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_TestLoop(void);                                                //裸屏循环测试
void LCD_DrawPoint(uint16_t x, uint16_t y);                              //画点
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r);               //画圆
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);   //画线
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); //画矩形
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color); //填充指定颜色
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t *p); //显示一个字符串,12/16字体


#endif //STM32H750_COM_LCD_H

