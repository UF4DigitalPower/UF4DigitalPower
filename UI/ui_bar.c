/**
 * @file    : ui_bar.c
 * @brief   : 条形图控件绘制
 */

#include "ui_bar.h"

#include <stdio.h>

void UI_BarDraw(const ui_bar_t *bar) {
	char value_text[16];
	uint16_t inner_x;
	uint16_t inner_y;
	uint16_t inner_w;
	uint16_t inner_h;
	uint16_t fill_w;
	uint32_t scaled;

	if (bar == NULL || bar->title == NULL || bar->max == 0U) {
		return;
	}

	UI_FillRect(bar->x, bar->y, bar->w, bar->h, UI_COLOR_CARD);
	UI_DrawRect(bar->x, bar->y, bar->w, bar->h, bar->focused ? UI_COLOR_WARN : UI_COLOR_BORDER);

	UI_DrawText(bar->x + 8U, bar->y + 6U, bar->title, UI_COLOR_TEXT, UI_COLOR_CARD, 1U);

	inner_x = bar->x + 78U;
	inner_y = bar->y + 8U;
	inner_w = (bar->w > 150U) ? (uint16_t) (bar->w - 150U) : 20U;
	inner_h = (bar->h > 16U) ? (uint16_t) (bar->h - 16U) : 8U;

	UI_FillRect(inner_x, inner_y, inner_w, inner_h, bar->bg_color);
	UI_DrawRect(inner_x, inner_y, inner_w, inner_h, UI_COLOR_BORDER);

	scaled = ((uint32_t) bar->value * inner_w) / bar->max;
	fill_w = (scaled > inner_w) ? inner_w : (uint16_t) scaled;
	if (fill_w > 0U) {
		UI_FillRect(inner_x + 1U, inner_y + 1U,
					(fill_w > 2U) ? (uint16_t) (fill_w - 2U) : 1U,
					(inner_h > 2U) ? (uint16_t) (inner_h - 2U) : 1U,
					bar->fill_color);
	}

	(void) snprintf(value_text, sizeof(value_text), "%3u%%", (unsigned int) ((bar->value * 100U) / bar->max));
	UI_DrawTextBox(bar->x + bar->w - 60U, bar->y, 52U, bar->h, value_text,
				   UI_COLOR_TEXT, UI_COLOR_CARD, 1U, UI_ALIGN_RIGHT);
}
