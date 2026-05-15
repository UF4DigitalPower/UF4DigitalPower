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
#define LCD_CACHE_LINE    32U
#define LCD_DMA2D_TIMEOUT 0x1FFFFFU

//LCD的画笔颜色和背景色
uint32_t POINT_COLOR = 0xFF000000; //画笔颜色
uint32_t BACK_COLOR = 0xFFFFFFFF;  //背景色

//管理LCD重要参数

//初始化为RGB屏480*640，默认竖屏，每个像素2字节
_lcd_dev lcddev = { .id = 0X7701, .width = LCD_LOGICAL_WIDTH, .height = LCD_LOGICAL_HEIGHT, .dir = 0, .pixsize = LTDC_PIXSIZE, };

uint16_t *const ltdc_lcd_framebuf = (uint16_t *)LCD_FRAME_BUFFER;

static uint16_t *lcd_front_framebuf = (uint16_t *)SDRAM_LCD_BUF1;
static uint16_t *lcd_back_framebuf  = (uint16_t *)SDRAM_LCD_BUF2;
static uint16_t *lcd_draw_framebuf  = (uint16_t *)SDRAM_LCD_BUF1;
static uint8_t lcd_double_buffer_enabled = 0U;

static inline uint16_t LCD_PhysicalWidth(void)
{
	return LCD_PANEL_WIDTH;
}

static inline uint16_t LCD_PhysicalHeight(void)
{
	return LCD_PANEL_HEIGHT;
}

static inline uint16_t *LCD_DrawBuffer(void)
{
	return lcd_draw_framebuf;
}

static inline uint32_t LCD_RawOffset(uint16_t x, uint16_t y)
{
	return (uint32_t)LCD_PhysicalWidth() * y + x;
}

static void LCD_CleanDCacheByAddr(const void *addr, uint32_t size)
{
	if (addr == NULL || size == 0U) {
		return;
	}

	uint32_t start = (uint32_t)addr & ~(LCD_CACHE_LINE - 1U);
	uint32_t end = ((uint32_t)addr + size + LCD_CACHE_LINE - 1U) & ~(LCD_CACHE_LINE - 1U);
	SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
	__DSB();
	__ISB();
}

static void LCD_InvalidateDCacheByAddr(const void *addr, uint32_t size)
{
	if (addr == NULL || size == 0U) {
		return;
	}

	uint32_t start = (uint32_t)addr & ~(LCD_CACHE_LINE - 1U);
	uint32_t end = ((uint32_t)addr + size + LCD_CACHE_LINE - 1U) & ~(LCD_CACHE_LINE - 1U);
	SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
	__DSB();
	__ISB();
}

static void LCD_CleanFrameBuffer(uint16_t *fb)
{
	LCD_CleanDCacheByAddr(fb, LTDC_FRAME_BYTES);
}

static void LCD_WaitForDma2dIdle(void)
{
	uint32_t timeout = 0U;

	while ((DMA2D->CR & DMA2D_CR_START) != 0U) {
		if (++timeout > LCD_DMA2D_TIMEOUT) {
			break;
		}
	}
}

static void LCD_DMA2D_WaitTransferComplete(void)
{
	uint32_t timeout = 0U;

	while ((DMA2D->ISR & DMA2D_ISR_TCIF) == 0U) {
		if (++timeout > LCD_DMA2D_TIMEOUT) {
			break;
		}
	}
	DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CTWIF | DMA2D_IFCR_CCTCIF | DMA2D_IFCR_CCEIF;
	LCD_WaitForDma2dIdle();
}

static void LCD_ClipRawRect(uint16_t *x, uint16_t *y, uint16_t *width, uint16_t *height)
{
	if (*x >= LCD_PhysicalWidth() || *y >= LCD_PhysicalHeight()) {
		*width = 0U;
		*height = 0U;
		return;
	}

	if ((uint32_t)*x + *width > LCD_PhysicalWidth()) {
		*width = (uint16_t)(LCD_PhysicalWidth() - *x);
	}
	if ((uint32_t)*y + *height > LCD_PhysicalHeight()) {
		*height = (uint16_t)(LCD_PhysicalHeight() - *y);
	}
}

