/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef WIDGET_INIT_H
#define WIDGET_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "gui_guider.h"

__attribute__((unused)) void kb_event_cb(lv_event_t *e);
__attribute__((unused)) void ta_event_cb(lv_event_t *e);
#if LV_USE_ANALOGCLOCK != 0
void clock_count(int *hour, int *min, int *sec);
#endif

extern lv_obj_t * PAGE_MAIN_spinbox_1;
void lv_PAGE_MAIN_spinbox_1_increment_event_cb(lv_event_t * event);
void lv_PAGE_MAIN_spinbox_1_decrement_event_cb(lv_event_t * event);
extern lv_obj_t * PAGE_MAIN_spinbox_2;
void lv_PAGE_MAIN_spinbox_2_increment_event_cb(lv_event_t * event);
void lv_PAGE_MAIN_spinbox_2_decrement_event_cb(lv_event_t * event);


#ifdef __cplusplus
}
#endif
#endif
