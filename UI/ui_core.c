/**
 * @file    : ui_core.c
 * @brief   : UI 核心：绘制、输入、页面调度、演示数据
 */

#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "gpio.h"
#include "tim.h"
#include "ui_font.h"

typedef enum {
	UI_EVT_NONE = 0,
	UI_EVT_KEY_UP,
	UI_EVT_KEY_DOWN,
	UI_EVT_KEY_LEFT,
	UI_EVT_KEY_RIGHT,
	UI_EVT_KEY_ENTER,
	UI_EVT_VENC_CW,
	UI_EVT_VENC_CCW,
	UI_EVT_IENC_CW,
	UI_EVT_IENC_CCW,
	UI_EVT_VENC_PUSH,
	UI_EVT_IENC_PUSH,
} ui_event_t;

enum {
	UI_KEY_MASK_UP = (1UL << 0),
	UI_KEY_MASK_DOWN = (1UL << 1),
	UI_KEY_MASK_LEFT = (1UL << 2),
	UI_KEY_MASK_RIGHT = (1UL << 3),
	UI_KEY_MASK_ENTER = (1UL << 4),
	UI_KEY_MASK_VPUSH = (1UL << 5),
	UI_KEY_MASK_IPUSH = (1UL << 6),
};

typedef struct {
	bool last_level;
	uint32_t last_transition_tick;
} ui_push_button_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	bool valid;
} ui_dirty_rect_t;

typedef struct {
	ui_power_snapshot_t snapshot;
	ui_action_callback_t callback;
	void *callback_user_data;
	ui_page_id_t current_page;
	uint8_t home_focus;
	uint8_t settings_focus;
	uint8_t menu_focus;
	bool dirty;
	bool demo_enabled;
	uint32_t last_demo_tick;
	uint16_t demo_phase;
	int32_t enc_v_last;
	int32_t enc_i_last;
	int32_t enc_v_accum;
	int32_t enc_i_accum;
	uint8_t adjust_step_index;
	ui_push_button_t vpush;
	ui_push_button_t ipush;
	ui_dirty_rect_t dirty_rect;
	bool full_redraw;
} ui_state_t;

static ui_state_t g_ui = {
	.dirty = true,
	.full_redraw = true,
};

static volatile uint32_t g_pending_key_mask = 0U;
static uint32_t g_irq_debounce_tick[5] = {0U};

static void ui_render_home_body(void);
static void ui_render_settings_body(void);
static void ui_render_menu_body(void);
static void ui_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
static void ui_process_pending_keys(void);
static void ui_handle_event(ui_event_t event);
static void ui_handle_home_event(ui_event_t event);
static void ui_handle_settings_event(ui_event_t event);
static void ui_handle_menu_event(ui_event_t event);
static void ui_emit_action(ui_action_t action);
static void ui_go_page(ui_page_id_t page);
static void ui_cycle_page(void);
static void ui_toggle_output(void);
static void ui_poll_push_buttons(void);
static void ui_poll_encoders(void);
static void ui_update_demo(void);
static void ui_copy_status_text(const char *text);
static void ui_adjust_voltage(int32_t delta_mv);
static void ui_adjust_current(int32_t delta_ma);
static int32_t ui_clamp_i32(int32_t value, int32_t min_value, int32_t max_value);
static uint16_t ui_get_irq_index(uint16_t gpio_pin);
static bool ui_get_glyph_rows(char ch, uint8_t rows[7]);
static uint16_t ui_to_upper_ascii(uint16_t ch);
static int32_t ui_pow10(uint8_t digits);
static uint16_t ui_get_adjust_multiplier(void);
static int32_t ui_get_voltage_step_mv(void);
static int32_t ui_get_current_step_ma(void);
static void ui_cycle_adjust_step(void);
static int32_t ui_get_input_power_mw(void);
static int32_t ui_get_output_power_mw(void);
static uint16_t ui_get_efficiency_permille(void);
static void ui_draw_metric_card(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
					 const char *title, int32_t raw_value, int32_t divisor,
					 uint8_t decimals, const char *unit, uint16_t accent_color, bool warning);
static void ui_draw_status_chip(uint16_t x, uint16_t y, uint16_t w, const char *text,
					 uint16_t accent_color, bool active);
static void ui_get_page_body_rect(ui_page_id_t page, uint16_t *x, uint16_t *y, uint16_t *w, uint16_t *h);
static void ui_invalidate_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static void ui_request_body_redraw(void);
static void ui_request_full_redraw(void);

static void ui_copy_status_text(const char *text) {
	if (text == NULL) {
		g_ui.snapshot.status_text[0] = '\0';
		return;
	}

	(void) snprintf(g_ui.snapshot.status_text, sizeof(g_ui.snapshot.status_text), "%s", text);
}

static void ui_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
	const uint32_t draw_addr = LCD_GetDrawBufferAddress();

	if (x >= LCD_DEV.width || y >= LCD_DEV.height) {
		return;
	}

	if (LCD_DEV.dir != 0U) {
		*(uint16_t *) (draw_addr + LCD_DEV.pixsize * (LTDC_WIDTH * (LTDC_HEIGHT - x - 1U) + y)) = LCD_EncodeColor(color);
	} else {
		*(uint16_t *) (draw_addr + LCD_DEV.pixsize * (LTDC_WIDTH * y + x)) = LCD_EncodeColor(color);
	}
}

static void ui_emit_action(const ui_action_t action) {
	if (g_ui.callback != NULL) {
		g_ui.callback(action, &g_ui.snapshot, g_ui.callback_user_data);
	}
}

static int32_t ui_clamp_i32(const int32_t value, const int32_t min_value, const int32_t max_value) {
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}


static int32_t ui_pow10(const uint8_t digits) {
	int32_t result = 1;
	uint8_t i;

	for (i = 0U; i < digits; ++i) {
		result *= 10;
	}
	return result;
}

