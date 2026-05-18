/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct
{
  
	lv_obj_t *PAGE_MAIN;
	bool PAGE_MAIN_del;
	lv_obj_t *PAGE_MAIN_label_1;
	lv_obj_t *PAGE_MAIN_label_3;
	lv_obj_t *PAGE_MAIN_cont_1;
	lv_obj_t *PAGE_MAIN_label_7;
	lv_obj_t *PAGE_MAIN_label_8;
	lv_obj_t *PAGE_MAIN_label_9;
	lv_obj_t *PAGE_MAIN_cont_2;
	lv_obj_t *PAGE_MAIN_label_12;
	lv_obj_t *PAGE_MAIN_label_11;
	lv_obj_t *PAGE_MAIN_label_10;
	lv_obj_t *PAGE_MAIN_label_16;
	lv_obj_t *PAGE_MAIN_label_17;
	lv_obj_t *PAGE_MAIN_label_18;
	lv_obj_t *PAGE_MAIN_label_22;
	lv_obj_t *PAGE_MAIN_label_23;
	lv_obj_t *PAGE_MAIN_label_24;
	lv_obj_t *PAGE_MAIN_label_13;
	lv_obj_t *PAGE_MAIN_label_14;
	lv_obj_t *PAGE_MAIN_label_15;
	lv_obj_t *PAGE_MAIN_label_19;
	lv_obj_t *PAGE_MAIN_label_20;
	lv_obj_t *PAGE_MAIN_label_21;
	lv_obj_t *PAGE_MAIN_spinbox_1;
	lv_obj_t *PAGE_MAIN_spinbox_1_btn_plus;
	lv_obj_t *PAGE_MAIN_spinbox_1_btn_minus;
	lv_obj_t *PAGE_MAIN_spinbox_2;
	lv_obj_t *PAGE_MAIN_spinbox_2_btn_plus;
	lv_obj_t *PAGE_MAIN_spinbox_2_btn_minus;
	lv_obj_t *PAGE_MAIN_sw_1;
	lv_obj_t *PAGE_MAIN_cont_3;
	lv_obj_t *PAGE_MAIN_label_25;
	lv_obj_t *PAGE_MAIN_label_26;
	lv_obj_t *PAGE_MAIN_chart_1;
	lv_chart_series_t *PAGE_MAIN_chart_1_0;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_scr_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, int32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                       uint16_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                       lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_ready_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_ui(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_PAGE_MAIN(lv_ui *ui);

LV_FONT_DECLARE(lv_font_blender_24)
LV_FONT_DECLARE(lv_font_blender_32)
LV_FONT_DECLARE(lv_font_montserratMedium_12)


#ifdef __cplusplus
}
#endif
#endif
