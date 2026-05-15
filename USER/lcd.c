//
// Created by UF4 on 2026/4/12.
//

#include "lcd.h"
#include <stdint.h>

#include "dma2d.h"
#include "stdio.h"
#include "string.h"

//LTDC中需要实现清屏，画点，区域填充单色，区域填充指定色块函数。

#define LCD_PANEL_WIDTH   480U
#define LCD_PANEL_HEIGHT  640U

//LCD的画笔颜色和背景色
uint32_t POINT_COLOR = 0xFF000000; //画笔颜色
uint32_t BACK_COLOR = 0xFFFFFFFF;  //背景色

//管理LCD重要参数

//初始化为RGB屏480*640，默认竖屏，每个像素2字节
_lcd_dev lcddev = { .id = 0X7701, .width = LCD_LOGICAL_WIDTH, .height = LCD_LOGICAL_HEIGHT, .dir = 0, .pixsize = LTDC_PIXSIZE, };

uint16_t *const ltdc_lcd_framebuf = (uint16_t *)LCD_FRAME_BUFFER;

static inline uint16_t LCD_PhysicalWidth(void)
{
	return LCD_PANEL_WIDTH;
}

static inline uint16_t LCD_PhysicalHeight(void)
{
	return LCD_PANEL_HEIGHT;
}

static void LCD_DMA2D_WaitTransferComplete(void)
{
	uint32_t timeout = 0;

	while ((DMA2D->ISR & DMA2D_ISR_TCIF) == 0U) {
		timeout++;
		if (timeout > 0x1FFFFFU) {
			break;
		}
	}
	DMA2D->IFCR |= DMA2D_IFCR_CTCIF;
}

static void LCD_DMA2D_FillRectRaw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
	if (width == 0U || height == 0U) {
		return;
	}

	if (x >= LCD_PhysicalWidth() || y >= LCD_PhysicalHeight()) {
		return;
	}

	if ((uint32_t)x + width > LCD_PhysicalWidth()) {
		width = (uint16_t)(LCD_PhysicalWidth() - x);
	}
	if ((uint32_t)y + height > LCD_PhysicalHeight()) {
		height = (uint16_t)(LCD_PhysicalHeight() - y);
	}

	RCC->AHB1ENR |= 1 << 23;
	DMA2D->CR &= ~DMA2D_CR_START;
	DMA2D->CR = 3U << 16; /* R2M */
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->OOR = LCD_PhysicalWidth() - width;
	DMA2D->OMAR = (uint32_t)ltdc_lcd_framebuf + (uint32_t)LTDC_PIXSIZE * (LCD_PhysicalWidth() * y + x);
	DMA2D->NLR = (uint32_t)height | ((uint32_t)width << 16);
	DMA2D->OCOLR = color;
	DMA2D->CR |= DMA2D_CR_START;
	LCD_DMA2D_WaitTransferComplete();
}

static void LCD_DMA2D_CopyRectRaw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *color)
{
	if (width == 0U || height == 0U || color == NULL) {
		return;
	}

	if (x >= LCD_PhysicalWidth() || y >= LCD_PhysicalHeight()) {
		return;
	}

	if ((uint32_t)x + width > LCD_PhysicalWidth()) {
		width = (uint16_t)(LCD_PhysicalWidth() - x);
	}
	if ((uint32_t)y + height > LCD_PhysicalHeight()) {
		height = (uint16_t)(LCD_PhysicalHeight() - y);
	}

	RCC->AHB1ENR |= 1 << 23;
	__HAL_DMA2D_CLEAR_FLAG(&hdma2d, DMA2D_FLAG_TC);
	DMA2D->CR &= ~DMA2D_CR_START;
	DMA2D->CR = DMA2D_M2M;
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->FGOR = 0;
	DMA2D->OOR = LCD_PhysicalWidth() - width;
	DMA2D->FGMAR = (uint32_t)color;
	DMA2D->OMAR = (uint32_t)ltdc_lcd_framebuf + (uint32_t)LTDC_PIXSIZE * (LCD_PhysicalWidth() * y + x);
	DMA2D->NLR = (uint32_t)height | ((uint32_t)width << 16);
	DMA2D->CR |= DMA2D_CR_START;
	LCD_DMA2D_WaitTransferComplete();
}


void LTDC_Draw_Point(uint16_t x, uint16_t y, uint32_t color) {
	if (x >= lcddev.width || y >= lcddev.height) {
		return;
	}

	if (lcddev.dir == 0U) {
		ltdc_lcd_framebuf[(uint32_t)y * LCD_PhysicalWidth() + x] = (uint16_t)color;
	} else {
		const uint16_t fb_x = y;
		const uint16_t fb_y = (uint16_t)(LCD_PhysicalHeight() - 1U - x);
		ltdc_lcd_framebuf[(uint32_t)fb_y * LCD_PhysicalWidth() + fb_x] = (uint16_t)color;
	}
}


uint16_t LTDC_PanelID_Read(void) {
return 0;
}

void LCD_Display_Dir(uint8_t dir) {
	lcddev.dir = (dir == 0U) ? 0U : 1U;        //竖屏/横屏
	if (lcddev.dir == 0U) {
		lcddev.width = LCD_PhysicalWidth();
		lcddev.height = LCD_PhysicalHeight();
	} else {
		lcddev.width = LCD_PhysicalHeight();
		lcddev.height = LCD_PhysicalWidth();
	}
}

