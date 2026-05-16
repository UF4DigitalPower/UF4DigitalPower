/**
 * @file    : ui_font.c
 * @brief   : UI 位图字体选择与字形查询
 */

#include "ui_font.h"

#include <stddef.h>


const ui_bitmap_font_t *UI_FontGetForScale(const uint8_t scale) {
	switch (scale) {
		case 1U:
			return &g_ui_font_small;
		case 2U:
			return &g_ui_font_title;
		case 3U:
			return &g_ui_font_value;
		default:
			return &g_ui_font_small;
	}
}

bool UI_FontGetGlyph(const ui_bitmap_font_t *font, const char ch, const ui_bitmap_glyph_t **glyph) {
	uint8_t index;

	if (font == NULL || glyph == NULL) {
		return false;
	}
	if ((uint8_t) ch < font->first_char || (uint8_t) ch > font->last_char) {
		return false;
	}

	index = (uint8_t) ((uint8_t) ch - font->first_char);
	if (font->glyphs[index].advance == 0U) {
		return false;
	}

	*glyph = &font->glyphs[index];
	return true;
}

uint8_t UI_FontGetHeight(const ui_bitmap_font_t *font) {
	return (font != NULL) ? font->height : 0U;
}

