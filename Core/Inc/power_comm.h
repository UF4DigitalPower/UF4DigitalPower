#ifndef POWER_COMM_H
#define POWER_COMM_H

#include <stdbool.h>

#include "stm32h7xx_hal.h"
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

void PowerComm_Init(UART_HandleTypeDef *uart);
bool PowerComm_Tick(ui_power_snapshot_t *snapshot);
void PowerComm_WriteSettings(int32_t vset_mv, int32_t iset_ma, bool output_enabled);

#ifdef __cplusplus
}
#endif

#endif /* POWER_COMM_H */
