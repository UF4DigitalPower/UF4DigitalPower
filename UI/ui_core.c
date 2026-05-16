/**
 * @file    : ui_core.c
 * @brief   : UI 核心：绘制、输入、页面调度、演示数据
 */

#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "gpio.h"
#include "tim.h"

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
	ui_push_button_t vpush;
	ui_push_button_t ipush;
} ui_state_t;

static ui_state_t g_ui = {
	.dirty = true,
};

static volatile uint32_t g_pending_key_mask = 0U;
static uint32_t g_irq_debounce_tick[5] = {0U};

static void ui_render_home(void);
static void ui_render_settings(void);
static void ui_render_menu(void);
static void ui_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
static void ui_process_pending_keys(void);
static void ui_handle_event(ui_event_t event);
static void ui_handle_home_event(ui_event_t event);
static void ui_handle_settings_event(ui_event_t event);
static void ui_handle_menu_event(ui_event_t event);
static void ui_emit_action(ui_action_t action);
static void ui_go_page(ui_page_id_t page);
static void ui_toggle_output(void);
static void ui_apply_settings(void);
static void ui_poll_push_buttons(void);
static void ui_poll_encoders(void);
static void ui_update_demo(void);
static void ui_copy_status_text(const char *text);
static void ui_adjust_voltage(int32_t delta_mv);
static void ui_adjust_current(int32_t delta_ma);
static int32_t ui_clamp_i32(int32_t value, int32_t min_value, int32_t max_value);
static uint16_t ui_clamp_percent(int32_t value);
static uint16_t ui_get_irq_index(uint16_t gpio_pin);
static bool ui_get_glyph_rows(char ch, uint8_t rows[7]);
static uint16_t ui_to_upper_ascii(uint16_t ch);
static int32_t ui_pow10(uint8_t digits);

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
		*(uint16_t *) (draw_addr + LCD_DEV.pixsize * (LTDC_WIDTH * (LTDC_HEIGHT - x - 1U) + y)) = color;
	} else {
		*(uint16_t *) (draw_addr + LCD_DEV.pixsize * (LTDC_WIDTH * y + x)) = color;
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

static uint16_t ui_clamp_percent(const int32_t value) {
	if (value <= 0) {
		return 0U;
	}
	if (value >= 100) {
		return 100U;
	}
	return (uint16_t) value;
}

static int32_t ui_pow10(const uint8_t digits) {
	int32_t result = 1;
	uint8_t i;

	for (i = 0U; i < digits; ++i) {
		result *= 10;
	}
	return result;
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

static void ui_go_page(const ui_page_id_t page) {
	if (g_ui.current_page != page) {
		g_ui.current_page = page;
		g_ui.dirty = true;
		ui_emit_action(UI_ACTION_PAGE_CHANGED);
	}
}

static void ui_toggle_output(void) {
	g_ui.snapshot.output_enabled = !g_ui.snapshot.output_enabled;
	ui_copy_status_text(g_ui.snapshot.output_enabled ? "OUTPUT ON" : "OUTPUT OFF");
	g_ui.dirty = true;
	ui_emit_action(UI_ACTION_OUTPUT_TOGGLED);
}

static void ui_apply_settings(void) {
	ui_copy_status_text("SETTINGS APPLIED");
	g_ui.dirty = true;
	ui_emit_action(UI_ACTION_APPLY_SETTINGS);
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
	uint16_t cursor_x = x;
	uint16_t row;
	uint16_t col;
	uint8_t rows[7];
	uint8_t sx;
	uint8_t sy;

	(void) bg_color;

	if (text == NULL || scale == 0U) {
		return;
	}

	while (*text != '\0') {
		(void) ui_get_glyph_rows(*text, rows);
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
		++text;
	}
}

uint16_t UI_TextWidth(const char *text, const uint8_t scale) {
	uint16_t len = 0U;

	if (text == NULL || scale == 0U) {
		return 0U;
	}

	while (*text != '\0') {
		++len;
		++text;
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
	text_h = (uint16_t) (7U * scale);

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
	ui_copy_status_text("VSET CHANGED");
	g_ui.dirty = true;
}

static void ui_adjust_current(const int32_t delta_ma) {
	g_ui.snapshot.iset_ma = ui_clamp_i32(g_ui.snapshot.iset_ma + delta_ma, 0, 5000);
	ui_copy_status_text("ISET CHANGED");
	g_ui.dirty = true;
}

static void ui_handle_home_event(const ui_event_t event) {
	switch (event) {
		case UI_EVT_KEY_LEFT:
		case UI_EVT_KEY_UP:
			g_ui.home_focus = (uint8_t) ((g_ui.home_focus + 2U) % 3U);
			g_ui.dirty = true;
			break;
		case UI_EVT_KEY_RIGHT:
		case UI_EVT_KEY_DOWN:
			g_ui.home_focus = (uint8_t) ((g_ui.home_focus + 1U) % 3U);
			g_ui.dirty = true;
			break;
		case UI_EVT_KEY_ENTER:
			if (g_ui.home_focus == 0U) {
				ui_go_page(UI_PAGE_MENU);
			} else if (g_ui.home_focus == 1U) {
				ui_go_page(UI_PAGE_SETTINGS);
			} else {
				ui_toggle_output();
			}
			break;
		case UI_EVT_VENC_PUSH:
			ui_go_page(UI_PAGE_SETTINGS);
			break;
		case UI_EVT_IENC_PUSH:
			ui_toggle_output();
			break;
		default:
			break;
	}
}

static void ui_handle_settings_event(const ui_event_t event) {
	switch (event) {
		case UI_EVT_KEY_UP:
			g_ui.settings_focus = (g_ui.settings_focus == 0U) ? 4U : (uint8_t) (g_ui.settings_focus - 1U);
			g_ui.dirty = true;
			break;
		case UI_EVT_KEY_DOWN:
			g_ui.settings_focus = (uint8_t) ((g_ui.settings_focus + 1U) % 5U);
			g_ui.dirty = true;
			break;
		case UI_EVT_KEY_LEFT:
			if (g_ui.settings_focus == 0U) {
				ui_adjust_voltage(-100);
			} else if (g_ui.settings_focus == 1U) {
				ui_adjust_current(-50);
			} else if (g_ui.settings_focus == 2U) {
				ui_toggle_output();
			} else if (g_ui.settings_focus == 4U) {
				ui_go_page(UI_PAGE_HOME);
			}
			break;
		case UI_EVT_KEY_RIGHT:
			if (g_ui.settings_focus == 0U) {
				ui_adjust_voltage(100);
			} else if (g_ui.settings_focus == 1U) {
				ui_adjust_current(50);
			} else if (g_ui.settings_focus == 2U) {
				ui_toggle_output();
			}
			break;
		case UI_EVT_KEY_ENTER:
			if (g_ui.settings_focus == 2U) {
				ui_toggle_output();
			} else if (g_ui.settings_focus == 3U) {
				ui_apply_settings();
			} else if (g_ui.settings_focus == 4U) {
				ui_go_page(UI_PAGE_HOME);
			}
			break;
		case UI_EVT_VENC_CW:
			ui_adjust_voltage(100);
			break;
		case UI_EVT_VENC_CCW:
			ui_adjust_voltage(-100);
			break;
		case UI_EVT_IENC_CW:
			ui_adjust_current(50);
			break;
		case UI_EVT_IENC_CCW:
			ui_adjust_current(-50);
			break;
		case UI_EVT_VENC_PUSH:
			ui_apply_settings();
			break;
		case UI_EVT_IENC_PUSH:
			ui_toggle_output();
			break;
		default:
			break;
	}
}

static void ui_handle_menu_event(const ui_event_t event) {
	switch (event) {
		case UI_EVT_KEY_UP:
			g_ui.menu_focus = (g_ui.menu_focus == 0U) ? 3U : (uint8_t) (g_ui.menu_focus - 1U);
			g_ui.dirty = true;
			break;
		case UI_EVT_KEY_DOWN:
			g_ui.menu_focus = (uint8_t) ((g_ui.menu_focus + 1U) % 4U);
			g_ui.dirty = true;
			break;
		case UI_EVT_KEY_LEFT:
			ui_go_page(UI_PAGE_HOME);
			break;
		case UI_EVT_KEY_RIGHT:
			ui_go_page(UI_PAGE_SETTINGS);
			break;
		case UI_EVT_KEY_ENTER:
			if (g_ui.menu_focus == 0U) {
				ui_go_page(UI_PAGE_HOME);
			} else if (g_ui.menu_focus == 1U) {
				ui_go_page(UI_PAGE_SETTINGS);
			} else if (g_ui.menu_focus == 2U) {
				ui_toggle_output();
			} else {
				ui_apply_settings();
			}
			break;
		case UI_EVT_VENC_PUSH:
			ui_go_page(UI_PAGE_SETTINGS);
			break;
		case UI_EVT_IENC_PUSH:
			ui_toggle_output();
			break;
		default:
			break;
	}
}

static void ui_handle_event(const ui_event_t event) {
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

	if (g_ui.snapshot.output_enabled) {
		g_ui.snapshot.vout_mv = g_ui.snapshot.vset_mv - 80 + ramp * 2;
		g_ui.snapshot.iout_ma = ui_clamp_i32((g_ui.snapshot.iset_ma * (40 + ramp / 2)) / 100, 0, g_ui.snapshot.iset_ma);
		g_ui.snapshot.cc_mode = (bool) (g_ui.snapshot.iout_ma >= (g_ui.snapshot.iset_ma - 80));
		g_ui.snapshot.temp_dC = 315 + ramp / 2;
		ui_copy_status_text(g_ui.snapshot.cc_mode ? "RUNNING CC" : "RUNNING CV");
	} else {
		g_ui.snapshot.vout_mv = 0;
		g_ui.snapshot.iout_ma = 0;
		g_ui.snapshot.cc_mode = false;
		g_ui.snapshot.temp_dC = 285 + ramp / 4;
		ui_copy_status_text("OUTPUT STANDBY");
	}

	g_ui.snapshot.fault_code = 0U;
	g_ui.dirty = true;
}

static void ui_render_home(void) {
	char status_line[48];
	int32_t power_mw;
	int32_t load_percent;
	int32_t temp_percent;
	ui_value_widget_t card;
	ui_bar_t bar;
	ui_button_t button;

	UI_PageDrawFrame("POWER DASHBOARD", UI_PAGE_HOME);

	card = (ui_value_widget_t) {
		.x = 20U, .y = 76U, .w = 290U, .h = 104U,
		.title = "VIN", .raw_value = g_ui.snapshot.vin_mv, .divisor = 1000, .decimals = 3,
		.unit = "V", .accent_color = UI_COLOR_ACCENT, .focused = false, .warning = false,
	};
	UI_ValueDraw(&card);

	card.x = 330U; card.y = 76U; card.title = "IIN"; card.raw_value = g_ui.snapshot.iin_ma; card.unit = "A";
	card.accent_color = UI_COLOR_OK;
	UI_ValueDraw(&card);

	card.x = 20U; card.y = 192U; card.title = "VOUT"; card.raw_value = g_ui.snapshot.vout_mv;
	card.accent_color = g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_BORDER;
	UI_ValueDraw(&card);

	card.x = 330U; card.y = 192U; card.title = "IOUT"; card.raw_value = g_ui.snapshot.iout_ma;
	card.accent_color = UI_COLOR_WARN;
	UI_ValueDraw(&card);

	load_percent = (g_ui.snapshot.iset_ma > 0) ? ((g_ui.snapshot.iout_ma * 100) / g_ui.snapshot.iset_ma) : 0;
	bar = (ui_bar_t) {
		.x = 20U, .y = 364U, .w = 600U, .h = 30U,
		.title = "LOAD", .value = ui_clamp_percent(load_percent), .max = 100U,
		.fill_color = UI_COLOR_WARN, .bg_color = UI_COLOR_PANEL, .focused = false,
	};
	UI_BarDraw(&bar);

	temp_percent = ((g_ui.snapshot.temp_dC - 250) * 100) / 450;
	bar.y = 404U;
	bar.title = "TEMP";
	bar.value = ui_clamp_percent(temp_percent);
	bar.fill_color = (g_ui.snapshot.temp_dC >= 650) ? UI_COLOR_ERROR : UI_COLOR_OK;
	UI_BarDraw(&bar);

	power_mw = (int32_t) (((int64_t) g_ui.snapshot.vout_mv * g_ui.snapshot.iout_ma) / 1000LL);
	(void) snprintf(status_line, sizeof(status_line), "STATUS:%s MODE:%s POUT:%ld.%01ldW",
					g_ui.snapshot.output_enabled ? "ON" : "OFF",
					g_ui.snapshot.cc_mode ? "CC" : "CV",
					(long) (power_mw / 1000), (long) ((power_mw % 1000) / 100));

	UI_DrawText(20U, 438U, status_line, UI_COLOR_TEXT, UI_COLOR_BACKGROUND, 1U);

	button = (ui_button_t) {
		.x = 20U, .y = 314U, .w = 120U, .h = 36U,
		.text = "MENU", .text_color = UI_COLOR_TEXT, .bg_color = UI_COLOR_PANEL,
		.border_color = UI_COLOR_BORDER, .accent_color = UI_COLOR_ACCENT,
		.focused = (bool) (g_ui.home_focus == 0U), .active = false,
	};
	UI_ButtonDraw(&button);

	button.x = 154U; button.text = "SETTINGS"; button.w = 140U; button.focused = (bool) (g_ui.home_focus == 1U);
	UI_ButtonDraw(&button);

	button.x = 308U; button.w = 160U; button.text = g_ui.snapshot.output_enabled ? "OUTPUT ON" : "OUTPUT OFF";
	button.focused = (bool) (g_ui.home_focus == 2U);
	button.active = g_ui.snapshot.output_enabled;
	button.accent_color = g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_ERROR;
	UI_ButtonDraw(&button);

	UI_PageDrawFooterHints("UP/DN/L/R MOVE", "M ENTER", "V PUSH:SET  I PUSH:OUT");
}

static void ui_render_settings(void) {
	char line[48];
	int32_t vout_percent;
	int32_t iout_percent;
	ui_value_widget_t card;
	ui_button_t button;
	ui_bar_t bar;

	UI_PageDrawFrame("OUTPUT SETTINGS", UI_PAGE_SETTINGS);

	UI_DrawText(24U, 72U, "USE ENCODER V/I TO TUNE SETPOINTS", UI_COLOR_TEXT_DIM, UI_COLOR_BACKGROUND, 1U);

	card = (ui_value_widget_t) {
		.x = 24U, .y = 104U, .w = 280U, .h = 104U,
		.title = "VSET", .raw_value = g_ui.snapshot.vset_mv, .divisor = 1000, .decimals = 3,
		.unit = "V", .accent_color = UI_COLOR_ACCENT, .focused = (bool) (g_ui.settings_focus == 0U), .warning = false,
	};
	UI_ValueDraw(&card);

	card.y = 220U; card.title = "ISET"; card.raw_value = g_ui.snapshot.iset_ma; card.unit = "A";
	card.accent_color = UI_COLOR_WARN; card.focused = (bool) (g_ui.settings_focus == 1U);
	UI_ValueDraw(&card);

	vout_percent = (g_ui.snapshot.vset_mv > 0) ? ((g_ui.snapshot.vout_mv * 100) / g_ui.snapshot.vset_mv) : 0;
	bar = (ui_bar_t) {
		.x = 334U, .y = 118U, .w = 270U, .h = 32U,
		.title = "VOUT/VSET", .value = ui_clamp_percent(vout_percent), .max = 100U,
		.fill_color = UI_COLOR_ACCENT, .bg_color = UI_COLOR_PANEL, .focused = false,
	};
	UI_BarDraw(&bar);

	iout_percent = (g_ui.snapshot.iset_ma > 0) ? ((g_ui.snapshot.iout_ma * 100) / g_ui.snapshot.iset_ma) : 0;
	bar.y = 166U;
	bar.title = "IOUT/ISET";
	bar.value = ui_clamp_percent(iout_percent);
	bar.fill_color = UI_COLOR_WARN;
	UI_BarDraw(&bar);

	bar.y = 214U;
	bar.title = "TEMP";
	bar.value = ui_clamp_percent(((g_ui.snapshot.temp_dC - 250) * 100) / 450);
	bar.fill_color = (g_ui.snapshot.temp_dC > 650) ? UI_COLOR_ERROR : UI_COLOR_OK;
	UI_BarDraw(&bar);

	button = (ui_button_t) {
		.x = 334U, .y = 286U, .w = 270U, .h = 40U,
		.text = g_ui.snapshot.output_enabled ? "OUTPUT ON" : "OUTPUT OFF",
		.text_color = UI_COLOR_TEXT, .bg_color = UI_COLOR_PANEL, .border_color = UI_COLOR_BORDER,
		.accent_color = g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_ERROR,
		.focused = (bool) (g_ui.settings_focus == 2U), .active = g_ui.snapshot.output_enabled,
	};
	UI_ButtonDraw(&button);

	button.y = 338U; button.text = "APPLY"; button.accent_color = UI_COLOR_ACCENT;
	button.focused = (bool) (g_ui.settings_focus == 3U); button.active = false;
	UI_ButtonDraw(&button);

	button.y = 390U; button.text = "BACK"; button.accent_color = UI_COLOR_WARN;
	button.focused = (bool) (g_ui.settings_focus == 4U);
	UI_ButtonDraw(&button);

	(void) snprintf(line, sizeof(line), "STATUS:%s FAULT:%04X", g_ui.snapshot.status_text, g_ui.snapshot.fault_code);
	UI_DrawText(24U, 370U, line, UI_COLOR_TEXT, UI_COLOR_BACKGROUND, 1U);
	UI_DrawText(24U, 394U, "KEY LEFT/RIGHT = FINE ADJUST", UI_COLOR_TEXT_DIM, UI_COLOR_BACKGROUND, 1U);

	UI_PageDrawFooterHints("UP/DN FOCUS", "M CLICK", "ENC V/I ADJUST");
}

static void ui_render_menu(void) {
	ui_button_t button;
	ui_value_widget_t card;
	static const char *items[4] = {"DASHBOARD", "SETTINGS", "TOGGLE OUTPUT", "APPLY"};
	uint8_t i;

	UI_PageDrawFrame("MAIN MENU", UI_PAGE_MENU);
	UI_DrawText(24U, 72U, "SIMPLE POWER UI DEMO", UI_COLOR_TEXT_DIM, UI_COLOR_BACKGROUND, 1U);

	for (i = 0U; i < 4U; ++i) {
		button = (ui_button_t) {
			.x = 24U,
			.y = (uint16_t) (104U + i * 62U),
			.w = 248U,
			.h = 46U,
			.text = items[i],
			.text_color = UI_COLOR_TEXT,
			.bg_color = UI_COLOR_PANEL,
			.border_color = UI_COLOR_BORDER,
			.accent_color = UI_COLOR_ACCENT,
			.focused = (bool) (g_ui.menu_focus == i),
			.active = (bool) (i == 2U && g_ui.snapshot.output_enabled),
		};
		if (i == 2U) {
			button.accent_color = g_ui.snapshot.output_enabled ? UI_COLOR_OK : UI_COLOR_ERROR;
		}
		UI_ButtonDraw(&button);
	}

	card = (ui_value_widget_t) {
		.x = 308U, .y = 104U, .w = 296U, .h = 96U,
		.title = "VOUT", .raw_value = g_ui.snapshot.vout_mv, .divisor = 1000, .decimals = 3,
		.unit = "V", .accent_color = UI_COLOR_OK, .focused = false, .warning = false,
	};
	UI_ValueDraw(&card);

	card.y = 216U; card.title = "IOUT"; card.raw_value = g_ui.snapshot.iout_ma; card.unit = "A"; card.accent_color = UI_COLOR_WARN;
	UI_ValueDraw(&card);

	card.y = 328U; card.title = "TEMP"; card.raw_value = g_ui.snapshot.temp_dC; card.divisor = 10; card.decimals = 1; card.unit = "C";
	card.accent_color = UI_COLOR_ACCENT;
	UI_ValueDraw(&card);

	UI_DrawText(308U, 438U, g_ui.snapshot.status_text, UI_COLOR_TEXT, UI_COLOR_BACKGROUND, 1U);
	UI_PageDrawFooterHints("UP/DN SELECT", "M EXECUTE", "LEFT HOME  RIGHT SET");
}

void UI_Init(void) {
	memset(&g_ui.snapshot, 0, sizeof(g_ui.snapshot));

	g_ui.current_page = UI_PAGE_HOME;
	g_ui.home_focus = 0U;
	g_ui.settings_focus = 0U;
	g_ui.menu_focus = 0U;
	g_ui.enc_v_accum = 0;
	g_ui.enc_i_accum = 0;
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
	g_ui.snapshot.vset_mv = 12000;
	g_ui.snapshot.iset_ma = 1500;
	g_ui.snapshot.output_enabled = true;
	g_ui.snapshot.cc_mode = false;
	g_ui.snapshot.fault_code = 0U;
	ui_copy_status_text(g_ui.demo_enabled ? "DEMO READY" : "READY");
	g_ui.dirty = true;

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
	switch (g_ui.current_page) {
		case UI_PAGE_HOME:
			ui_render_home();
			break;
		case UI_PAGE_SETTINGS:
			ui_render_settings();
			break;
		case UI_PAGE_MENU:
			ui_render_menu();
			break;
		default:
			break;
	}

	LCD_Present();

	g_ui.dirty = false;
}

void UI_RequestRedraw(void) {
	g_ui.dirty = true;
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
	g_ui.dirty = true;
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
	g_ui.dirty = true;
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