static void LCD_DMA2D_FillRectTo(uint16_t *dst, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
	if (dst == NULL || width == 0U || height == 0U) {
		return;
	}

	LCD_ClipRawRect(&x, &y, &width, &height);
	if (width == 0U || height == 0U) {
		return;
	}

	uint32_t dst_addr = (uint32_t)&dst[LCD_RawOffset(x, y)];
	uint32_t dst_span = ((uint32_t)(height - 1U) * LCD_PhysicalWidth() + width) * LTDC_PIXSIZE;

	LCD_CleanDCacheByAddr((void *)dst_addr, dst_span);

	RCC->AHB1ENR |= 1U << 23;
	LCD_WaitForDma2dIdle();
	DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CTWIF  | DMA2D_IFCR_CCTCIF | DMA2D_IFCR_CCEIF;
	DMA2D->CR = 3U << 16; /* R2M */
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->OOR = LCD_PhysicalWidth() - width;
	DMA2D->OMAR = dst_addr;
	DMA2D->NLR = (uint32_t)height | ((uint32_t)width << 16);
	DMA2D->OCOLR = color;
	DMA2D->CR |= DMA2D_CR_START;
	LCD_DMA2D_WaitTransferComplete();

	LCD_InvalidateDCacheByAddr((void *)dst_addr, dst_span);
}

static void LCD_DMA2D_CopyRectTo(uint16_t *dst, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *color)
{
	if (dst == NULL || width == 0U || height == 0U || color == NULL) {
		return;
	}

	LCD_ClipRawRect(&x, &y, &width, &height);
	if (width == 0U || height == 0U) {
		return;
	}

	uint32_t dst_addr = (uint32_t)&dst[LCD_RawOffset(x, y)];
	uint32_t dst_span = ((uint32_t)(height - 1U) * LCD_PhysicalWidth() + width) * LTDC_PIXSIZE;
	uint32_t src_span = (uint32_t)width * height * LTDC_PIXSIZE;

	LCD_CleanDCacheByAddr(color, src_span);
	LCD_CleanDCacheByAddr((void *)dst_addr, dst_span);

	RCC->AHB1ENR |= 1U << 23;
	LCD_WaitForDma2dIdle();
	DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CTWIF | DMA2D_IFCR_CCTCIF | DMA2D_IFCR_CCEIF;
	DMA2D->CR = DMA2D_M2M;
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->FGOR = 0;
	DMA2D->OOR = LCD_PhysicalWidth() - width;
	DMA2D->FGMAR = (uint32_t)color;
	DMA2D->OMAR = dst_addr;
	DMA2D->NLR = (uint32_t)height | ((uint32_t)width << 16);
	DMA2D->CR |= DMA2D_CR_START;
	LCD_DMA2D_WaitTransferComplete();

	LCD_InvalidateDCacheByAddr((void *)dst_addr, dst_span);
}

static void LCD_DMA2D_CopyFrame(uint16_t *dst, const uint16_t *src)
{
	if (dst == NULL || src == NULL || dst == src) {
		return;
	}

	LCD_CleanDCacheByAddr(src, LTDC_FRAME_BYTES);
	LCD_CleanDCacheByAddr(dst, LTDC_FRAME_BYTES);

	RCC->AHB1ENR |= 1U << 23;
	LCD_WaitForDma2dIdle();
	DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CTWIF |  DMA2D_IFCR_CCTCIF | DMA2D_IFCR_CCEIF;
	DMA2D->CR = DMA2D_M2M;
	DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;
	DMA2D->FGOR = 0;
	DMA2D->OOR = 0;
	DMA2D->FGMAR = (uint32_t)src;
	DMA2D->OMAR = (uint32_t)dst;
	DMA2D->NLR = (uint32_t)LCD_PhysicalHeight() | ((uint32_t)LCD_PhysicalWidth() << 16);
	DMA2D->CR |= DMA2D_CR_START;
	LCD_DMA2D_WaitTransferComplete();

	LCD_InvalidateDCacheByAddr(dst, LTDC_FRAME_BYTES);
}