static uint16_t ui_get_adjust_multiplier(void) {
	switch (g_ui.adjust_step_index) {
		case 1U:
			return 10U;
		case 2U:
			return 100U;
		default:
			return 1U;
	}
}

static int32_t ui_get_voltage_step_mv(void) {
	return 100 * (int32_t) ui_get_adjust_multiplier();
}

static int32_t ui_get_current_step_ma(void) {
	return 50 * (int32_t) ui_get_adjust_multiplier();
}

static void ui_cycle_adjust_step(void) {
	char line[20];

	g_ui.adjust_step_index = (uint8_t) ((g_ui.adjust_step_index + 1U) % 3U);
	(void) snprintf(line, sizeof(line), "STEP %uX", (unsigned int) ui_get_adjust_multiplier());
	ui_copy_status_text(line);
	ui_request_body_redraw();
}

static int32_t ui_get_input_power_mw(void) {
	return (int32_t) (((int64_t) g_ui.snapshot.vin_mv * g_ui.snapshot.iin_ma) / 1000LL);
}

static int32_t ui_get_output_power_mw(void) {
	return (int32_t) (((int64_t) g_ui.snapshot.vout_mv * g_ui.snapshot.iout_ma) / 1000LL);
}

static uint16_t ui_get_efficiency_permille(void) {
	const int32_t pin_mw = ui_get_input_power_mw();
	const int32_t pout_mw = ui_get_output_power_mw();
	int32_t value;

	if (pin_mw <= 0 || pout_mw <= 0) {
		return 0U;
	}

	value = (int32_t) (((int64_t) pout_mw * 1000LL) / pin_mw);
	if (value < 0) {
		return 0U;
	}
	if (value > 1000) {
		return 1000U;
	}
	return (uint16_t) value;
}

static void ui_draw_metric_card(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
						 const char *title, const int32_t raw_value, const int32_t divisor,
						 const uint8_t decimals, const char *unit, const uint16_t accent_color,
						 const bool warning) {
	char value_text[24];
	uint16_t value_area_y;
	uint16_t value_area_h;
	uint16_t value_y;
	uint16_t unit_y;

	if (!g_ui.full_redraw) {
		if (unit == NULL) {
			return;
		}

		value_area_y = (uint16_t) (y + ((h >= 100U) ? 42U : 28U));
		value_area_h = (uint16_t) (h - ((h >= 100U) ? 44U : 30U));
		value_y = (uint16_t) (y + ((h >= 100U) ? 52U : 36U));
		unit_y = (uint16_t) (y + h - ((h >= 100U) ? 32U : 24U));

		UI_FillRect((uint16_t) (x + 2U), value_area_y, (uint16_t) (w - 4U), value_area_h, UI_COLOR_CARD);
		UI_FormatScaled(value_text, sizeof(value_text), raw_value, divisor, decimals);
		UI_DrawText((uint16_t) (x + 10U), value_y, value_text, UI_COLOR_TEXT, UI_COLOR_CARD, 3U);
		UI_DrawTextBox((uint16_t) (x + w - 54U), unit_y, 44U, 22U, unit,
					   UI_COLOR_TEXT_DIM, UI_COLOR_CARD, 1U, UI_ALIGN_RIGHT);
		return;
	}

	ui_value_widget_t card = {
		.x = x,
		.y = y,
		.w = w,
		.h = h,
		.title = title,
		.raw_value = raw_value,
		.divisor = divisor,
		.decimals = decimals,
		.unit = unit,
		.accent_color = accent_color,
		.focused = false,
		.warning = warning,
	};

	UI_ValueDraw(&card);
}

static void ui_draw_status_chip(const uint16_t x, const uint16_t y, const uint16_t w, const char *text,
						 const uint16_t accent_color, const bool active) {
	ui_button_t button = {
		.x = x,
		.y = y,
		.w = w,
		.h = 34U,
		.text = text,
		.text_color = UI_COLOR_TEXT,
		.bg_color = UI_COLOR_PANEL,
		.border_color = UI_COLOR_BORDER,
		.accent_color = accent_color,
		.focused = false,
		.active = active,
		.scale = 2U,
	};

	UI_ButtonDraw(&button);
}

static uint16_t ui_get_irq_index(const uint16_t gpio_pin) {
	if (gpio_pin == KEY_UP_Pin) {
		return 0U;
	}
	if (gpio_pin == KEY_DN_Pin) {
		return 1U;
	}
	if (gpio_pin == KEY_L_Pin) {
		return 2U;
	}
	if (gpio_pin == KEY_R_Pin) {
		return 3U;
	}
	return 4U;
}

static void ui_get_page_body_rect(const ui_page_id_t page, uint16_t *x, uint16_t *y, uint16_t *w, uint16_t *h) {
	(void) page;

	if (x != NULL) {
		*x = 16U;
	}
	if (y != NULL) {
		*y = 68U;
	}
	if (w != NULL) {
		*w = 608U;
	}
	if (h != NULL) {
		*h = 382U;
	}
}

static void ui_invalidate_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	uint16_t x2;
	uint16_t y2;
	uint16_t old_x2;
	uint16_t old_y2;

	if (w == 0U || h == 0U) {
		return;
	}

	x2 = (uint16_t) (x + w - 1U);
	y2 = (uint16_t) (y + h - 1U);

	if (!g_ui.dirty_rect.valid) {
		g_ui.dirty_rect.x = x;
		g_ui.dirty_rect.y = y;
		g_ui.dirty_rect.w = w;
		g_ui.dirty_rect.h = h;
		g_ui.dirty_rect.valid = true;
		return;
	}

	old_x2 = (uint16_t) (g_ui.dirty_rect.x + g_ui.dirty_rect.w - 1U);
	old_y2 = (uint16_t) (g_ui.dirty_rect.y + g_ui.dirty_rect.h - 1U);

	if (x < g_ui.dirty_rect.x) {
		g_ui.dirty_rect.x = x;
	}
	if (y < g_ui.dirty_rect.y) {
		g_ui.dirty_rect.y = y;
	}
	if (x2 > old_x2) {
		old_x2 = x2;
	}
	if (y2 > old_y2) {
		old_y2 = y2;
	}

	g_ui.dirty_rect.w = (uint16_t) (old_x2 - g_ui.dirty_rect.x + 1U);
	g_ui.dirty_rect.h = (uint16_t) (old_y2 - g_ui.dirty_rect.y + 1U);
}

