/**
 * @file    : ui.h
 * @brief   : 轻量级电源参数显示 UI 库
 * @author  : UF4
 * @date    : 2026/5/16
 * @project : STM32H743
 */

#ifndef STM32H743_UI_H
#define STM32H743_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_STATUS_TEXT_MAX_LEN 20U

#define UI_COLOR_BACKGROUND ((uint16_t)0x0841)
#define UI_COLOR_PANEL      ((uint16_t)0x10A2)
#define UI_COLOR_CARD       ((uint16_t)0x18E3)
#define UI_COLOR_BORDER     ((uint16_t)0x3186)
#define UI_COLOR_TEXT       ((uint16_t)0xFFFF)
#define UI_COLOR_TEXT_DIM   ((uint16_t)0xC618)
#define UI_COLOR_TEXT_DARK  ((uint16_t)0x0000)
#define UI_COLOR_ACCENT     ((uint16_t)0x2D7F)
#define UI_COLOR_OK         ((uint16_t)0x3666)
#define UI_COLOR_WARN       ((uint16_t)0xFD20)
#define UI_COLOR_ERROR      ((uint16_t)0xF8C3)

typedef enum {
	UI_ALIGN_LEFT = 0,
	UI_ALIGN_CENTER,
	UI_ALIGN_RIGHT,
} ui_align_t;

typedef enum {
	UI_PAGE_HOME = 0,
	UI_PAGE_SETTINGS,
	UI_PAGE_MENU,
	UI_PAGE_COUNT,
} ui_page_id_t;

typedef enum {
	UI_ACTION_NONE = 0,
	UI_ACTION_PAGE_CHANGED,
	UI_ACTION_OUTPUT_TOGGLED,
	UI_ACTION_APPLY_SETTINGS,
} ui_action_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	const char *text;
	uint16_t text_color;
	uint16_t bg_color;
	uint8_t scale;
	ui_align_t align;
	bool filled;
} ui_label_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	const char *text;
	uint16_t text_color;
	uint16_t bg_color;
	uint16_t border_color;
	uint16_t accent_color;
	bool focused;
	bool active;
} ui_button_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	const char *title;
	int32_t raw_value;
	int32_t divisor;
	uint8_t decimals;
	const char *unit;
	uint16_t accent_color;
	bool focused;
	bool warning;
} ui_value_widget_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	const char *title;
	uint16_t value;
	uint16_t max;
	uint16_t fill_color;
	uint16_t bg_color;
	bool focused;
} ui_bar_t;

typedef struct {
	int32_t vin_mv;
	int32_t iin_ma;
	int32_t vout_mv;
	int32_t iout_ma;
	int32_t temp_dC;
	int32_t vset_mv;
	int32_t iset_ma;
	uint16_t fault_code;
	bool output_enabled;
	bool cc_mode;
	char status_text[UI_STATUS_TEXT_MAX_LEN];
} ui_power_snapshot_t;

typedef void (*ui_action_callback_t)(ui_action_t action, const ui_power_snapshot_t *snapshot, void *user_data);

void UI_Init(void);
void UI_Tick(void);
void UI_Render(void);
void UI_RequestRedraw(void);
void UI_OnKeyInterrupt(uint16_t gpio_pin);

void UI_SetDemoEnabled(bool enabled);
void UI_SetActionCallback(ui_action_callback_t callback, void *user_data);
void UI_SetPowerSnapshot(const ui_power_snapshot_t *snapshot);
void UI_GetPowerSnapshot(ui_power_snapshot_t *snapshot);
ui_page_id_t UI_GetCurrentPage(void);

void UI_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void UI_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void UI_DrawText(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg_color, uint8_t scale);
void UI_DrawTextBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *text,
					uint16_t color, uint16_t bg_color, uint8_t scale, ui_align_t align);
uint16_t UI_TextWidth(const char *text, uint8_t scale);
void UI_FormatScaled(char *buffer, size_t size, int32_t raw_value, int32_t divisor, uint8_t decimals);

void UI_LabelDraw(const ui_label_t *label);
void UI_ButtonDraw(const ui_button_t *button);
void UI_ValueDraw(const ui_value_widget_t *widget);
void UI_BarDraw(const ui_bar_t *bar);
void UI_PageDrawFrame(const char *title, ui_page_id_t current_page);
void UI_PageDrawFooterHints(const char *left_hint, const char *center_hint, const char *right_hint);

#ifdef __cplusplus
}
#endif

#endif /* STM32H743_UI_H */