static void LCD_WaitForVBlank(void)
{
	uint32_t timeout = 0x3FFFFU;
	uint32_t y;

	/* 先等进入一次有效显示区，再等离开有效显示区，避免正好在消隐尾部切地址。 */
	do {
		y = (LTDC->CPSR & LTDC_CPSR_CYPOS_Msk) >> LTDC_CPSR_CYPOS_Pos;
		if (timeout-- == 0U) {
			return;
		}
	} while ((y < hltdc.Init.AccumulatedVBP) || (y >= hltdc.Init.AccumulatedActiveH));

	timeout = 0x3FFFFU;
	do {
		y = (LTDC->CPSR & LTDC_CPSR_CYPOS_Msk) >> LTDC_CPSR_CYPOS_Pos;
		if (timeout-- == 0U) {
			return;
		}
	} while ((y >= hltdc.Init.AccumulatedVBP) && (y < hltdc.Init.AccumulatedActiveH));
}

static void LCD_SwapLayerAddress(uint16_t *framebuf)
{
	if (framebuf == NULL) {
		return;
	}

	LCD_WaitForVBlank();
	LTDC_Layer1->CFBAR = (uint32_t)framebuf;
	LTDC->SRCR = LTDC_SRCR_VBR;
	__DSB();
}

static void LCD_LogicalRectToRaw(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
								 uint16_t *rx, uint16_t *ry, uint16_t *rw, uint16_t *rh)
{
	if (lcddev.dir == 0U) {
		*rx = sx;
		*ry = sy;
		*rw = width;
		*rh = height;
	} else {
		*rx = sy;
		*ry = (uint16_t)(LCD_PhysicalHeight() - sx - width);
		*rw = height;
		*rh = width;
	}
}