static void ui_request_body_redraw(void) {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;

	ui_get_page_body_rect(g_ui.current_page, &x, &y, &w, &h);
	ui_invalidate_rect(x, y, w, h);
	g_ui.dirty = true;
}

static void ui_request_full_redraw(void) {
	g_ui.full_redraw = true;
	g_ui.dirty_rect.valid = false;
	g_ui.dirty = true;
}

static void ui_go_page(const ui_page_id_t page) {
	if (g_ui.current_page != page) {
		g_ui.current_page = page;
		ui_request_full_redraw();
		ui_emit_action(UI_ACTION_PAGE_CHANGED);
	}
}

static void ui_cycle_page(void) {
	ui_go_page((ui_page_id_t) (((uint8_t) g_ui.current_page + 1U) % (uint8_t) UI_PAGE_COUNT));
}

static void ui_toggle_output(void) {
	g_ui.snapshot.output_enabled = !g_ui.snapshot.output_enabled;
	ui_copy_status_text(g_ui.snapshot.output_enabled ? "OUTPUT ON" : "OUTPUT OFF");
	ui_request_body_redraw();
	ui_emit_action(UI_ACTION_OUTPUT_TOGGLED);
}


void UI_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t color) {
	uint16_t end_x;
	uint16_t end_y;

	if (w == 0U || h == 0U) {
		return;
	}
	if (x >= LCD_DEV.width || y >= LCD_DEV.height) {
		return;
	}

	if ((uint32_t) x + w > LCD_DEV.width) {
		w = (uint16_t) (LCD_DEV.width - x);
	}
	if ((uint32_t) y + h > LCD_DEV.height) {
		h = (uint16_t) (LCD_DEV.height - y);
	}

	end_x = (uint16_t) (x + w - 1U);
	end_y = (uint16_t) (y + h - 1U);
	LCD_Rect_Fill(x, y, end_x, end_y, color);
}

void UI_DrawRect(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint16_t color) {
	if (w == 0U || h == 0U) {
		return;
	}

	UI_FillRect(x, y, w, 1U, color);
	UI_FillRect(x, (uint16_t) (y + h - 1U), w, 1U, color);
	UI_FillRect(x, y, 1U, h, color);
	UI_FillRect((uint16_t) (x + w - 1U), y, 1U, h, color);
}

static uint16_t ui_to_upper_ascii(const uint16_t ch) {
	if (ch >= 'a' && ch <= 'z') {
		return (uint16_t) (ch - ('a' - 'A'));
	}
	return ch;
}

static bool ui_get_glyph_rows(char ch, uint8_t rows[7]) {
	ch = (char) ui_to_upper_ascii((uint16_t) ch);
	memset(rows, 0, 7U);

	switch (ch) {
		case ' ': return true;
		case '-': rows[3] = 0x0E; return true;
		case '.': rows[6] = 0x04; return true;
		case ':': rows[2] = 0x04; rows[5] = 0x04; return true;
		case '/': rows[0] = 0x01; rows[1] = 0x02; rows[2] = 0x04; rows[3] = 0x08; rows[4] = 0x10; return true;
		case '%': rows[0] = 0x19; rows[1] = 0x19; rows[2] = 0x02; rows[3] = 0x04; rows[4] = 0x08; rows[5] = 0x13; rows[6] = 0x13; return true;
		case '?': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x01; rows[3] = 0x02; rows[4] = 0x04; rows[6] = 0x04; return true;
		case '0': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x13; rows[3] = 0x15; rows[4] = 0x19; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case '1': rows[0] = 0x04; rows[1] = 0x0C; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x0E; return true;
		case '2': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x01; rows[3] = 0x02; rows[4] = 0x04; rows[5] = 0x08; rows[6] = 0x1F; return true;
		case '3': rows[0] = 0x1E; rows[1] = 0x01; rows[2] = 0x01; rows[3] = 0x06; rows[4] = 0x01; rows[5] = 0x01; rows[6] = 0x1E; return true;
		case '4': rows[0] = 0x02; rows[1] = 0x06; rows[2] = 0x0A; rows[3] = 0x12; rows[4] = 0x1F; rows[5] = 0x02; rows[6] = 0x02; return true;
		case '5': rows[0] = 0x1F; rows[1] = 0x10; rows[2] = 0x1E; rows[3] = 0x01; rows[4] = 0x01; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case '6': rows[0] = 0x06; rows[1] = 0x08; rows[2] = 0x10; rows[3] = 0x1E; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case '7': rows[0] = 0x1F; rows[1] = 0x01; rows[2] = 0x02; rows[3] = 0x04; rows[4] = 0x08; rows[5] = 0x08; rows[6] = 0x08; return true;
		case '8': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x0E; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case '9': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x0F; rows[4] = 0x01; rows[5] = 0x02; rows[6] = 0x0C; return true;
		case 'A': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1F; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11; return true;
		case 'B': rows[0] = 0x1E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1E; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x1E; return true;
		case 'C': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x10; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case 'D': rows[0] = 0x1C; rows[1] = 0x12; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x12; rows[6] = 0x1C; return true;
		case 'E': rows[0] = 0x1F; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x1E; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x1F; return true;
		case 'F': rows[0] = 0x1F; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x1E; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x10; return true;
		case 'G': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x10; rows[3] = 0x17; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case 'H': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1F; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11; return true;
		case 'I': rows[0] = 0x0E; rows[1] = 0x04; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x0E; return true;
		case 'K': rows[0] = 0x11; rows[1] = 0x12; rows[2] = 0x14; rows[3] = 0x18; rows[4] = 0x14; rows[5] = 0x12; rows[6] = 0x11; return true;
		case 'L': rows[0] = 0x10; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x1F; return true;
		case 'M': rows[0] = 0x11; rows[1] = 0x1B; rows[2] = 0x15; rows[3] = 0x15; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11; return true;
		case 'N': rows[0] = 0x11; rows[1] = 0x19; rows[2] = 0x15; rows[3] = 0x13; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x11; return true;
		case 'O': rows[0] = 0x0E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case 'P': rows[0] = 0x1E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1E; rows[4] = 0x10; rows[5] = 0x10; rows[6] = 0x10; return true;
		case 'R': rows[0] = 0x1E; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1E; rows[4] = 0x14; rows[5] = 0x12; rows[6] = 0x11; return true;
		case 'S': rows[0] = 0x0F; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x0E; rows[4] = 0x01; rows[5] = 0x01; rows[6] = 0x1E; return true;
		case 'T': rows[0] = 0x1F; rows[1] = 0x04; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x04; return true;
		case 'U': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x11; rows[6] = 0x0E; return true;
		case 'V': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x0A; rows[6] = 0x04; return true;
		case 'W': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x15; rows[4] = 0x15; rows[5] = 0x15; rows[6] = 0x0A; return true;
		case 'X': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x0A; rows[3] = 0x04; rows[4] = 0x0A; rows[5] = 0x11; rows[6] = 0x11; return true;
		case 'Y': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x0A; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x04; rows[6] = 0x04; return true;
		default:
			return ui_get_glyph_rows('?', rows);
	}
}

