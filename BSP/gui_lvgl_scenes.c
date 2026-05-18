#include "gui_lvgl_scenes.h"

#include "lvgl.h"
#include <stdio.h>

#define SCENE_PERIOD_MS 900U
#define SCENE_ANIM_MS   180U

static lv_timer_t *g_scene_timer;
static uint8_t g_scene_index;

static void add_title(lv_obj_t *scr, const char *title, const char *subtitle)
{
	lv_obj_t *label = lv_label_create(scr);
	lv_label_set_text(label, title);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 28);

	label = lv_label_create(scr);
	lv_label_set_text(label, subtitle);
	lv_obj_set_style_text_color(label, lv_color_hex(0xD8E2EA), 0);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 72);
}

static lv_obj_t *new_screen(uint32_t bg_color)
{
	lv_obj_t *scr = lv_obj_create(NULL);

	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_bg_color(scr, lv_color_hex(bg_color), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	return scr;
}

static void add_tile(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
					 uint32_t color, const char *text)
{
	lv_obj_t *tile = lv_obj_create(parent);
	lv_obj_t *label;

	lv_obj_set_pos(tile, x, y);
	lv_obj_set_size(tile, w, h);
	lv_obj_set_style_radius(tile, 8, 0);
	lv_obj_set_style_border_width(tile, 0, 0);
	lv_obj_set_style_bg_color(tile, lv_color_hex(color), 0);
	lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);

	label = lv_label_create(tile);
	lv_label_set_text(label, text);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
	lv_obj_center(label);
}

static lv_obj_t *create_dashboard(uint8_t seq)
{
	lv_obj_t *scr = new_screen(0x101820);
	uint8_t i;

	add_title(scr, "DASHBOARD", "full-screen slide transition");
	for (i = 0U; i < 6U; ++i) {
		char text[16];
		snprintf(text, sizeof(text), "%u", (unsigned int) (seq * 6U + i));
		add_tile(scr, (lv_coord_t) (30 + (i % 2U) * 218), (lv_coord_t) (132 + (i / 2U) * 126),
				 178, 92,
				 i % 3U == 0U ? 0x2D9CDB : (i % 3U == 1U ? 0x27AE60 : 0xF2994A),
				 text);
	}
	return scr;
}

static lv_obj_t *create_stripes(uint8_t seq)
{
	static const uint32_t colors[] = {0xEB5757, 0xF2994A, 0xF2C94C, 0x27AE60, 0x2D9CDB, 0x9B51E0};
	lv_obj_t *scr = new_screen(0x161616);
	uint8_t i;

	add_title(scr, "STRIPES", "large filled areas");
	for (i = 0U; i < 6U; ++i) {
		lv_obj_t *stripe = lv_obj_create(scr);
		lv_obj_set_pos(stripe, 0, (lv_coord_t) (118 + i * 70));
		lv_obj_set_size(stripe, 480, 52);
		lv_obj_set_style_radius(stripe, 0, 0);
		lv_obj_set_style_border_width(stripe, 0, 0);
		lv_obj_set_style_bg_color(stripe, lv_color_hex(colors[(i + seq) % 6U]), 0);
		lv_obj_set_style_bg_opa(stripe, LV_OPA_COVER, 0);
	}
	return scr;
}

static lv_obj_t *create_cards(uint8_t seq)
{
	lv_obj_t *scr = new_screen(0x211A2C);
	uint8_t i;

	add_title(scr, "CARDS", "text and rounded panels");
	for (i = 0U; i < 8U; ++i) {
		char text[32];
		snprintf(text, sizeof(text), "Scene %u.%u", (unsigned int) seq, (unsigned int) i);
		add_tile(scr, (lv_coord_t) (22 + (i % 2U) * 232), (lv_coord_t) (118 + (i / 2U) * 95),
				 204, 68,
				 i % 2U == 0U ? 0x6C5CE7 : 0xE84393,
				 text);
	}
	return scr;
}

static lv_obj_t *create_meter_static(uint8_t seq)
{
	lv_obj_t *scr = new_screen(0x0E1E25);
	uint8_t i;

	add_title(scr, "METERS", "static controls, screen transition only");
	for (i = 0U; i < 4U; ++i) {
		lv_obj_t *arc = lv_arc_create(scr);
		lv_obj_set_size(arc, 130, 130);
		lv_obj_set_pos(arc, (lv_coord_t) (42 + (i % 2U) * 235), (lv_coord_t) (134 + (i / 2U) * 190));
		lv_arc_set_range(arc, 0, 100);
		lv_arc_set_value(arc, (int16_t) (22 + ((seq * 17U + i * 19U) % 76U)));
		lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
		lv_obj_set_style_arc_width(arc, 14, LV_PART_MAIN);
		lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
		lv_obj_set_style_arc_color(arc, lv_color_hex(0x25414B), LV_PART_MAIN);
		lv_obj_set_style_arc_color(arc, lv_color_hex(i % 2U == 0U ? 0x56CCF2 : 0xF2C94C), LV_PART_INDICATOR);
	}
	return scr;
}

static lv_obj_t *build_scene(uint8_t index)
{
	switch (index % 4U) {
		case 0:
			return create_dashboard(index);
		case 1:
			return create_stripes(index);
		case 2:
			return create_cards(index);
		default:
			return create_meter_static(index);
	}
}

static lv_scr_load_anim_t scene_anim_type(uint8_t index)
{
	switch (index % 5U) {
		case 0:
			return LV_SCR_LOAD_ANIM_MOVE_LEFT;
		case 1:
			return LV_SCR_LOAD_ANIM_MOVE_TOP;
		case 2:
			return LV_SCR_LOAD_ANIM_OVER_RIGHT;
		case 3:
			return LV_SCR_LOAD_ANIM_OVER_BOTTOM;
		default:
			return LV_SCR_LOAD_ANIM_FADE_IN;
	}
}

static void load_next_scene(bool animated)
{
	lv_obj_t *scr = build_scene(g_scene_index);

	if (animated) {
		lv_scr_load_anim(scr, scene_anim_type(g_scene_index), SCENE_ANIM_MS, 0, true);
	} else {
		lv_scr_load(scr);
	}
}

static void scene_timer_cb(lv_timer_t *timer)
{
	(void) timer;
	g_scene_index++;
	load_next_scene(true);
}

void GUI_LVGL_TestScenesStart(void)
{
	g_scene_index = 0U;
	load_next_scene(false);

	if (g_scene_timer == NULL) {
		g_scene_timer = lv_timer_create(scene_timer_cb, SCENE_PERIOD_MS, NULL);
	} else {
		lv_timer_set_period(g_scene_timer, SCENE_PERIOD_MS);
		lv_timer_reset(g_scene_timer);
		lv_timer_resume(g_scene_timer);
	}
}