void LTDC_Draw_Point(uint16_t x, uint16_t y, uint32_t color) {
	if (x >= lcddev.width || y >= lcddev.height) {
		return;
	}

	if (lcddev.dir == 0U) {
		LCD_DrawBuffer()[(uint32_t)y * LCD_PhysicalWidth() + x] = (uint16_t)color;
	} else {
		const uint16_t fb_x = y;
		const uint16_t fb_y = (uint16_t)(LCD_PhysicalHeight() - 1U - x);
		LCD_DrawBuffer()[(uint32_t)fb_y * LCD_PhysicalWidth() + fb_x] = (uint16_t)color;
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
	lcd_front_framebuf = (uint16_t *)SDRAM_LCD_BUF1;
	lcd_back_framebuf = (uint16_t *)SDRAM_LCD_BUF2;
	lcd_draw_framebuf = lcd_front_framebuf;
	lcd_double_buffer_enabled = 0U;
//			LTDC_PanelID_Read();
	printf("LCD ID:%#x\r\n", lcddev.id);
}

void LCD_EnableDoubleBuffer(uint8_t enable)
{
	if (enable != 0U) {
		if (lcd_double_buffer_enabled == 0U) {
			lcd_front_framebuf = (uint16_t *)SDRAM_LCD_BUF1;
			lcd_back_framebuf = (uint16_t *)SDRAM_LCD_BUF2;
			lcd_draw_framebuf = lcd_back_framebuf;
			LCD_DMA2D_CopyFrame(lcd_draw_framebuf, lcd_front_framebuf);
			lcd_double_buffer_enabled = 1U;
		}
		return;
	}

	if (lcd_double_buffer_enabled != 0U) {
		LCD_PresentFrame();
		lcd_draw_framebuf = lcd_front_framebuf;
		lcd_double_buffer_enabled = 0U;
	}
}

void LCD_PresentFrame(void)
{
	if (lcd_double_buffer_enabled == 0U) {
		LCD_CleanFrameBuffer(lcd_draw_framebuf);
		return;
	}

	LCD_CleanFrameBuffer(lcd_draw_framebuf);
	LCD_SwapLayerAddress(lcd_draw_framebuf);

	lcd_front_framebuf = lcd_draw_framebuf;
	lcd_draw_framebuf = (lcd_front_framebuf == (uint16_t *)SDRAM_LCD_BUF1) ?
					  (uint16_t *)SDRAM_LCD_BUF2 : (uint16_t *)SDRAM_LCD_BUF1;

	/* 让下一帧从刚显示的完整画面开始，适合裸屏局部擦除/局部重绘。 */
	LCD_DMA2D_CopyFrame(lcd_draw_framebuf, lcd_front_framebuf);
}

void LCD_ScanoutFrame(uint16_t *framebuf)
{
	if (framebuf == NULL) {
		return;
	}

	/* Zero-copy present path for LVGL full-frame buffers. */
	LCD_CleanFrameBuffer(framebuf);
	LCD_SwapLayerAddress(framebuf);
}

void LCD_Clear(uint32_t color) {
	LCD_DMA2D_FillRectTo(LCD_DrawBuffer(), 0, 0, LCD_PhysicalWidth(), LCD_PhysicalHeight(), color);
}

void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
	if (sx > ex || sy > ey || sx >= lcddev.width || sy >= lcddev.height) {
		return;
	}

	if (ex >= lcddev.width) {
		ex = (uint16_t)(lcddev.width - 1U);
	}
	if (ey >= lcddev.height) {
		ey = (uint16_t)(lcddev.height - 1U);
	}

	uint16_t rx, ry, rw, rh;
	LCD_LogicalRectToRaw(sx, sy, (uint16_t)(ex - sx + 1U), (uint16_t)(ey - sy + 1U), &rx, &ry, &rw, &rh);
	LCD_DMA2D_FillRectTo(LCD_DrawBuffer(), rx, ry, rw, rh, color);
}

void LCD_Rect_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color) {
	if (sx >= lcddev.width || sy >= lcddev.height || ex == 0U || ey == 0U) {
		return;
	}

	if ((uint32_t)sx + ex > lcddev.width) {
		ex = (uint16_t)(lcddev.width - sx);
	}
	if ((uint32_t)sy + ey > lcddev.height) {
		ey = (uint16_t)(lcddev.height - sy);
	}

	uint16_t rx, ry, rw, rh;
	LCD_LogicalRectToRaw(sx, sy, ex, ey, &rx, &ry, &rw, &rh);
	LCD_DMA2D_FillRectTo(LCD_DrawBuffer(), rx, ry, rw, rh, color);
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
	LCD_DMA2D_CopyRectTo(LCD_DrawBuffer(), (uint16_t)psx, (uint16_t)psy, (uint16_t)(pex - psx + 1U), (uint16_t)(pey - psy + 1U), color);
}

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	if (x1 == x2) {
		if (y1 > y2) {
			uint16_t t = y1;
			y1 = y2;
			y2 = t;
		}
		LCD_Fill(x1, y1, x2, y2, POINT_COLOR);
		return;
	}

	if (y1 == y2) {
		if (x1 > x2) {
			uint16_t t = x1;
			x1 = x2;
		x2 = t;
		}
		LCD_Fill(x1, y1, x2, y2, POINT_COLOR);
		return;
	}

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
	LCD_EnableDoubleBuffer(1U);
	LCD_Clear(bg);
	POINT_COLOR = WHITE;
	LCD_DrawRectangle(0, 0, lcddev.width - 1U, lcddev.height - 1U);
	LCD_PresentFrame();

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

		LCD_PresentFrame();
		frame++;
	}
}
