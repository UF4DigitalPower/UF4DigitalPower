/**
 * @file    : bsp_lcd.c
 * @brief   : TODO: 请填写模块功能简介
 * @author  : UF4
 * @date    : 2026/5/16 11:20
 * @version : CLion
 * @project : STM32H743
 * @details : 
 * TODO: 请填写详细说明
 */

#include "bsp_lcd.h"

#include <stddef.h>
#include <string.h>

// 物理 framebuffer 为 LTDC 的 480x640，软件默认使用 640x480 横屏逻辑坐标
lcd_dev LCD_DEV = {
    .id = 0X7701,
	.width = LCD_LOGICAL_LANDSCAPE_WIDTH,
	.height = LCD_LOGICAL_LANDSCAPE_HEIGHT,
    .dir = 1,
    .pixsize = 2,
};

__attribute__((section(".sdram"), aligned(32))) uint16_t ltdc_lcd_framebuf[LTDC_HEIGHT][LTDC_WIDTH]; // LTDC framebuffer in SDRAM

static uint32_t g_lcd_front_buffer_addr = LCD_FRAMEBUFFER_ADDR;
static uint32_t g_lcd_draw_buffer_addr = LCD_FRAMEBUFFER_BACK_ADDR;

uint32_t POINT_COLOR = WHITE;
uint32_t BACK_COLOR = BLACK;

static uint32_t lcd_get_pixel_address(const uint32_t base_addr, const uint16_t x, const uint16_t y) {
	if (LCD_DEV.dir) {
		return base_addr + LCD_DEV.pixsize * (LTDC_WIDTH * (LTDC_HEIGHT - x - 1U) + y);
	}

	return base_addr + LCD_DEV.pixsize * (LTDC_WIDTH * y + x);
}

static void lcd_map_rect_to_physical(const uint16_t sx, const uint16_t sy, const uint16_t ex, const uint16_t ey,
		uint32_t *psx, uint32_t *psy, uint32_t *pex, uint32_t *pey) {
	if (LCD_DEV.dir) {
		*psx = sy;
		*psy = LTDC_HEIGHT - ex - 1U;
		*pex = ey;
		*pey = LTDC_HEIGHT - sx - 1U;
		return;
	}

	*psx = sx;
	*psy = sy;
	*pex = ex;
	*pey = ey;
}

static void lcd_clean_dcache(const uint32_t addr, const uint32_t size) {
	uintptr_t aligned_addr;
	uint32_t aligned_size;

	aligned_addr = (uintptr_t) addr & ~(uintptr_t) 31U;
	aligned_size = size + (uint32_t) ((uintptr_t) addr - aligned_addr);
	aligned_size = (aligned_size + 31U) & ~31U;

	SCB_CleanDCache_by_Addr((uint32_t *) aligned_addr, (int32_t) aligned_size);
}

uint32_t LCD_GetDrawBufferAddress(void) {
	return g_lcd_draw_buffer_addr;
	}

void LCD_Present(void) {
	uint32_t old_front_addr;
	uint32_t timeout;

	lcd_clean_dcache(g_lcd_draw_buffer_addr, LCD_FRAMEBUFFER_BYTES);

	if (HAL_LTDC_SetAddress_NoReload(&hltdc, g_lcd_draw_buffer_addr, 0) != HAL_OK) {
		Error_Handler();
	}

	HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

	timeout = HAL_GetTick();
	while ((LTDC->SRCR & LTDC_SRCR_VBR) != 0U) {
		if ((HAL_GetTick() - timeout) > 50U) {
			break;
		}
	}

	old_front_addr = g_lcd_front_buffer_addr;
	g_lcd_front_buffer_addr = g_lcd_draw_buffer_addr;
	g_lcd_draw_buffer_addr = old_front_addr;
}

void LCD_Init(void) {
	LCD_DEV.id = 0X7701;
	LCD_DEV.width = LCD_LOGICAL_LANDSCAPE_WIDTH;
	LCD_DEV.height = LCD_LOGICAL_LANDSCAPE_HEIGHT;
	LCD_DEV.dir = 1;
	LCD_DEV.pixsize = 2;
	g_lcd_front_buffer_addr = LCD_FRAMEBUFFER_ADDR;
	g_lcd_draw_buffer_addr = LCD_FRAMEBUFFER_BACK_ADDR;
	POINT_COLOR = WHITE;
	BACK_COLOR = BLACK;
	(void) memset((void *) g_lcd_front_buffer_addr, 0, LCD_FRAMEBUFFER_BYTES);
	(void) memset((void *) g_lcd_draw_buffer_addr, 0, LCD_FRAMEBUFFER_BYTES);
	lcd_clean_dcache(g_lcd_front_buffer_addr, LCD_FRAMEBUFFER_BYTES);
	lcd_clean_dcache(g_lcd_draw_buffer_addr, LCD_FRAMEBUFFER_BYTES);
}