//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(uint16_t x, uint16_t y) {
	LTDC_Draw_Point(x, y, POINT_COLOR);
}

void LCD_Init(void) {
	lcddev.id = 0X7701;
	lcddev.dir = 0;
	lcddev.width = LCD_PhysicalWidth();
	lcddev.height = LCD_PhysicalHeight();
	lcddev.pixsize = LTDC_PIXSIZE;
//			LTDC_PanelID_Read();
	printf("LCD ID:%#x\r\n", lcddev.id);
}

void LCD_Clear(uint32_t color) {
	LCD_DMA2D_FillRectRaw(0, 0, LCD_PhysicalWidth(), LCD_PhysicalHeight(), color);
}

void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
	if (sx > ex || sy > ey) {
		return;
	}

	if (lcddev.dir == 0U) {
		LCD_DMA2D_FillRectRaw(sx, sy, (uint16_t)(ex - sx + 1U), (uint16_t)(ey - sy + 1U), color);
		return;
	}

	for (uint16_t y = sy; y <= ey; y++) {
		for (uint16_t x = sx; x <= ex; x++) {
			LTDC_Draw_Point(x, y, color);
		}
	}
}

void LCD_Rect_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color) {
	if (sx >= lcddev.width || sy >= lcddev.height || ex == 0 || ey == 0) {
		return;
	}

	if (lcddev.dir == 0U) {
		LCD_DMA2D_FillRectRaw(sx, sy, ex, ey, color);
		return;
	}

	const uint16_t pex = (uint16_t)(sx + ex - 1U);
	const uint16_t pey = (uint16_t)(sy + ey - 1U);
	for (uint16_t y = sy; y <= pey; y++) {
		for (uint16_t x = sx; x <= pex; x++) {
			LTDC_Draw_Point(x, y, color);
		}
	}
}

void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color) {
	uint32_t psx, psy, pex, pey; //以LCD面板为基准的坐标系,不随横竖屏变化而变化
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
	LCD_DMA2D_CopyRectRaw((uint16_t)psx, (uint16_t)psy, (uint16_t)(pex - psx + 1U), (uint16_t)(pey - psy + 1U), color);
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

void LCD_TestLoop(void)
{
	static const uint16_t palette[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE, 0xFD20U};
	const int16_t box_w = 96;
	const int16_t box_h = 72;
	const int16_t ball_r = 26;
	const uint16_t bg = BLACK;
	int16_t box_x = 16;
	int16_t box_y = 16;
	int16_t ball_x = (int16_t)(lcddev.width / 2U);
	int16_t ball_y = (int16_t)(lcddev.height / 2U);
	int16_t box_vx = 5;
	int16_t box_vy = 4;
	int16_t ball_vx = -4;
	int16_t ball_vy = 6;
	uint32_t frame = 0U;

	LCD_Display_Dir(0);
	LCD_Clear(bg);
	POINT_COLOR = WHITE;
	LCD_DrawRectangle(0, 0, lcddev.width - 1U, lcddev.height - 1U);

	while (1) {
		uint16_t box_color = palette[(frame >> 3U) & 0x07U];
		uint16_t ball_color = palette[(frame >> 4U) & 0x07U];
		int16_t prev_box_x = box_x;
		int16_t prev_box_y = box_y;
		int16_t prev_ball_x = ball_x;
		int16_t prev_ball_y = ball_y;

		LCD_Rect_Fill((uint16_t)prev_box_x, (uint16_t)prev_box_y, (uint16_t)box_w, (uint16_t)box_h, bg);
		POINT_COLOR = bg;
		LCD_Draw_Circle((uint16_t)prev_ball_x, (uint16_t)prev_ball_y, (uint8_t)ball_r);

		box_x += box_vx;
		box_y += box_vy;
		if (box_x <= 1 || (box_x + box_w) >= (int16_t)(lcddev.width - 1U)) {
			box_vx = (int16_t)(-box_vx);
			box_x += box_vx;
		}
		if (box_y <= 1 || (box_y + box_h) >= (int16_t)(lcddev.height - 1U)) {
			box_vy = (int16_t)(-box_vy);
			box_y += box_vy;
		}

		ball_x += ball_vx;
		ball_y += ball_vy;
		if ((ball_x - ball_r) <= 1 || (ball_x + ball_r) >= (int16_t)(lcddev.width - 1U)) {
			ball_vx = (int16_t)(-ball_vx);
			ball_x += ball_vx;
		}
		if ((ball_y - ball_r) <= 1 || (ball_y + ball_r) >= (int16_t)(lcddev.height - 1U)) {
			ball_vy = (int16_t)(-ball_vy);
			ball_y += ball_vy;
		}

		LCD_Rect_Fill((uint16_t)box_x, (uint16_t)box_y, (uint16_t)box_w, (uint16_t)box_h, box_color);
		POINT_COLOR = ball_color;
		LCD_Draw_Circle((uint16_t)ball_x, (uint16_t)ball_y, (uint8_t)ball_r);

		/* Sweep a moving horizontal marker to verify per-line update behavior. */
		LCD_Rect_Fill(2, (uint16_t)(2U + ((frame * 3U) % (lcddev.height - 4U))), lcddev.width - 4U, 2, palette[(frame >> 2U) & 0x07U]);

		frame++;
		HAL_Delay(16);
	}
}