void UI_DrawText(const uint16_t x, const uint16_t y, const char *text, const uint16_t color,
				 const uint16_t bg_color, const uint8_t scale) {
	const ui_bitmap_font_t *font;
	const ui_bitmap_glyph_t *glyph;
	uint16_t cursor_x = x;
	uint16_t row;
	uint16_t col;
	uint16_t font_height;
	uint8_t rows[7];
	uint8_t sx;
	uint8_t sy;

	(void) bg_color;

	if (text == NULL || scale == 0U) {
		return;
	}

	font = UI_FontGetForScale(scale);
	font_height = UI_FontGetHeight(font);

	while (*text != '\0') {
		const char draw_ch = (char) ui_to_upper_ascii((uint16_t) *text);

		if (UI_FontGetGlyph(font, draw_ch, &glyph)) {
			if (glyph->width > 0U) {
				for (row = 0U; row < font_height; ++row) {
					for (col = 0U; col < glyph->width; ++col) {
						if (font->bitmap[glyph->bitmap_offset + row * glyph->width + col] != 0U) {
							ui_draw_pixel((uint16_t) (cursor_x + col), (uint16_t) (y + row), color);
						}
					}
				}
			}
			cursor_x = (uint16_t) (cursor_x + glyph->advance);
		} else {
			(void) ui_get_glyph_rows(draw_ch, rows);
			for (row = 0U; row < 7U; ++row) {
				for (col = 0U; col < 5U; ++col) {
					if ((rows[row] & (uint8_t) (1U << (4U - col))) != 0U) {
						for (sy = 0U; sy < scale; ++sy) {
							for (sx = 0U; sx < scale; ++sx) {
								ui_draw_pixel((uint16_t) (cursor_x + col * scale + sx),
										  (uint16_t) (y + row * scale + sy), color);
							}
						}
					}
				}
			}
			cursor_x = (uint16_t) (cursor_x + (6U * scale));
		}
		++text;
	}
}

uint16_t UI_TextWidth(const char *text, const uint8_t scale) {
	const ui_bitmap_font_t *font;
	const ui_bitmap_glyph_t *glyph;
	uint16_t len = 0U;
	uint16_t width = 0U;

	if (text == NULL || scale == 0U) {
		return 0U;
	}

	font = UI_FontGetForScale(scale);

	while (*text != '\0') {
		const char draw_ch = (char) ui_to_upper_ascii((uint16_t) *text);

		if (UI_FontGetGlyph(font, draw_ch, &glyph)) {
			width = (uint16_t) (width + glyph->advance);
		} else {
			++len;
		}
		++text;
	}

	if (width > 0U) {
		return width;
	}

	if (len == 0U) {
		return 0U;
	}
	return (uint16_t) ((len * 6U * scale) - scale);
}

void UI_DrawTextBox(uint16_t x, uint16_t y, const uint16_t w, const uint16_t h, const char *text,
					const uint16_t color, const uint16_t bg_color, const uint8_t scale, const ui_align_t align) {
	uint16_t text_width;
	uint16_t text_x = x;
	uint16_t text_y = y;
	uint16_t text_h;

	if (text == NULL || scale == 0U) {
		return;
	}

	text_width = UI_TextWidth(text, scale);
	text_h = UI_FontGetHeight(UI_FontGetForScale(scale));
	if (text_h == 0U) {
		text_h = (uint16_t) (7U * scale);
	}

	if (w > text_width) {
		if (align == UI_ALIGN_CENTER) {
			text_x = (uint16_t) (x + ((w - text_width) / 2U));
		} else if (align == UI_ALIGN_RIGHT) {
			text_x = (uint16_t) (x + (w - text_width));
		}
	}
	if (h > text_h) {
		text_y = (uint16_t) (y + ((h - text_h) / 2U));
	}

	UI_DrawText(text_x, text_y, text, color, bg_color, scale);
}

