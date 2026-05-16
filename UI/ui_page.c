/**
 * @file    : ui_page.c
 * @brief   : 页面公共框架绘制
 */

#include "ui_page.h"

void UI_PageDrawFrame(const char *title, const ui_page_id_t current_page) {
	static const char *tabs[UI_PAGE_COUNT] = {"POWER", "SET", "STATE"};
	uint16_t tab_x = 330U;
	uint8_t i;

	UI_FillRect(0U, 0U, LCD_DEV.width, LCD_DEV.height, UI_COLOR_BACKGROUND);

	UI_FillRect(0U, 0U, LCD_DEV.width, 56U, UI_COLOR_PANEL);
	UI_FillRect(0U, 56U, LCD_DEV.width, 2U, UI_COLOR_ACCENT);

	UI_DrawText(18U, 18U, title, UI_COLOR_TEXT, UI_COLOR_PANEL, 2U);

	for (i = 0U; i < (uint8_t) UI_PAGE_COUNT; ++i) {
		ui_button_t button = {
			.x = tab_x,
			.y = 10U,
			.w = 96U,
			.h = 36U,
			.text = tabs[i],
			.text_color = UI_COLOR_TEXT,
			.bg_color = UI_COLOR_PANEL,
			.border_color = UI_COLOR_BORDER,
			.accent_color = UI_COLOR_ACCENT,
			.focused = (bool) (i == (uint8_t) current_page),
			.active = false,
			.scale = 2U,
		};
		UI_ButtonDraw(&button);
		tab_x = (uint16_t) (tab_x + 102U);
	}
}

void UI_PageDrawFooterHints(const char *left_hint, const char *center_hint, const char *right_hint) {
	UI_FillRect(0U, LCD_DEV.height - 30U, LCD_DEV.width, 30U, UI_COLOR_PANEL);
	UI_FillRect(0U, LCD_DEV.height - 30U, LCD_DEV.width, 1U, UI_COLOR_BORDER);

	if (left_hint != NULL) {
		UI_DrawText(12U, LCD_DEV.height - 22U, left_hint, UI_COLOR_TEXT_DIM, UI_COLOR_PANEL, 1U);
	}
	if (center_hint != NULL) {
		UI_DrawTextBox(0U, LCD_DEV.height - 26U, LCD_DEV.width, 20U, center_hint,
					   UI_COLOR_TEXT_DIM, UI_COLOR_PANEL, 1U, UI_ALIGN_CENTER);
	}
	if (right_hint != NULL) {
		UI_DrawTextBox(0U, LCD_DEV.height - 26U, LCD_DEV.width - 12U, 20U, right_hint,
					   UI_COLOR_TEXT_DIM, UI_COLOR_PANEL, 1U, UI_ALIGN_RIGHT);
	}
}
