/**
 * @file    : ui_value.c
 * @brief   : 数值卡片控件绘制
 */

#include "ui_value.h"

void UI_ValueDraw(const ui_value_widget_t *widget) {
	char value_text[24];
	uint16_t border_color;
	uint16_t accent_color;

	if (widget == NULL || widget->title == NULL || widget->unit == NULL) {
		return;
	}

	accent_color = widget->accent_color;
	border_color = widget->focused ? UI_COLOR_WARN : UI_COLOR_BORDER;
	if (widget->warning) {
		border_color = UI_COLOR_ERROR;
		accent_color = UI_COLOR_ERROR;
	}

	UI_FillRect(widget->x, widget->y, widget->w, widget->h, UI_COLOR_CARD);
	UI_DrawRect(widget->x, widget->y, widget->w, widget->h, border_color);
	UI_FillRect(widget->x + 1U, widget->y + 1U, widget->w - 2U, 4U, accent_color);

	UI_DrawText(widget->x + 10U, widget->y + 12U, widget->title, UI_COLOR_TEXT_DIM, UI_COLOR_CARD, 1U);

	UI_FormatScaled(value_text, sizeof(value_text), widget->raw_value, widget->divisor, widget->decimals);
	UI_DrawText(widget->x + 10U, widget->y + 42U, value_text, UI_COLOR_TEXT, UI_COLOR_CARD, 3U);
	UI_DrawTextBox(widget->x + widget->w - 60U, widget->y + 48U, 50U, 26U, widget->unit,
				   UI_COLOR_TEXT_DIM, UI_COLOR_CARD, 1U, UI_ALIGN_RIGHT);
}
