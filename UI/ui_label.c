/**
 * @file    : ui_label.c
 * @brief   : 标签与按钮控件绘制
 */

#include "ui_label.h"

void UI_LabelDraw(const ui_label_t *label) {
	if (label == NULL || label->text == NULL) {
		return;
	}

	if (label->filled) {
		UI_FillRect(label->x, label->y, label->w, label->h, label->bg_color);
	}

	UI_DrawTextBox(label->x, label->y, label->w, label->h, label->text,
				   label->text_color, label->bg_color, label->scale, label->align);
}

void UI_ButtonDraw(const ui_button_t *button) {
	uint16_t fill_color;
	uint16_t text_color;
	uint16_t border_color;

	if (button == NULL || button->text == NULL) {
		return;
	}

	fill_color = button->bg_color;
	text_color = button->text_color;
	border_color = button->border_color;

	if (button->active) {
		border_color = button->accent_color;
		text_color = button->accent_color;
	}

	if (button->focused) {
		fill_color = button->accent_color;
		text_color = UI_COLOR_TEXT_DARK;
		border_color = button->accent_color;
	}

	UI_FillRect(button->x, button->y, button->w, button->h, fill_color);
	UI_DrawRect(button->x, button->y, button->w, button->h, border_color);

	if (button->w > 4U && button->h > 4U) {
		UI_DrawRect(button->x + 2U, button->y + 2U, button->w - 4U, button->h - 4U,
					button->focused ? button->accent_color : UI_COLOR_PANEL);
	}

	UI_DrawTextBox(button->x, button->y, button->w, button->h, button->text,
				   text_color, fill_color, 1U, UI_ALIGN_CENTER);
}
