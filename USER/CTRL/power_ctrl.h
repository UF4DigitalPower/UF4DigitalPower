/**
 * @file power_ctrl.h
 * @brief Minimal power-control state interface for app/protocol layers.
 */

#ifndef UF4DIGITALPOWER_POWER_CTRL_H
#define UF4DIGITALPOWER_POWER_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "bsp_power.h"

typedef enum
{
    POWER_CTRL_STAGE_NA = 0U,
    POWER_CTRL_STAGE_BUCK = 1U,
    POWER_CTRL_STAGE_BOOST = 2U,
    POWER_CTRL_STAGE_MIX = 3U,
} power_ctrl_stage_t;

typedef enum
{
    POWER_CTRL_CVCC_CC = 0U,
    POWER_CTRL_CVCC_CV = 1U,
} power_ctrl_cvcc_mode_t;

enum
{
    POWER_CTRL_STATE_FLAG_INIT = 0x01U,
    POWER_CTRL_STATE_FLAG_WAIT = 0x02U,
    POWER_CTRL_STATE_FLAG_RISE = 0x04U,
    POWER_CTRL_STATE_FLAG_RUN = 0x08U,
    POWER_CTRL_STATE_FLAG_ERR = 0x0FU,
};

typedef struct
{
    uint32_t set_voltage_mv;
    uint32_t set_current_ma;
    int32_t otp_set_mc;
    uint32_t ovp_set_mv;
    uint32_t ocp_set_ma;
    uint32_t fan_set_permille;
} power_ctrl_settings_t;

typedef struct
{
    uint32_t input_voltage_mv;
    uint32_t input_current_ma;
    uint32_t output_voltage_mv;
    uint32_t output_current_ma;
    int32_t core_temperature_mc;
    int32_t board_temperature_mc;

    uint16_t input_voltage_raw;
    uint16_t input_current_raw;
    uint16_t output_voltage_raw;
    uint16_t output_current_raw;

    power_ctrl_settings_t settings;
    uint8_t power_enabled;
    uint32_t fault_flags;
    uint8_t state_flag_bits;
    uint8_t stage_mode;
    uint8_t cvcc_mode;

    int32_t otp_value_mc;
    uint32_t ovp_value_mv;
    uint32_t ocp_value_ma;

    uint32_t duty_cmd;
    uint32_t pwm_a_compare;
    uint32_t pwm_d_compare;
    uint32_t fan_speed_permille;
    uint32_t loop_current_feedback_ma;
    uint32_t loop_current_reference_ma;
    uint32_t voltage_loop_reference_mv;
} power_ctrl_snapshot_t;

void PowerCtrl_Init(void);
void PowerCtrl_GetSnapshot(power_ctrl_snapshot_t *snapshot);
void PowerCtrl_ApplySettings(const power_ctrl_settings_t *settings);
void PowerCtrl_SetEnabled(uint8_t enabled);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_POWER_CTRL_H */
