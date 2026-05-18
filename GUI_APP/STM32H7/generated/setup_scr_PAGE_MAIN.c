/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_PAGE_MAIN(lv_ui *ui)
{
    //Write codes PAGE_MAIN
    ui->PAGE_MAIN = lv_obj_create(NULL);
    lv_obj_set_size(ui->PAGE_MAIN, 480, 640);
    lv_obj_set_scrollbar_mode(ui->PAGE_MAIN, LV_SCROLLBAR_MODE_OFF);

    //Write style for PAGE_MAIN, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN, lv_color_hex(0xcccccc), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_1
    ui->PAGE_MAIN_label_1 = lv_label_create(ui->PAGE_MAIN);
    lv_label_set_text(ui->PAGE_MAIN_label_1, "VSET:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_1, 8, 490);
    lv_obj_set_size(ui->PAGE_MAIN_label_1, 60, 24);

    //Write style for PAGE_MAIN_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_1, &lv_font_blender_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_3
    ui->PAGE_MAIN_label_3 = lv_label_create(ui->PAGE_MAIN);
    lv_label_set_text(ui->PAGE_MAIN_label_3, "ISET:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_3, 23, 528);
    lv_obj_set_size(ui->PAGE_MAIN_label_3, 60, 24);

    //Write style for PAGE_MAIN_label_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_3, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_3, &lv_font_blender_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_cont_1
    ui->PAGE_MAIN_cont_1 = lv_obj_create(ui->PAGE_MAIN);
    lv_obj_set_pos(ui->PAGE_MAIN_cont_1, 16, 58);
    lv_obj_set_size(ui->PAGE_MAIN_cont_1, 220, 140);
    lv_obj_set_scrollbar_mode(ui->PAGE_MAIN_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for PAGE_MAIN_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_cont_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_7
    ui->PAGE_MAIN_label_7 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_7, "VIN:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_7, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_7, 9, 8);
    lv_obj_set_size(ui->PAGE_MAIN_label_7, 60, 32);

    //Write style for PAGE_MAIN_label_7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_7, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_7, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_8
    ui->PAGE_MAIN_label_8 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_8, "IIN:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_8, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_8, 14, 55);
    lv_obj_set_size(ui->PAGE_MAIN_label_8, 60, 32);

    //Write style for PAGE_MAIN_label_8, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_8, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_8, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_8, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_8, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_8, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_9
    ui->PAGE_MAIN_label_9 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_9, "PIN:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_9, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_9, 10, 98);
    lv_obj_set_size(ui->PAGE_MAIN_label_9, 60, 32);

    //Write style for PAGE_MAIN_label_9, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_9, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_9, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_9, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_9, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_9, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_cont_2
    ui->PAGE_MAIN_cont_2 = lv_obj_create(ui->PAGE_MAIN_cont_1);
    lv_obj_set_pos(ui->PAGE_MAIN_cont_2, 231, -1);
    lv_obj_set_size(ui->PAGE_MAIN_cont_2, 220, 140);
    lv_obj_set_scrollbar_mode(ui->PAGE_MAIN_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for PAGE_MAIN_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_cont_2, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_cont_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_12
    ui->PAGE_MAIN_label_12 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_12, "VOUT:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_12, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_12, 10, 9);
    lv_obj_set_size(ui->PAGE_MAIN_label_12, 80, 32);

    //Write style for PAGE_MAIN_label_12, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_12, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_12, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_12, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_12, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_12, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_11
    ui->PAGE_MAIN_label_11 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_11, "IOUT:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_11, 5, 55);
    lv_obj_set_size(ui->PAGE_MAIN_label_11, 80, 32);

    //Write style for PAGE_MAIN_label_11, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_11, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_11, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_11, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_11, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_10
    ui->PAGE_MAIN_label_10 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_10, "POUT:");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_10, 8, 97);
    lv_obj_set_size(ui->PAGE_MAIN_label_10, 80, 32);

    //Write style for PAGE_MAIN_label_10, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_10, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_10, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_10, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_10, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_10, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_16
    ui->PAGE_MAIN_label_16 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_16, "00.00");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_16, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_16, 93, 7);
    lv_obj_set_size(ui->PAGE_MAIN_label_16, 93, 32);

    //Write style for PAGE_MAIN_label_16, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_16, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_16, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_16, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_16, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_16, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_17
    ui->PAGE_MAIN_label_17 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_17, "00.00");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_17, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_17, 90, 53);
    lv_obj_set_size(ui->PAGE_MAIN_label_17, 93, 32);

    //Write style for PAGE_MAIN_label_17, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_17, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_17, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_17, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_17, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_17, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_18
    ui->PAGE_MAIN_label_18 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_18, "00.00");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_18, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_18, 93, 97);
    lv_obj_set_size(ui->PAGE_MAIN_label_18, 93, 32);

    //Write style for PAGE_MAIN_label_18, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_18, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_18, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_18, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_18, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_18, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_22
    ui->PAGE_MAIN_label_22 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_22, "V");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_22, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_22, 182, 7);
    lv_obj_set_size(ui->PAGE_MAIN_label_22, 30, 32);

    //Write style for PAGE_MAIN_label_22, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_22, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_22, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_22, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_22, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_22, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_23
    ui->PAGE_MAIN_label_23 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_23, "A");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_23, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_23, 182, 55);
    lv_obj_set_size(ui->PAGE_MAIN_label_23, 30, 32);

    //Write style for PAGE_MAIN_label_23, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_23, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_23, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_23, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_23, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_23, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_24
    ui->PAGE_MAIN_label_24 = lv_label_create(ui->PAGE_MAIN_cont_2);
    lv_label_set_text(ui->PAGE_MAIN_label_24, "W");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_24, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_24, 183, 98);
    lv_obj_set_size(ui->PAGE_MAIN_label_24, 30, 32);

    //Write style for PAGE_MAIN_label_24, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_24, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_24, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_24, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_24, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_24, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_13
    ui->PAGE_MAIN_label_13 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_13, "00.00");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_13, 74, 11);
    lv_obj_set_size(ui->PAGE_MAIN_label_13, 93, 32);

    //Write style for PAGE_MAIN_label_13, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_13, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_13, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_13, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_13, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_13, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_14
    ui->PAGE_MAIN_label_14 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_14, "00.00");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_14, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_14, 72, 55);
    lv_obj_set_size(ui->PAGE_MAIN_label_14, 93, 32);

    //Write style for PAGE_MAIN_label_14, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_14, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_14, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_14, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_14, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_14, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_15
    ui->PAGE_MAIN_label_15 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_15, "00.00");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_15, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_15, 71, 96);
    lv_obj_set_size(ui->PAGE_MAIN_label_15, 93, 32);

    //Write style for PAGE_MAIN_label_15, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_15, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_15, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_15, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_15, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_15, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_19
    ui->PAGE_MAIN_label_19 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_19, "V");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_19, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_19, 171, 9);
    lv_obj_set_size(ui->PAGE_MAIN_label_19, 30, 32);

    //Write style for PAGE_MAIN_label_19, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_19, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_19, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_19, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_19, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_19, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_20
    ui->PAGE_MAIN_label_20 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_20, "A");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_20, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_20, 171, 54);
    lv_obj_set_size(ui->PAGE_MAIN_label_20, 30, 32);

    //Write style for PAGE_MAIN_label_20, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_20, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_20, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_20, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_20, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_20, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_21
    ui->PAGE_MAIN_label_21 = lv_label_create(ui->PAGE_MAIN_cont_1);
    lv_label_set_text(ui->PAGE_MAIN_label_21, "W");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_21, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_21, 171, 96);
    lv_obj_set_size(ui->PAGE_MAIN_label_21, 30, 32);

    //Write style for PAGE_MAIN_label_21, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_21, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_21, &lv_font_blender_32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_21, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_21, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_21, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_spinbox_1
    ui->PAGE_MAIN_spinbox_1 = lv_spinbox_create(ui->PAGE_MAIN);
    lv_obj_set_pos(ui->PAGE_MAIN_spinbox_1, 133, 494);
    lv_obj_set_width(ui->PAGE_MAIN_spinbox_1, 70);
    lv_obj_set_height(ui->PAGE_MAIN_spinbox_1, 40);
    lv_spinbox_set_digit_format(ui->PAGE_MAIN_spinbox_1, 5, 3);
    lv_spinbox_set_range(ui->PAGE_MAIN_spinbox_1, -99999, 99999);
    lv_coord_t PAGE_MAIN_spinbox_1_h = lv_obj_get_height(ui->PAGE_MAIN_spinbox_1);
    ui->PAGE_MAIN_spinbox_1_btn_plus = lv_btn_create(ui->PAGE_MAIN);
    lv_obj_set_size(ui->PAGE_MAIN_spinbox_1_btn_plus, PAGE_MAIN_spinbox_1_h, PAGE_MAIN_spinbox_1_h);
    lv_obj_align_to(ui->PAGE_MAIN_spinbox_1_btn_plus, ui->PAGE_MAIN_spinbox_1, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_set_style_bg_img_src(ui->PAGE_MAIN_spinbox_1_btn_plus, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(ui->PAGE_MAIN_spinbox_1_btn_plus, lv_PAGE_MAIN_spinbox_1_increment_event_cb, LV_EVENT_ALL, NULL);
    ui->PAGE_MAIN_spinbox_1_btn_minus = lv_btn_create(ui->PAGE_MAIN);
    lv_obj_set_size(ui->PAGE_MAIN_spinbox_1_btn_minus, PAGE_MAIN_spinbox_1_h, PAGE_MAIN_spinbox_1_h);
    lv_obj_align_to(ui->PAGE_MAIN_spinbox_1_btn_minus, ui->PAGE_MAIN_spinbox_1, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_img_src(ui->PAGE_MAIN_spinbox_1_btn_minus, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(ui->PAGE_MAIN_spinbox_1_btn_minus, lv_PAGE_MAIN_spinbox_1_decrement_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(ui->PAGE_MAIN_spinbox_1, 133, 494);

    //Write style for PAGE_MAIN_spinbox_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_spinbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_spinbox_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_spinbox_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->PAGE_MAIN_spinbox_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->PAGE_MAIN_spinbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->PAGE_MAIN_spinbox_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->PAGE_MAIN_spinbox_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_spinbox_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_spinbox_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_spinbox_1, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_spinbox_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_spinbox_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_spinbox_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_spinbox_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for PAGE_MAIN_spinbox_1, Part: LV_PART_CURSOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->PAGE_MAIN_spinbox_1, lv_color_hex(0xffffff), LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_spinbox_1, &lv_font_montserratMedium_12, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_spinbox_1, 255, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_spinbox_1, 255, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_spinbox_1, lv_color_hex(0x2195f6), LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_spinbox_1, LV_GRAD_DIR_NONE, LV_PART_CURSOR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_PAGE_MAIN_spinbox_1_extra_btns_main_default
    static lv_style_t style_PAGE_MAIN_spinbox_1_extra_btns_main_default;
    ui_init_style(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default);

    lv_style_set_text_color(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, 255);
    lv_style_set_bg_opa(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, 0);
    lv_style_set_radius(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, 5);
    lv_style_set_shadow_width(&style_PAGE_MAIN_spinbox_1_extra_btns_main_default, 0);
    lv_obj_add_style(ui->PAGE_MAIN_spinbox_1_btn_plus, &style_PAGE_MAIN_spinbox_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->PAGE_MAIN_spinbox_1_btn_minus, &style_PAGE_MAIN_spinbox_1_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_spinbox_2
    ui->PAGE_MAIN_spinbox_2 = lv_spinbox_create(ui->PAGE_MAIN);
    lv_obj_set_pos(ui->PAGE_MAIN_spinbox_2, 146, 462);
    lv_obj_set_width(ui->PAGE_MAIN_spinbox_2, 70);
    lv_obj_set_height(ui->PAGE_MAIN_spinbox_2, 40);
    lv_spinbox_set_digit_format(ui->PAGE_MAIN_spinbox_2, 5, 3);
    lv_spinbox_set_range(ui->PAGE_MAIN_spinbox_2, -99999, 99999);
    lv_coord_t PAGE_MAIN_spinbox_2_h = lv_obj_get_height(ui->PAGE_MAIN_spinbox_2);
    ui->PAGE_MAIN_spinbox_2_btn_plus = lv_btn_create(ui->PAGE_MAIN);
    lv_obj_set_size(ui->PAGE_MAIN_spinbox_2_btn_plus, PAGE_MAIN_spinbox_2_h, PAGE_MAIN_spinbox_2_h);
    lv_obj_align_to(ui->PAGE_MAIN_spinbox_2_btn_plus, ui->PAGE_MAIN_spinbox_2, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_set_style_bg_img_src(ui->PAGE_MAIN_spinbox_2_btn_plus, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(ui->PAGE_MAIN_spinbox_2_btn_plus, lv_PAGE_MAIN_spinbox_2_increment_event_cb, LV_EVENT_ALL, NULL);
    ui->PAGE_MAIN_spinbox_2_btn_minus = lv_btn_create(ui->PAGE_MAIN);
    lv_obj_set_size(ui->PAGE_MAIN_spinbox_2_btn_minus, PAGE_MAIN_spinbox_2_h, PAGE_MAIN_spinbox_2_h);
    lv_obj_align_to(ui->PAGE_MAIN_spinbox_2_btn_minus, ui->PAGE_MAIN_spinbox_2, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_img_src(ui->PAGE_MAIN_spinbox_2_btn_minus, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(ui->PAGE_MAIN_spinbox_2_btn_minus, lv_PAGE_MAIN_spinbox_2_decrement_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(ui->PAGE_MAIN_spinbox_2, 146, 462);

    //Write style for PAGE_MAIN_spinbox_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_spinbox_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_spinbox_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_spinbox_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->PAGE_MAIN_spinbox_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->PAGE_MAIN_spinbox_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->PAGE_MAIN_spinbox_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->PAGE_MAIN_spinbox_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_spinbox_2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_spinbox_2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_spinbox_2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_spinbox_2, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_spinbox_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_spinbox_2, &lv_font_montserratMedium_12, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_spinbox_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_spinbox_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_spinbox_2, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_spinbox_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for PAGE_MAIN_spinbox_2, Part: LV_PART_CURSOR, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->PAGE_MAIN_spinbox_2, lv_color_hex(0xffffff), LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_spinbox_2, &lv_font_montserratMedium_12, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_spinbox_2, 255, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_spinbox_2, 255, LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_spinbox_2, lv_color_hex(0x2195f6), LV_PART_CURSOR|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_spinbox_2, LV_GRAD_DIR_NONE, LV_PART_CURSOR|LV_STATE_DEFAULT);

    //Write style state: LV_STATE_DEFAULT for &style_PAGE_MAIN_spinbox_2_extra_btns_main_default
    static lv_style_t style_PAGE_MAIN_spinbox_2_extra_btns_main_default;
    ui_init_style(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default);

    lv_style_set_text_color(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, &lv_font_montserratMedium_12);
    lv_style_set_text_opa(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, 255);
    lv_style_set_bg_opa(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, 255);
    lv_style_set_bg_color(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, lv_color_hex(0x2195f6));
    lv_style_set_bg_grad_dir(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, 0);
    lv_style_set_radius(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, 5);
    lv_style_set_shadow_width(&style_PAGE_MAIN_spinbox_2_extra_btns_main_default, 0);
    lv_obj_add_style(ui->PAGE_MAIN_spinbox_2_btn_plus, &style_PAGE_MAIN_spinbox_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_add_style(ui->PAGE_MAIN_spinbox_2_btn_minus, &style_PAGE_MAIN_spinbox_2_extra_btns_main_default, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_sw_1
    ui->PAGE_MAIN_sw_1 = lv_switch_create(ui->PAGE_MAIN);
    lv_obj_set_pos(ui->PAGE_MAIN_sw_1, 289, 499);
    lv_obj_set_size(ui->PAGE_MAIN_sw_1, 40, 20);

    //Write style for PAGE_MAIN_sw_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_sw_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_sw_1, lv_color_hex(0xe6e2e6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_sw_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->PAGE_MAIN_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_sw_1, 10, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_sw_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for PAGE_MAIN_sw_1, Part: LV_PART_INDICATOR, State: LV_STATE_CHECKED.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_sw_1, 255, LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_sw_1, lv_color_hex(0x2195f6), LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_sw_1, LV_GRAD_DIR_NONE, LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_border_width(ui->PAGE_MAIN_sw_1, 0, LV_PART_INDICATOR|LV_STATE_CHECKED);

    //Write style for PAGE_MAIN_sw_1, Part: LV_PART_KNOB, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_sw_1, 255, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_sw_1, lv_color_hex(0xffffff), LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_sw_1, LV_GRAD_DIR_NONE, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->PAGE_MAIN_sw_1, 0, LV_PART_KNOB|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_sw_1, 10, LV_PART_KNOB|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_cont_3
    ui->PAGE_MAIN_cont_3 = lv_obj_create(ui->PAGE_MAIN);
    lv_obj_set_pos(ui->PAGE_MAIN_cont_3, 16, 15);
    lv_obj_set_size(ui->PAGE_MAIN_cont_3, 450, 32);
    lv_obj_set_scrollbar_mode(ui->PAGE_MAIN_cont_3, LV_SCROLLBAR_MODE_OFF);

    //Write style for PAGE_MAIN_cont_3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_cont_3, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_cont_3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_cont_3, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_cont_3, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_cont_3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_25
    ui->PAGE_MAIN_label_25 = lv_label_create(ui->PAGE_MAIN_cont_3);
    lv_label_set_text(ui->PAGE_MAIN_label_25, "UF4DigitalPower");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_25, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_25, 4, 3);
    lv_obj_set_size(ui->PAGE_MAIN_label_25, 180, 24);

    //Write style for PAGE_MAIN_label_25, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_25, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_25, &lv_font_blender_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_25, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_25, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_25, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_label_26
    ui->PAGE_MAIN_label_26 = lv_label_create(ui->PAGE_MAIN_cont_3);
    lv_label_set_text(ui->PAGE_MAIN_label_26, "V1.0.0.112");
    lv_label_set_long_mode(ui->PAGE_MAIN_label_26, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->PAGE_MAIN_label_26, 231, 4);
    lv_obj_set_size(ui->PAGE_MAIN_label_26, 180, 24);

    //Write style for PAGE_MAIN_label_26, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->PAGE_MAIN_label_26, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_label_26, &lv_font_blender_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_label_26, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->PAGE_MAIN_label_26, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_label_26, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes PAGE_MAIN_chart_1
    ui->PAGE_MAIN_chart_1 = lv_chart_create(ui->PAGE_MAIN);
    lv_chart_set_type(ui->PAGE_MAIN_chart_1, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(ui->PAGE_MAIN_chart_1, 3, 5);
    lv_chart_set_point_count(ui->PAGE_MAIN_chart_1, 5);
    lv_chart_set_range(ui->PAGE_MAIN_chart_1, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_range(ui->PAGE_MAIN_chart_1, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    lv_chart_set_zoom_x(ui->PAGE_MAIN_chart_1, 256);
    lv_chart_set_zoom_y(ui->PAGE_MAIN_chart_1, 256);
    ui->PAGE_MAIN_chart_1_0 = lv_chart_add_series(ui->PAGE_MAIN_chart_1, lv_color_hex(0x000000), LV_CHART_AXIS_PRIMARY_Y);
#if LV_USE_FREEMASTER == 0
    lv_chart_set_next_value(ui->PAGE_MAIN_chart_1, ui->PAGE_MAIN_chart_1_0, 1);
    lv_chart_set_next_value(ui->PAGE_MAIN_chart_1, ui->PAGE_MAIN_chart_1_0, 20);
    lv_chart_set_next_value(ui->PAGE_MAIN_chart_1, ui->PAGE_MAIN_chart_1_0, 30);
    lv_chart_set_next_value(ui->PAGE_MAIN_chart_1, ui->PAGE_MAIN_chart_1_0, 40);
    lv_chart_set_next_value(ui->PAGE_MAIN_chart_1, ui->PAGE_MAIN_chart_1_0, 5);
#endif
    lv_obj_set_pos(ui->PAGE_MAIN_chart_1, 20, 214);
    lv_obj_set_size(ui->PAGE_MAIN_chart_1, 447, 160);
    lv_obj_set_scrollbar_mode(ui->PAGE_MAIN_chart_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for PAGE_MAIN_chart_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->PAGE_MAIN_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->PAGE_MAIN_chart_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->PAGE_MAIN_chart_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->PAGE_MAIN_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->PAGE_MAIN_chart_1, 6, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->PAGE_MAIN_chart_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->PAGE_MAIN_chart_1, lv_color_hex(0xe8e8e8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->PAGE_MAIN_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->PAGE_MAIN_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for PAGE_MAIN_chart_1, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->PAGE_MAIN_chart_1, lv_color_hex(0x151212), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->PAGE_MAIN_chart_1, &lv_font_montserratMedium_12, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->PAGE_MAIN_chart_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->PAGE_MAIN_chart_1, 2, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->PAGE_MAIN_chart_1, lv_color_hex(0xe8e8e8), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->PAGE_MAIN_chart_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    //The custom code of PAGE_MAIN.


    //Update current screen layout.
    lv_obj_update_layout(ui->PAGE_MAIN);

}
