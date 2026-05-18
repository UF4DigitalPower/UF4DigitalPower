#include "gui_lvgl_port.h"

#include "bsp_lcd.h"
#include "lvgl.h"

static lv_disp_draw_buf_t g_lvgl_draw_buf;
static lv_disp_drv_t g_lvgl_disp_drv;

static void gui_lvgl_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
	(void) area;

	if (lv_disp_flush_is_last(disp_drv)) {
		LCD_PresentBuffer((uint32_t) color_p);
	}

	lv_disp_flush_ready(disp_drv);
}

void GUI_LVGL_PortInit(void)
{
	lv_init();

	LCD_SetDisplayDir(0);
	LCD_PresentBuffer(LCD_FRAMEBUFFER_BACK_ADDR);
	lv_disp_draw_buf_init(&g_lvgl_draw_buf,
						  (void *) LCD_FRAMEBUFFER_ADDR,
						  (void *) LCD_FRAMEBUFFER_BACK_ADDR,
						  LCD_FRAMEBUFFER_PIXELS);

	lv_disp_drv_init(&g_lvgl_disp_drv);
	g_lvgl_disp_drv.hor_res = LTDC_WIDTH;
	g_lvgl_disp_drv.ver_res = LTDC_HEIGHT;
	g_lvgl_disp_drv.flush_cb = gui_lvgl_flush;
	g_lvgl_disp_drv.draw_buf = &g_lvgl_draw_buf;
	g_lvgl_disp_drv.full_refresh = 1;
	lv_disp_drv_register(&g_lvgl_disp_drv);
}
