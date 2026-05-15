#include "hanoi_app.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_DISKS 10
#define MIN_ANIM_SPEED_MS 10
#define MAX_ANIM_SPEED_MS 2000
#define PEG_Y_OFS 100
#define PEG_WIDTH 10

typedef struct {
    int from;
    int to;
} move_t;

static move_t *moves_queue = NULL;
static int total_moves = 0;
static int current_move = 0;

static lv_obj_t * disks_obj[MAX_DISKS];
static int disk_pos[3][MAX_DISKS]; // 每个销钉上叠放的盘片
static int peg_counts[3];          // 每个钉子上的圆盘数

static int num_disks = 5;
static int anim_speed_ms = 500;
static bool is_running = false;

static lv_timer_t * play_timer = NULL;

static lv_obj_t * label_status;
static lv_obj_t * pegs_obj[3];
static lv_obj_t * ui_slider_disks;
static lv_obj_t * ui_slider_speed;
static lv_obj_t * ui_btn_start;
static lv_obj_t * ui_btn_reset;
static lv_obj_t * ui_label_disks;
static lv_obj_t * ui_label_speed;

static void hanoi_update_run_state(bool running);
static int hanoi_clamp_disk_count(int count);
static int hanoi_clamp_interval_ms(int interval_ms);

static void calculate_moves_recursive(int n, int from, int to, int aux) {
    if (n == 1) {
        moves_queue[total_moves].from = from;
        moves_queue[total_moves].to = to;
        total_moves++;
        return;
    }
    calculate_moves_recursive(n - 1, from, aux, to);
    moves_queue[total_moves].from = from;
    moves_queue[total_moves].to = to;
    total_moves++;
    calculate_moves_recursive(n - 1, aux, to, from);
}

static void update_status_label(void) {
    lv_label_set_text_fmt(label_status, "Step: %d / %d", current_move, total_moves);
}

static void hanoi_update_run_state(bool running) {
    is_running = running;

    if (play_timer != NULL) {
        if (running) {
            lv_timer_resume(play_timer);
        } else {
            lv_timer_pause(play_timer);
        }
    }

    if (ui_btn_start != NULL) {
        const char *text = "Start";

        if (current_move >= total_moves) {
            text = "Finished";
        } else if (running) {
            text = "Pause";
        } else if (current_move > 0) {
            text = "Resume";
        }

        lv_label_set_text(lv_obj_get_child(ui_btn_start, 0), text);
    }
}

static int hanoi_clamp_disk_count(int count) {
    if (count < 1) return 1;
    if (count > MAX_DISKS) return MAX_DISKS;
    return count;
}

static int hanoi_clamp_interval_ms(int interval_ms) {
    if (interval_ms < MIN_ANIM_SPEED_MS) return MIN_ANIM_SPEED_MS;
    if (interval_ms > MAX_ANIM_SPEED_MS) return MAX_ANIM_SPEED_MS;
    return interval_ms;
}