void UI_FormatScaled(char *buffer, const size_t size, const int32_t raw_value, const int32_t divisor,
					 const uint8_t decimals) {
	int64_t abs_value;
	int64_t integer_part;
	int64_t fraction_part;
	int32_t scale;
	bool negative;

	if (buffer == NULL || size == 0U || divisor <= 0) {
		return;
	}

	negative = (raw_value < 0);
	abs_value = negative ? -(int64_t) raw_value : (int64_t) raw_value;
	integer_part = abs_value / divisor;

	if (decimals == 0U) {
		(void) snprintf(buffer, size, negative ? "-%ld" : "%ld", (long) integer_part);
		return;
	}

	scale = ui_pow10(decimals);
	fraction_part = ((abs_value % divisor) * scale + (divisor / 2)) / divisor;
	if (fraction_part >= scale) {
		integer_part += 1;
		fraction_part = 0;
	}

	(void) snprintf(buffer, size, negative ? "-%ld.%0*ld" : "%ld.%0*ld",
					(long) integer_part, decimals, (long) fraction_part);
}

static void ui_adjust_voltage(const int32_t delta_mv) {
	g_ui.snapshot.vset_mv = ui_clamp_i32(g_ui.snapshot.vset_mv + delta_mv, 0, 30000);
	ui_copy_status_text("VSET READY");
	ui_request_body_redraw();
}

static void ui_adjust_current(const int32_t delta_ma) {
	g_ui.snapshot.iset_ma = ui_clamp_i32(g_ui.snapshot.iset_ma + delta_ma, 0, 5000);
	ui_copy_status_text("ISET READY");
	ui_request_body_redraw();
}

static void ui_handle_home_event(const ui_event_t event) {
	if (event == UI_EVT_KEY_LEFT) {
		ui_go_page(UI_PAGE_MENU);
	} else if (event == UI_EVT_KEY_RIGHT) {
		ui_go_page(UI_PAGE_SETTINGS);
	}
}

static void ui_handle_settings_event(const ui_event_t event) {
	switch (event) {
		case UI_EVT_KEY_RIGHT:
		case UI_EVT_VENC_CW:
			ui_adjust_voltage(ui_get_voltage_step_mv());
			break;
		case UI_EVT_KEY_LEFT:
		case UI_EVT_VENC_CCW:
			ui_adjust_voltage(-ui_get_voltage_step_mv());
			break;
		case UI_EVT_KEY_UP:
		case UI_EVT_IENC_CW:
			ui_adjust_current(ui_get_current_step_ma());
			break;
		case UI_EVT_KEY_DOWN:
		case UI_EVT_IENC_CCW:
			ui_adjust_current(-ui_get_current_step_ma());
			break;
		default:
			break;
	}
}

static void ui_handle_menu_event(const ui_event_t event) {
	if (event == UI_EVT_KEY_LEFT) {
		ui_go_page(UI_PAGE_HOME);
	} else if (event == UI_EVT_KEY_RIGHT) {
		ui_go_page(UI_PAGE_SETTINGS);
	}
}

static void ui_handle_event(const ui_event_t event) {
	if (event == UI_EVT_KEY_ENTER) {
		ui_cycle_page();
		return;
	}
	if (event == UI_EVT_VENC_PUSH) {
		ui_toggle_output();
		return;
	}
	if (event == UI_EVT_IENC_PUSH) {
		ui_cycle_adjust_step();
		return;
	}

	switch (g_ui.current_page) {
		case UI_PAGE_HOME:
			ui_handle_home_event(event);
			break;
		case UI_PAGE_SETTINGS:
			ui_handle_settings_event(event);
			break;
		case UI_PAGE_MENU:
			ui_handle_menu_event(event);
			break;
		default:
			break;
	}
}

static void ui_process_pending_keys(void) {
	uint32_t mask = g_pending_key_mask;
	g_pending_key_mask = 0U;

	if ((mask & UI_KEY_MASK_UP) != 0U) {
		ui_handle_event(UI_EVT_KEY_UP);
	}
	if ((mask & UI_KEY_MASK_DOWN) != 0U) {
		ui_handle_event(UI_EVT_KEY_DOWN);
	}
	if ((mask & UI_KEY_MASK_LEFT) != 0U) {
		ui_handle_event(UI_EVT_KEY_LEFT);
	}
	if ((mask & UI_KEY_MASK_RIGHT) != 0U) {
		ui_handle_event(UI_EVT_KEY_RIGHT);
	}
	if ((mask & UI_KEY_MASK_ENTER) != 0U) {
		ui_handle_event(UI_EVT_KEY_ENTER);
	}
	if ((mask & UI_KEY_MASK_VPUSH) != 0U) {
		ui_handle_event(UI_EVT_VENC_PUSH);
	}
	if ((mask & UI_KEY_MASK_IPUSH) != 0U) {
		ui_handle_event(UI_EVT_IENC_PUSH);
	}
}

static void ui_poll_push_buttons(void) {
	const uint32_t now = HAL_GetTick();
	const bool v_level = (HAL_GPIO_ReadPin(KEY_V_PUSH_GPIO_Port, KEY_V_PUSH_Pin) == GPIO_PIN_SET);
	const bool i_level = (HAL_GPIO_ReadPin(KEY_I_PUSH_GPIO_Port, KEY_I_PUSH_Pin) == GPIO_PIN_SET);

	if (v_level != g_ui.vpush.last_level && (now - g_ui.vpush.last_transition_tick) > 30U) {
		g_ui.vpush.last_transition_tick = now;
		g_ui.vpush.last_level = v_level;
		if (v_level) {
			g_pending_key_mask |= UI_KEY_MASK_VPUSH;
		}
	}

	if (i_level != g_ui.ipush.last_level && (now - g_ui.ipush.last_transition_tick) > 30U) {
		g_ui.ipush.last_transition_tick = now;
		g_ui.ipush.last_level = i_level;
		if (i_level) {
			g_pending_key_mask |= UI_KEY_MASK_IPUSH;
		}
	}
}