//打开LCD开关
//lcd_switch:1 打开,0，关闭
void LTDC_Switch(uint8_t sw) {
	if (sw == 1)
		__HAL_LTDC_ENABLE(&hltdc);
	else if (sw == 0)
		__HAL_LTDC_DISABLE(&hltdc);
}

//画点函数
//x,y:坐标
//color:颜色
void LTDC_Draw_Point(uint16_t x, uint16_t y, uint32_t color) {
	*(uint16_t*) lcd_get_pixel_address(g_lcd_draw_buffer_addr, x, y) = color;
}

//设置LCD显示方向
//dir:0,竖屏；1,横屏
void LCD_SetDisplayDir(uint8_t dir) {
	LCD_DEV.dir = dir;        //横屏/竖屏
	if (dir == 0){ // 竖屏
		LCD_DEV.width = LCD_LOGICAL_PORTRAIT_WIDTH;
		LCD_DEV.height = LCD_LOGICAL_PORTRAIT_HEIGHT;
	} else if (dir == 1){ //横屏
		LCD_DEV.width = LCD_LOGICAL_LANDSCAPE_WIDTH;
		LCD_DEV.height = LCD_LOGICAL_LANDSCAPE_HEIGHT;
	}
}

//LCD开启显示
void LCD_DisplayOn(void) {
	LTDC_Switch(1); //开启LCD
}

//LCD关闭显示
void LCD_DisplayOff(void) {
	LTDC_Switch(0); //关闭LCD
}
//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(uint16_t x, uint16_t y) {
	LTDC_Draw_Point(x, y, POINT_COLOR);
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint32_t color) {
	LCD_Rect_Fill(0, 0, LCD_DEV.width - 1, LCD_DEV.height - 1, color);
}

//Fill a specified area with a single color
//(sx,sy),(ex,ey): Coordinates of the diagonal corners of the filled rectangle. The area size is: (ex - sx + 1) * (ey - sy + 1)
//color: The color to be filled
void LCD_Rect_Fill(const uint16_t sx, const uint16_t sy, const uint16_t ex, const uint16_t ey, const uint32_t color) {
	uint32_t psx;
	uint32_t psy;
	uint32_t pex;
	uint32_t pey;
	uint32_t timeout = 0;
	uint16_t offline;
	uint32_t addr;

	lcd_map_rect_to_physical(sx, sy, ex, ey, &psx, &psy, &pex, &pey);

	offline = LTDC_WIDTH - (uint16_t) (pex - psx + 1U);
	addr = g_lcd_draw_buffer_addr + LCD_DEV.pixsize * (LTDC_WIDTH * psy + psx);
	RCC->AHB1ENR |= 1 << 23;								//使能DM2D时钟
	DMA2D->CR = 3 << 16;									//寄存器到存储器模式
	DMA2D->OPFCCR = LTDC_PIXEL_FORMAT_RGB565;							//设置颜色格式
	DMA2D->OOR = offline;									//设置行偏移
	DMA2D->CR &= ~(1 << 0);									//先停止DMA2D
	DMA2D->OMAR = addr;										//输出存储器地址
	DMA2D->NLR = (uint32_t) ((pey - psy + 1U) | ((pex - psx + 1U) << 16)); //设定行数寄存器
	DMA2D->OCOLR = color;									//设定输出颜色寄存器
	DMA2D->CR |= 1 << 0;									//启动DMA2D
	while ((DMA2D->ISR & (1 << 1)) == 0)					//等待传输完成
	{
		timeout++;
		if (timeout > 0X1FFFFF)
			break; //超时退出
	}
	DMA2D->IFCR |= 1 << 1; //清除传输完成标志
}

//在指定区域内填充指定颜色块
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色
void LCD_Color_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color) {
	uint32_t psx, psy, pex, pey; //以LCD面板为基准的坐标系,不随横竖屏变化而变化
	uint32_t timeout = 0;
	uint16_t offline;
	uint32_t addr;
	lcd_map_rect_to_physical(sx, sy, ex, ey, &psx, &psy, &pex, &pey);
	offline = LTDC_WIDTH - (uint16_t) (pex - psx + 1U);
	addr = g_lcd_draw_buffer_addr + LCD_DEV.pixsize * (LTDC_WIDTH * psy + psx);
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
	DMA2D->NLR = (uint32_t) ((pey - psy + 1U) | ((pex - psx + 1U) << 16)); //设定行数寄存器
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
