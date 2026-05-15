//
// Created by UF4 on 2026/4/12.
//

#include "lcd.h"
#include <stdint.h>

#include "stdio.h"
#include "string.h"
//#include "dma2d.h"

//LTDC中需要实现清屏，画点，区域填充单色，区域填充指定色块函数。

//LCD的画笔颜色和背景色
uint32_t POINT_COLOR = 0xFF000000; //画笔颜色
uint32_t BACK_COLOR = 0xFFFFFFFF;  //背景色

//管理LCD重要参数

//初始化为RGB屏480*272，横屏，每个像素2字节
_lcd_dev lcddev = { .id = 0X7701, .width = LCD_LOGICAL_WIDTH, .height = LCD_LOGICAL_HEIGHT, .dir = 1, .pixsize = LTDC_PIXSIZE, };

uint16_t *const ltdc_lcd_framebuf = (uint16_t *)LCD_FRAME_BUFFER;


//画点函数
//x,y:坐标
//color:颜色
void LTDC_Draw_Point(uint16_t x, uint16_t y, uint32_t color) {
	uint16_t fb_x;
	uint16_t fb_y;

	if (x >= lcddev.width || y >= lcddev.height) {
		return;
	}

	fb_x = y;
	fb_y = (uint16_t)(LTDC_HEIGHT - 1U - x);
	*(uint16_t*) ((uint32_t) ltdc_lcd_framebuf + LTDC_PIXSIZE * (LTDC_WIDTH * fb_y + fb_x)) = color;
}


uint16_t LTDC_PanelID_Read(void) {
return 0;
}

//设置LCD显示方向
//dir:0,竖屏；1,横屏
void LCD_Display_Dir(uint8_t dir) {
	lcddev.dir = dir;        //横屏/竖屏
	lcddev.width = LCD_LOGICAL_WIDTH;
	lcddev.height = LCD_LOGICAL_HEIGHT;
}

//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(uint16_t x, uint16_t y) {
	LTDC_Draw_Point(x, y, POINT_COLOR);
}

//初始化lcd
void LCD_Init(void) {
	lcddev.id = 0X7701;
	lcddev.width = LCD_LOGICAL_WIDTH;
	lcddev.height = LCD_LOGICAL_HEIGHT;
	lcddev.dir = 1;
	lcddev.pixsize = LTDC_PIXSIZE;
//			LTDC_PanelID_Read();
	printf("LCD ID:%#x\r\n", lcddev.id);
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint32_t color) {
	uint32_t timeout = 0;

	RCC->AHB1ENR |= 1 << 23;
	DMA2D->CR = 3 << 16;
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->OOR = 0;
	DMA2D->CR &= ~(1 << 0);
	DMA2D->OMAR = (uint32_t)ltdc_lcd_framebuf;
	DMA2D->NLR = LTDC_HEIGHT | (LTDC_WIDTH << 16);
	DMA2D->OCOLR = color;
	DMA2D->CR |= 1 << 0;
	while ((DMA2D->ISR & (1 << 1)) == 0) {
		timeout++;
		if (timeout > 0X1FFFFF) {
			break;
		}
	}
	DMA2D->IFCR |= 1 << 1;
}

//Fill a specified area with a single color
//(sx,sy),(ex,ey): Coordinates of the diagonal corners of the filled rectangle. The area size is: (ex - sx + 1) * (ey - sy + 1)
//color: The color to be filled
void LCD_Rect_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color) {
	uint16_t x;
	uint16_t y;
	uint16_t pex;
	uint16_t pey;

	if (sx >= lcddev.width || sy >= lcddev.height || ex == 0 || ey == 0) {
		return;
	}

	pex = sx + ex - 1U;
	pey = sy + ey - 1U;
	if (pex >= lcddev.width) {
		pex = lcddev.width - 1U;
	}
	if (pey >= lcddev.height) {
		pey = lcddev.height - 1U;
	}

	for (y = sy; y <= pey; y++) {
		for (x = sx; x <= pex; x++) {
			LTDC_Draw_Point(x, y, color);
		}
	}
}

