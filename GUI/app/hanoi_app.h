#ifndef HANOI_APP_H
#define HANOI_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

void hanoi_app_init(void);
void hanoi_app_start(void);
void hanoi_app_pause(void);
void hanoi_app_reset(void);
void hanoi_app_set_disk_count(uint16_t count);
void hanoi_app_set_interval_ms(uint16_t interval_ms);
uint16_t hanoi_app_get_disk_count(void);
uint16_t hanoi_app_get_interval_ms(void);
bool hanoi_app_is_running(void);
uint16_t hanoi_app_get_current_move(void);
uint16_t hanoi_app_get_total_moves(void);

#ifdef __cplusplus
}
#endif

#endif // HANOI_APP_H

