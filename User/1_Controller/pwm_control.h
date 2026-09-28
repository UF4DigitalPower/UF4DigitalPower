#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H
#include <stdbool.h>
#include "stm32g4xx_hal.h"
#include "control_types.h"
void PwmControl_Init(HRTIM_HandleTypeDef *hrtim);
void PwmControl_SetMode(control_direction_t direction, control_region_t region);
HAL_StatusTypeDef PwmControl_Enable(void);
HAL_StatusTypeDef PwmControl_Disable(void);
bool PwmControl_IsEnabled(void);
HAL_StatusTypeDef PwmControl_SetDutyQ15(uint16_t duty_q15);
#endif