//在指定区域内填充指定颜色块
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color) {
	uint32_t psx, psy, pex, pey; //以LCD面板为基准的坐标系,不随横竖屏变化而变化
	uint32_t timeout = 0;
	uint16_t offline;
	uint32_t addr;
	//坐标系转换
	if (lcddev.dir) //横屏
	{
		psx = sx;
		psy = sy;
		pex = ex;
		pey = ey;
	} else //竖屏
	{
		psx = sy;
		psy = lcddev.height - ex - 1;
		pex = ey;
		pey = lcddev.height - sx - 1;
	}
	offline = lcddev.width - (pex - psx + 1);
	addr = ((uint32_t) ltdc_lcd_framebuf + lcddev.pixsize * (lcddev.width * psy + psx));
	__HAL_DMA2D_CLEAR_FLAG(&hdma2d, DMA2D_FLAG_TC);			//清除传输完成标志
	RCC->AHB1ENR |= 1 << 23;								//使能DM2D时钟
	DMA2D->CR &= ~(DMA2D_CR_START);							//先停止 DMA2D
	DMA2D->CR = DMA2D_M2M;									//存储器到存储器模式
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;							//设置颜色格式
	DMA2D->FGOR = 0;										//前景层行偏移为0
	DMA2D->OOR = offline;									//设置行偏移
	DMA2D->CR &= ~(1 << 0);									//先停止DMA2D
	DMA2D->FGMAR = (uint32_t) color;								//源地址
	DMA2D->OMAR = addr;										//输出存储器地址
	DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16); //设定行数寄存器
	DMA2D->CR |= 1 << 0;									//启动DMA2D
	while ((DMA2D->ISR & (1 << 1)) == 0)					//等待传输完成
	{
		timeout++;
		if (timeout > 0X1FFFFF)
			break; //超时退出
	}
	DMA2D->IFCR |= 1 << 1; //清除传输完成标志
}

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	uint16_t t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1; //计算坐标增量
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1; //设置单步方向
	else if (delta_x == 0)
		incx = 0; //垂直线
	else {
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; //水平线
	else {
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x; //选取基本增量坐标轴
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++) //画线输出
			{
		LCD_DrawPoint(uRow, uCol); //画点
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) {
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance) {
			yerr -= distance;
			uCol += incy;
		}
	}
}
//画矩形
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}
//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r) {
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1); //判断下个点位置的标志
	while (a <= b) {
		LCD_DrawPoint(x0 + a, y0 - b); //5
		LCD_DrawPoint(x0 + b, y0 - a); //0
		LCD_DrawPoint(x0 + b, y0 + a); //4
		LCD_DrawPoint(x0 + a, y0 + b); //6
		LCD_DrawPoint(x0 - a, y0 + b); //1
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b); //2
		LCD_DrawPoint(x0 - b, y0 - a); //7
		a++;
		//使用Bresenham算法画圆
		if (di < 0)
			di += 4 * a + 6;
		else {
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}

//m^n函数
//返回值:m^n次方.
uint32_t LCD_Pow(uint8_t m, uint8_t n) {
	uint32_t result = 1;
	while (n--)
		result *= m;
	return result;
}


void LCD_Char(int16_t x, int16_t y, const GFXglyph *glyph, const GFXfont *font, uint8_t size, uint32_t color24)
{
	uint8_t  *bitmap = font -> bitmap;
	uint16_t bo = glyph -> bitmapOffset;
	uint8_t bits = 0, bit = 0;
	uint16_t set_pixels = 0;
	uint8_t  cur_x, cur_y;
	for (cur_y = 0; cur_y < glyph -> height; cur_y++)
	{
		for (cur_x = 0; cur_x < glyph -> width; cur_x++)
		{
			if (bit == 0)
			{
				bits = (*(const unsigned char *)(&bitmap[bo++]));
				bit  = 0x80;
			}
			if (bits & bit) set_pixels++;
			else if (set_pixels > 0)
			{
				LCD_Rect_Fill(x + (glyph -> yOffset + cur_x - set_pixels) * size, y + (glyph -> xOffset + cur_y) * size, size * set_pixels, size, color24);
				set_pixels = 0;
			}
			bit >>= 1;
		}
		if (set_pixels > 0)
		{
			LCD_Rect_Fill(x + (glyph -> yOffset + cur_x-set_pixels) * size, y + (glyph -> xOffset + cur_y) * size, size * set_pixels, size, color24);
			set_pixels = 0;
		}
	}
}

void LCD_Font(uint16_t x, uint16_t y, const char *text, const GFXfont *p_font, uint8_t size, uint32_t color24)
{
	int16_t cursor_x = x;
	int16_t cursor_y = y;
	GFXfont font;
	memcpy(&font, p_font, sizeof(GFXfont));
	for (uint16_t text_pos = 0; text_pos < strlen(text); text_pos++)
	{
		char c = text[text_pos];
		if (c == '\n')
		{
			cursor_x = x;
			cursor_y += font.yAdvance * size;
		}
		else if (c >= font.first && c <= font.last && c != '\r')
		{
			GFXglyph glyph;
			memcpy(&glyph, &font.glyph[c - font.first], sizeof(GFXglyph));
			LCD_Char(cursor_x, cursor_y, &glyph, &font, size, color24);
			cursor_x += glyph.xAdvance * size;
		}
	}
}