static void ui_poll_encoders(void) {
	int32_t current_v;
	int32_t current_i;
	int32_t diff_v;
	int32_t diff_i;

	current_v = (int32_t) (uint16_t) __HAL_TIM_GET_COUNTER(&htim4);
	current_i = (int32_t) __HAL_TIM_GET_COUNTER(&htim2);

	diff_v = (int32_t) (int16_t) ((uint16_t) current_v - (uint16_t) g_ui.enc_v_last);
	diff_i = current_i - g_ui.enc_i_last;

	g_ui.enc_v_last = current_v;
	g_ui.enc_i_last = current_i;

	g_ui.enc_v_accum += diff_v;
	g_ui.enc_i_accum += diff_i;

	while (g_ui.enc_v_accum >= 2) {
		ui_handle_event(UI_EVT_VENC_CW);
		g_ui.enc_v_accum -= 2;
	}
	while (g_ui.enc_v_accum <= -2) {
		ui_handle_event(UI_EVT_VENC_CCW);
		g_ui.enc_v_accum += 2;
	}
	while (g_ui.enc_i_accum >= 2) {
		ui_handle_event(UI_EVT_IENC_CW);
		g_ui.enc_i_accum -= 2;
	}
	while (g_ui.enc_i_accum <= -2) {
		ui_handle_event(UI_EVT_IENC_CCW);
		g_ui.enc_i_accum += 2;
	}
}

static void ui_update_demo(void) {
	uint32_t now;
	int32_t ramp;

	if (!g_ui.demo_enabled) {
		return;
	}

	now = HAL_GetTick();
	if ((now - g_ui.last_demo_tick) < 120U) {
		return;
	}
	g_ui.last_demo_tick = now;

	g_ui.demo_phase = (uint16_t) ((g_ui.demo_phase + 3U) % 200U);
	ramp = (g_ui.demo_phase <= 100U) ? g_ui.demo_phase : (200 - g_ui.demo_phase);

	g_ui.snapshot.vin_mv = 23500 + ramp * 8;
	g_ui.snapshot.iin_ma = 620 + ramp * 4;
	g_ui.snapshot.fan_permille = (uint16_t) ui_clamp_i32(180 + ramp * 6, 0, 1000);

	if (g_ui.snapshot.output_enabled) {
		g_ui.snapshot.vout_mv = g_ui.snapshot.vset_mv - 80 + ramp * 2;
		g_ui.snapshot.iout_ma = ui_clamp_i32((g_ui.snapshot.iset_ma * (40 + ramp / 2)) / 100, 0, g_ui.snapshot.iset_ma);
		g_ui.snapshot.cc_mode = (bool) (g_ui.snapshot.iout_ma >= (g_ui.snapshot.iset_ma - 80));
		g_ui.snapshot.temp_dC = 315 + ramp / 2;
		g_ui.snapshot.fan_permille = (uint16_t) ui_clamp_i32(260 + ramp * 7, 0, 1000);
		ui_copy_status_text(g_ui.snapshot.cc_mode ? "RUNNING CC" : "RUNNING CV");
	} else {
		g_ui.snapshot.vout_mv = 0;
		g_ui.snapshot.iout_ma = 0;
		g_ui.snapshot.cc_mode = false;
		g_ui.snapshot.temp_dC = 285 + ramp / 4;
		g_ui.snapshot.fan_permille = (uint16_t) ui_clamp_i32(120 + ramp * 3, 0, 1000);
		ui_copy_status_text("OUTPUT STANDBY");
	}

	g_ui.snapshot.fault_code = 0U;
	ui_request_body_redraw();
}

static void ui_render_home_body(void) {
	const uint16_t card_w = 193U;
	const uint16_t card_h = 108U;
	const uint16_t x0 = 20U;
	const uint16_t y0 = 72U;
	const uint16_t gap_x = 10U;
	const uint16_t gap_y = 10U;

	ui_draw_metric_card(x0, y0, card_w, card_h, "VIN", g_ui.snapshot.vin_mv, 1000, 3, "V", UI_COLOR_ACCENT, false);
	ui_draw_metric_card((uint16_t) (x0 + card_w + gap_x), y0, card_w, card_h,
					"IIN", g_ui.snapshot.iin_ma, 1000, 3, "A", UI_COLOR_OK, false);
	ui_draw_metric_card((uint16_t) (x0 + (card_w + gap_x) * 2U), y0, card_w, card_h,
					"PIN", ui_get_input_power_mw(), 1000, 3, "W", UI_COLOR_WARN, false);

	ui_draw_metric_card(x0, (uint16_t) (y0 + card_h + gap_y), card_w, card_h,
					"VOUT", g_ui.snapshot.vout_mv, 1000, 3, "V",
					g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_BORDER, false);
	ui_draw_metric_card((uint16_t) (x0 + card_w + gap_x), (uint16_t) (y0 + card_h + gap_y), card_w, card_h,
					"IOUT", g_ui.snapshot.iout_ma, 1000, 3, "A", UI_COLOR_WARN, false);
	ui_draw_metric_card((uint16_t) (x0 + (card_w + gap_x) * 2U), (uint16_t) (y0 + card_h + gap_y), card_w, card_h,
					"POUT", ui_get_output_power_mw(), 1000, 3, "W", UI_COLOR_OK, false);

	ui_draw_metric_card(x0, (uint16_t) (y0 + (card_h + gap_y) * 2U), card_w, card_h,
					"EFF", ui_get_efficiency_permille(), 10, 1, "%", UI_COLOR_OK, false);
	ui_draw_metric_card((uint16_t) (x0 + card_w + gap_x), (uint16_t) (y0 + (card_h + gap_y) * 2U), card_w, card_h,
					"TEMP", g_ui.snapshot.temp_dC, 10, 1, "C",
					(g_ui.snapshot.temp_dC >= 650) ? UI_COLOR_ERROR : UI_COLOR_ACCENT,
					(bool) (g_ui.snapshot.temp_dC >= 650));
	ui_draw_metric_card((uint16_t) (x0 + (card_w + gap_x) * 2U), (uint16_t) (y0 + (card_h + gap_y) * 2U), card_w, card_h,
					"FAN", g_ui.snapshot.fan_permille, 10, 1, "%", UI_COLOR_ACCENT, false);
}

