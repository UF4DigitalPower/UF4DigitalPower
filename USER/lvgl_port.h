#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void LVGL_Port_Init(void);
void LVGL_Port_RunBenchmark(void);
uint32_t LVGL_Port_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_PORT_H */

