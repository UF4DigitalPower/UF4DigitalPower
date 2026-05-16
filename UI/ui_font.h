/**
 * @file    : ui_font.h
 * @brief   : UI 位图字体资源与查询接口
 */

#ifndef STM32H743_UI_FONT_H
#define STM32H743_UI_FONT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t bitmap_offset;
	uint8_t width;
	uint8_t advance;
} ui_bitmap_glyph_t;

typedef struct {
	uint8_t first_char;
	uint8_t last_char;
	uint8_t height;
	const ui_bitmap_glyph_t *glyphs;
	const uint8_t *bitmap;
} ui_bitmap_font_t;

extern const ui_bitmap_font_t g_ui_font_small;
extern const ui_bitmap_font_t g_ui_font_title;
extern const ui_bitmap_font_t g_ui_font_value;

const ui_bitmap_font_t *UI_FontGetForScale(uint8_t scale);
bool UI_FontGetGlyph(const ui_bitmap_font_t *font, char ch, const ui_bitmap_glyph_t **glyph);
uint8_t UI_FontGetHeight(const ui_bitmap_font_t *font);

#ifdef __cplusplus
}
#endif

#endif /* STM32H743_UI_FONT_H */