static void ui_render_settings_body(void) {
	char line[56];
	char chip[20];

	UI_DrawText(24U, 72U, "V ROTATE=VSET  I ROTATE=ISET", UI_COLOR_TEXT_DIM, UI_COLOR_BACKGROUND, 1U);
	(void) snprintf(line, sizeof(line), "MODE:%s  FAULT:%04X", g_ui.snapshot.cc_mode ? "CC" : "CV",
					(unsigned int) g_ui.snapshot.fault_code);
	UI_DrawText(24U, 86U, line, UI_COLOR_TEXT_DIM, UI_COLOR_BACKGROUND, 1U);

	ui_draw_metric_card(24U, 106U, 286U, 136U, "VSET", g_ui.snapshot.vset_mv, 1000, 3, "V", UI_COLOR_ACCENT, false);
	ui_draw_metric_card(24U, 252U, 286U, 136U, "ISET", g_ui.snapshot.iset_ma, 1000, 3, "A", UI_COLOR_WARN, false);

	ui_draw_metric_card(334U, 106U, 270U, 84U, "EFF", ui_get_efficiency_permille(), 10, 1, "%", UI_COLOR_OK, false);
	ui_draw_metric_card(334U, 200U, 270U, 84U, "TEMP", g_ui.snapshot.temp_dC, 10, 1, "C",
					(g_ui.snapshot.temp_dC >= 650) ? UI_COLOR_ERROR : UI_COLOR_ACCENT,
					(bool) (g_ui.snapshot.temp_dC >= 650));
	ui_draw_metric_card(334U, 294U, 270U, 84U, "FAN", g_ui.snapshot.fan_permille, 10, 1, "%", UI_COLOR_ACCENT, false);

	ui_draw_status_chip(24U, 398U, 184U, g_ui.snapshot.output_enabled ? "OUTPUT ON" : "OUTPUT OFF",
					 g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_ERROR,
					 g_ui.snapshot.output_enabled);
	(void) snprintf(chip, sizeof(chip), "STEP %uX", (unsigned int) ui_get_adjust_multiplier());
	ui_draw_status_chip(224U, 398U, 180U, chip, UI_COLOR_WARN, true);
	ui_draw_status_chip(420U, 398U, 184U, g_ui.snapshot.status_text, UI_COLOR_ACCENT, false);
}

static void ui_render_menu_body(void) {
	char chip[20];
	const int32_t load_permille = (g_ui.snapshot.iset_ma > 0) ?
		(g_ui.snapshot.iout_ma * 1000) / g_ui.snapshot.iset_ma : 0;

	UI_DrawText(24U, 72U, "LIVE STATUS OVERVIEW", UI_COLOR_TEXT_DIM, UI_COLOR_BACKGROUND, 1U);

	ui_draw_metric_card(20U, 102U, 193U, 118U, "PIN", ui_get_input_power_mw(), 1000, 3, "W", UI_COLOR_WARN, false);
	ui_draw_metric_card(223U, 102U, 193U, 118U, "POUT", ui_get_output_power_mw(), 1000, 3, "W", UI_COLOR_OK, false);
	ui_draw_metric_card(426U, 102U, 193U, 118U, "EFF", ui_get_efficiency_permille(), 10, 1, "%", UI_COLOR_OK, false);

	ui_draw_metric_card(20U, 234U, 193U, 118U, "LOAD", ui_clamp_i32(load_permille, 0, 1000), 10, 1, "%", UI_COLOR_WARN, false);
	ui_draw_metric_card(223U, 234U, 193U, 118U, "TEMP", g_ui.snapshot.temp_dC, 10, 1, "C",
					(g_ui.snapshot.temp_dC >= 650) ? UI_COLOR_ERROR : UI_COLOR_ACCENT,
					(bool) (g_ui.snapshot.temp_dC >= 650));
	ui_draw_metric_card(426U, 234U, 193U, 118U, "FAN", g_ui.snapshot.fan_permille, 10, 1, "%", UI_COLOR_ACCENT, false);

	ui_draw_status_chip(20U, 366U, 184U, g_ui.snapshot.output_enabled ? "OUTPUT ON" : "OUTPUT OFF",
					 g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_ERROR,
					 g_ui.snapshot.output_enabled);
	ui_draw_status_chip(228U, 366U, 184U, g_ui.snapshot.cc_mode ? "MODE CC" : "MODE CV",
					 UI_COLOR_ACCENT, true);
	(void) snprintf(chip, sizeof(chip), "FAULT %04X", (unsigned int) g_ui.snapshot.fault_code);
	ui_draw_status_chip(436U, 366U, 184U, chip,
					 (g_ui.snapshot.fault_code == 0U) ? UI_COLOR_OK : UI_COLOR_ERROR,
					 (g_ui.snapshot.fault_code != 0U));

	UI_DrawText(24U, 412U, g_ui.snapshot.status_text, UI_COLOR_TEXT, UI_COLOR_BACKGROUND, 1U);
}

