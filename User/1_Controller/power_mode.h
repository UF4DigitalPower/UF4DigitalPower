#ifndef POWER_MODE_H
#define POWER_MODE_H

#include <stdbool.h>
#include <stdint.h>
#include "control_types.h"

typedef enum { POWER_MODE_SELF_CHECK = 0u, POWER_MODE_IDLE, POWER_MODE_SOFT_START,
               POWER_MODE_RUN, POWER_MODE_STOPPING, POWER_MODE_DIRECTION_SWITCH,
               POWER_MODE_FAULT } power_mode_state_t;
typedef enum { POWER_FAULT_NONE = 0u, POWER_FAULT_SELF_CHECK, POWER_FAULT_OVERVOLTAGE,
               POWER_FAULT_OVERCURRENT, POWER_FAULT_ADC, POWER_FAULT_PWM,
               POWER_FAULT_COMMUNICATION, POWER_FAULT_EXTERNAL } power_fault_reason_t;
typedef void (*power_mode_action_fn_t)(void *user);
typedef bool (*power_mode_ready_fn_t)(void *user);
typedef struct { uint32_t self_check_ms, soft_start_ms, stopping_ms, direction_deadtime_ms;
                 power_mode_action_fn_t pwm_enable, pwm_disable;
                 power_mode_ready_fn_t self_check; void *user; } power_mode_config_t;
typedef struct { power_mode_state_t state; power_fault_reason_t fault;
                 control_direction_t requested_direction, active_direction;
                 uint32_t state_tick, fault_count; bool requested_enable, pwm_enabled,
                 self_check_passed; } power_mode_t;
void PowerMode_Init(power_mode_t *, const power_mode_config_t *, uint32_t now_ms);
void PowerMode_Process(power_mode_t *, uint32_t now_ms);
void PowerMode_RequestEnable(power_mode_t *, bool enable);
void PowerMode_RequestDirection(power_mode_t *, control_direction_t direction);
void PowerMode_ReportFault(power_mode_t *, power_fault_reason_t reason);
void PowerMode_ClearFault(power_mode_t *);
power_mode_state_t PowerMode_GetState(const power_mode_t *);
power_fault_reason_t PowerMode_GetFault(const power_mode_t *);
control_direction_t PowerMode_GetDirection(const power_mode_t *);
bool PowerMode_IsPwmEnabled(const power_mode_t *);
uint16_t PowerMode_GetFlagBits(const power_mode_t *);
#endif