static void reset_game(void) {
    if (moves_queue != NULL) {
        free(moves_queue);
        moves_queue = NULL;
    }

    // Calculate total steps = 2^n - 1
    int max_queue = (1 << num_disks) - 1;
    moves_queue = (move_t*)malloc(max_queue * sizeof(move_t));
    total_moves = 0;
    calculate_moves_recursive(num_disks, 0, 2, 1);

    current_move = 0;

    peg_counts[0] = num_disks;
    peg_counts[1] = 0;
    peg_counts[2] = 0;

    for (int i = 0; i < num_disks; i++) {
        disk_pos[0][i] = i; // Store disk size index
    }

    hanoi_update_run_state(false);

    // Draw disks
    lv_obj_t * scr = lv_scr_act();
    lv_coord_t area_w = lv_obj_get_width(scr);
    lv_coord_t area_h = lv_obj_get_height(scr);

    // Remove old disks
    for(int i=0; i<MAX_DISKS; i++) {
        if (disks_obj[i] != NULL) {
            lv_obj_del(disks_obj[i]);
            disks_obj[i] = NULL;
        }
    }

    // Recreate disks
    lv_coord_t max_disk_w = area_w / 3 - 20;
    lv_coord_t min_disk_w = 20;
    lv_coord_t disk_h = (area_h - PEG_Y_OFS) / num_disks;
    if (disk_h > 30) disk_h = 30; // Max height

    for (int i = 0; i < num_disks; i++) {
        disks_obj[i] = lv_obj_create(scr);
        lv_obj_remove_style_all(disks_obj[i]);
        lv_obj_set_style_bg_color(disks_obj[i], lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(disks_obj[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(disks_obj[i], disk_h/2, LV_PART_MAIN);

        lv_coord_t w = max_disk_w - ((max_disk_w - min_disk_w) * i / (num_disks > 1 ? num_disks - 1 : 1));
        lv_obj_set_size(disks_obj[i], w, disk_h - 2);

        // Initial position on Peg 0
        lv_coord_t peg_x = (area_w / 6) * 1 - w/2;
        // Fix inverted pyramid bug
        lv_coord_t d_y = area_h - (i + 1) * disk_h;
        lv_obj_set_pos(disks_obj[i], peg_x, d_y);
    }
    update_status_label();
}

static lv_coord_t g_anim_start_x, g_anim_start_y;
static lv_coord_t g_anim_new_x, g_anim_new_y;
static lv_coord_t g_anim_up_y;

static void _anim_disk_cb(void * var, int32_t v) {
    lv_obj_t * obj = (lv_obj_t *)var;
    if (v <= 10000) {
        lv_coord_t curr_y = g_anim_start_y + (g_anim_up_y - g_anim_start_y) * v / 10000;
        lv_obj_set_pos(obj, g_anim_start_x, curr_y);
    } else if (v <= 20000) {
        lv_coord_t curr_x = g_anim_start_x + (g_anim_new_x - g_anim_start_x) * (v - 10000) / 10000;
        lv_obj_set_pos(obj, curr_x, g_anim_up_y);
    } else {
        lv_coord_t curr_y = g_anim_up_y + (g_anim_new_y - g_anim_up_y) * (v - 20000) / 10000;
        lv_obj_set_pos(obj, g_anim_new_x, curr_y);
    }
}

static void anim_timer_cb(lv_timer_t * timer) {
    if (!is_running) return;

    if (current_move >= total_moves) {
        hanoi_update_run_state(false);
        lv_label_set_text(lv_obj_get_child(ui_btn_start, 0), "Finished");
        return;
    }

    move_t m = moves_queue[current_move];

    int from_peg = m.from;
    int to_peg = m.to;

    if (peg_counts[from_peg] == 0) return; // Should not happen

    int disk_idx = disk_pos[from_peg][peg_counts[from_peg] - 1]; // Top disk on source
    peg_counts[from_peg]--;

    disk_pos[to_peg][peg_counts[to_peg]] = disk_idx;
    peg_counts[to_peg]++;

    // Bring currently moving disk to the top Z-order
    lv_obj_move_foreground(disks_obj[disk_idx]);

    // Move UI object
    lv_obj_t * scr = lv_scr_act();
    lv_coord_t area_w = lv_obj_get_width(scr);
    lv_coord_t area_h = lv_obj_get_height(scr);

    lv_coord_t disk_h = (area_h - PEG_Y_OFS) / num_disks;
    if (disk_h > 30) disk_h = 30; // Max height

    lv_coord_t w = lv_obj_get_width(disks_obj[disk_idx]);

    lv_coord_t new_x = (area_w / 6) * (2 * to_peg + 1) - w/2;
    lv_coord_t new_y = area_h - peg_counts[to_peg] * disk_h;

    // Get current position before animating
    g_anim_start_y = lv_obj_get_y(disks_obj[disk_idx]);
    g_anim_start_x = lv_obj_get_x(disks_obj[disk_idx]);
    g_anim_new_x = new_x;
    g_anim_new_y = new_y;

    // Set animation target heights
    g_anim_up_y = PEG_Y_OFS - disk_h - 10;
    if (g_anim_up_y < 0) g_anim_up_y = 0;

    uint32_t a_time = anim_speed_ms > 20 ? (anim_speed_ms - 10) : 10;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, disks_obj[disk_idx]);
    lv_anim_set_time(&a, a_time);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_values(&a, 0, 30000);
    lv_anim_set_exec_cb(&a, _anim_disk_cb);
    lv_anim_start(&a);

    current_move++;
    update_status_label();
}

static void start_btn_event_cb(lv_event_t * e) {
    (void)e;
    if (current_move >= total_moves) {
        return; // Already finished, do nothing. Need reset to start again.
    }

    if (is_running) {
        hanoi_update_run_state(false);
    } else {
        hanoi_update_run_state(true);
    }
}

static void reset_btn_event_cb(lv_event_t * e) {
    (void)e;
    reset_game();
}

static void disk_slider_event_cb(lv_event_t * e) {
    (void)e;
    hanoi_app_set_disk_count((uint16_t)lv_slider_get_value(ui_slider_disks));
}

static void speed_slider_event_cb(lv_event_t * e) {
    (void)e;
    hanoi_app_set_interval_ms((uint16_t)lv_slider_get_value(ui_slider_speed));
}

void hanoi_app_start(void) {
    if (current_move >= total_moves) {
        return;
    }

    hanoi_update_run_state(true);
}

void hanoi_app_pause(void) {
    hanoi_update_run_state(false);
}

void hanoi_app_reset(void) {
    reset_game();
}

void hanoi_app_set_disk_count(uint16_t count) {
    bool was_running = is_running;

    num_disks = hanoi_clamp_disk_count((int)count);

    if (ui_slider_disks != NULL) {
        lv_slider_set_value(ui_slider_disks, num_disks, LV_ANIM_OFF);
    }

    if (ui_label_disks != NULL) {
        lv_label_set_text_fmt(ui_label_disks, "Disks: %d", num_disks);
    }

    reset_game();

    if (was_running) {
        hanoi_app_start();
    }
}

void hanoi_app_set_interval_ms(uint16_t interval_ms) {
    anim_speed_ms = hanoi_clamp_interval_ms((int)interval_ms);

    if (ui_slider_speed != NULL) {
        lv_slider_set_value(ui_slider_speed, anim_speed_ms, LV_ANIM_OFF);
    }

    if (ui_label_speed != NULL) {
        lv_label_set_text_fmt(ui_label_speed, "Speed: %d ms", anim_speed_ms);
    }

    if (play_timer != NULL) {
        lv_timer_set_period(play_timer, anim_speed_ms);
    }
}

uint16_t hanoi_app_get_disk_count(void) {
    return (uint16_t)num_disks;
}

uint16_t hanoi_app_get_interval_ms(void) {
    return (uint16_t)anim_speed_ms;
}

bool hanoi_app_is_running(void) {
    return is_running;
}

uint16_t hanoi_app_get_current_move(void) {
    return (uint16_t)current_move;
}

uint16_t hanoi_app_get_total_moves(void) {
    return (uint16_t)total_moves;
}

void hanoi_app_init(void) {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_clean(scr); // Clean previous screen content

    lv_obj_set_style_bg_color(scr, lv_palette_lighten(LV_PALETTE_GREY, 4), LV_PART_MAIN);

    // Title / Status
    label_status = lv_label_create(scr);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(label_status, "Hanoi Tower");

    // Disk settings directly on screen
    ui_label_disks = lv_label_create(scr);
    lv_label_set_text_fmt(ui_label_disks, "Disks: %d", num_disks);
    lv_obj_align(ui_label_disks, LV_ALIGN_TOP_LEFT, 20, 10);
    ui_slider_disks = lv_slider_create(scr);
    lv_slider_set_range(ui_slider_disks, 1, MAX_DISKS);
    lv_slider_set_value(ui_slider_disks, num_disks, LV_ANIM_OFF);
    lv_obj_set_size(ui_slider_disks, 120, 10);
    lv_obj_align(ui_slider_disks, LV_ALIGN_TOP_LEFT, 20, 40);
    lv_obj_add_event_cb(ui_slider_disks, disk_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Speed settings directly on screen
    ui_label_speed = lv_label_create(scr);
    lv_label_set_text_fmt(ui_label_speed, "Speed: %d ms", anim_speed_ms);
    lv_obj_align(ui_label_speed, LV_ALIGN_TOP_RIGHT, -20, 10);
    ui_slider_speed = lv_slider_create(scr);
    lv_slider_set_range(ui_slider_speed, MIN_ANIM_SPEED_MS, MAX_ANIM_SPEED_MS);
    lv_slider_set_value(ui_slider_speed, anim_speed_ms, LV_ANIM_OFF);
    lv_obj_set_size(ui_slider_speed, 120, 10);
    lv_obj_align(ui_slider_speed, LV_ALIGN_TOP_RIGHT, -20, 40);
    lv_obj_add_event_cb(ui_slider_speed, speed_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Start/Pause Button directly on screen
    ui_btn_start = lv_btn_create(scr);
    lv_obj_t * btn_lbl = lv_label_create(ui_btn_start);
    lv_label_set_text(btn_lbl, "Start");
    lv_obj_align(ui_btn_start, LV_ALIGN_TOP_MID, -50, 40);
    lv_obj_add_event_cb(ui_btn_start, start_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Reset Button
    ui_btn_reset = lv_btn_create(scr);
    lv_obj_t * rst_lbl = lv_label_create(ui_btn_reset);
    lv_label_set_text(rst_lbl, "Reset");
    lv_obj_align(ui_btn_reset, LV_ALIGN_TOP_MID, 50, 40);
    lv_obj_add_event_cb(ui_btn_reset, reset_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Draw 3 Pegs
    for(int i = 0; i < 3; i++) {
        pegs_obj[i] = lv_obj_create(scr);
        lv_obj_remove_style_all(pegs_obj[i]);
        lv_obj_set_style_bg_color(pegs_obj[i], lv_palette_main(LV_PALETTE_BROWN), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(pegs_obj[i], LV_OPA_COVER, LV_PART_MAIN);
    }

    // Timer
    play_timer = lv_timer_create(anim_timer_cb, anim_speed_ms, NULL);
    lv_timer_pause(play_timer);

    // Wait 1 cycle so sizes are calculated
    lv_obj_update_layout(scr);

    // Adjust pegs positions
    lv_coord_t area_w = lv_obj_get_width(scr);
    lv_coord_t area_h = lv_obj_get_height(scr);
    for(int i = 0; i < 3; i++) {
        lv_obj_t * peg = pegs_obj[i];
        lv_obj_set_size(peg, PEG_WIDTH, area_h - PEG_Y_OFS);
        lv_obj_align(peg, LV_ALIGN_DEFAULT, 0, 0);
        lv_obj_set_pos(peg, (area_w / 6) * (2*i + 1) - PEG_WIDTH/2, PEG_Y_OFS);
    }

    for (int i=0; i<MAX_DISKS; i++) disks_obj[i] = NULL;

    reset_game();
}