void UI_Init(void) {
	memset(&g_ui.snapshot, 0, sizeof(g_ui.snapshot));

	g_ui.current_page = UI_PAGE_HOME;
	g_ui.home_focus = 0U;
	g_ui.settings_focus = 0U;
	g_ui.menu_focus = 0U;
	g_ui.enc_v_accum = 0;
	g_ui.enc_i_accum = 0;
	g_ui.adjust_step_index = 0U;
	g_ui.enc_v_last = (int32_t) (uint16_t) __HAL_TIM_GET_COUNTER(&htim4);
	g_ui.enc_i_last = (int32_t) __HAL_TIM_GET_COUNTER(&htim2);
	g_ui.vpush.last_level = (HAL_GPIO_ReadPin(KEY_V_PUSH_GPIO_Port, KEY_V_PUSH_Pin) == GPIO_PIN_SET);
	g_ui.ipush.last_level = (HAL_GPIO_ReadPin(KEY_I_PUSH_GPIO_Port, KEY_I_PUSH_Pin) == GPIO_PIN_SET);
	g_ui.vpush.last_transition_tick = HAL_GetTick();
	g_ui.ipush.last_transition_tick = HAL_GetTick();

	g_ui.snapshot.vin_mv = 24000;
	g_ui.snapshot.iin_ma = 650;
	g_ui.snapshot.vout_mv = 12000;
	g_ui.snapshot.iout_ma = 600;
	g_ui.snapshot.temp_dC = 320;
	g_ui.snapshot.fan_permille = 320U;
	g_ui.snapshot.vset_mv = 12000;
	g_ui.snapshot.iset_ma = 1500;
	g_ui.snapshot.output_enabled = true;
	g_ui.snapshot.cc_mode = false;
	g_ui.snapshot.fault_code = 0U;
	ui_copy_status_text(g_ui.demo_enabled ? "DEMO READY" : "READY");
	ui_request_full_redraw();

	UI_Render();
}

void UI_Tick(void) {
	ui_poll_push_buttons();
	ui_poll_encoders();
	ui_update_demo();
	ui_process_pending_keys();

	if (g_ui.dirty) {
		UI_Render();
	}
}

void UI_Render(void) {
	const bool was_full_redraw = g_ui.full_redraw;

	if (g_ui.full_redraw) {
		switch (g_ui.current_page) {
			case UI_PAGE_HOME:
				UI_PageDrawFrame("POWER METRICS", UI_PAGE_HOME);
				ui_render_home_body();
				UI_PageDrawFooterHints("M PAGE", "V PUSH OUTPUT", "I PUSH STEP");
				break;
			case UI_PAGE_SETTINGS:
				UI_PageDrawFrame("SETPOINT TUNE", UI_PAGE_SETTINGS);
				ui_render_settings_body();
				UI_PageDrawFooterHints("L/R VSET  U/D ISET", "M PAGE", "ENC V/I ADJUST");
				break;
			case UI_PAGE_MENU:
				UI_PageDrawFrame("SYSTEM STATE", UI_PAGE_MENU);
				ui_render_menu_body();
				UI_PageDrawFooterHints("LEFT POWER", "M PAGE", "RIGHT SET");
				break;
			default:
				break;
		}
	} else {
		if (g_ui.dirty_rect.valid) {
			LCD_CopyRectFromFrontToDraw(g_ui.dirty_rect.x, g_ui.dirty_rect.y, g_ui.dirty_rect.w, g_ui.dirty_rect.h);
		}

		switch (g_ui.current_page) {
			case UI_PAGE_HOME:
				ui_render_home_body();
				break;
			case UI_PAGE_SETTINGS:
				ui_render_settings_body();
				break;
			case UI_PAGE_MENU:
				ui_render_menu_body();
				break;
			default:
				break;
		}
	}

	LCD_Present();
	if (was_full_redraw) {
		LCD_CopyRectFromFrontToDraw(0U, 0U, LCD_DEV.width, LCD_DEV.height);
	}

	g_ui.full_redraw = false;
	g_ui.dirty_rect.valid = false;
	g_ui.dirty = false;
}

void UI_RequestRedraw(void) {
	ui_request_full_redraw();
}

void UI_OnKeyInterrupt(const uint16_t gpio_pin) {
	const uint16_t irq_index = ui_get_irq_index(gpio_pin);
	const uint32_t now = HAL_GetTick();
	uint32_t mask = 0U;

	if ((now - g_irq_debounce_tick[irq_index]) < 100U) {
		return;
	}
	g_irq_debounce_tick[irq_index] = now;

	if (gpio_pin == KEY_UP_Pin) {
		mask = UI_KEY_MASK_UP;
	} else if (gpio_pin == KEY_DN_Pin) {
		mask = UI_KEY_MASK_DOWN;
	} else if (gpio_pin == KEY_L_Pin) {
		mask = UI_KEY_MASK_LEFT;
	} else if (gpio_pin == KEY_R_Pin) {
		mask = UI_KEY_MASK_RIGHT;
	} else if (gpio_pin == KEY_M_Pin) {
		mask = UI_KEY_MASK_ENTER;
	}

	g_pending_key_mask |= mask;
}

void UI_SetDemoEnabled(const bool enabled) {
	g_ui.demo_enabled = enabled;
	ui_copy_status_text(enabled ? "DEMO ENABLED" : "DEMO DISABLED");
	ui_request_body_redraw();
}

void UI_SetActionCallback(const ui_action_callback_t callback, void *user_data) {
	g_ui.callback = callback;
	g_ui.callback_user_data = user_data;
}

void UI_SetPowerSnapshot(const ui_power_snapshot_t *snapshot) {
	if (snapshot == NULL) {
		return;
	}

	g_ui.snapshot = *snapshot;
	ui_request_body_redraw();
}

void UI_GetPowerSnapshot(ui_power_snapshot_t *snapshot) {
	if (snapshot == NULL) {
		return;
	}

	*snapshot = g_ui.snapshot;
}

ui_page_id_t UI_GetCurrentPage(void) {
	return g_ui.current_page;
}
