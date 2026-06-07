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
} POWER_ctrlStage_t;

typedef enum
{
    POWER_CTRL_CVCC_CC = 0U,
    POWER_CTRL_CVCC_CV = 1U,
} POWER_ctrlCvccMode_t;

enum
{
    POWER_CTRL_FAULT_NONE = 0x0000U,
    POWER_CTRL_FAULT_VIN_UVP = 0x0001U,
    POWER_CTRL_FAULT_VIN_OVP = 0x0002U,
    POWER_CTRL_FAULT_VOUT_UVP = 0x0004U,
    POWER_CTRL_FAULT_VOUT_OVP = 0x0008U,
    POWER_CTRL_FAULT_IOUT_OCP = 0x0010U,
    POWER_CTRL_FAULT_SHORT = 0x0020U,
    POWER_CTRL_FAULT_OTP = 0x0040U,
};

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
} POWER_ctrlSettings_t;

typedef struct
{
    uint32_t input_voltage_mv;
    uint32_t input_current_ma;
    uint32_t output_voltage_mv;
    uint32_t output_current_ma;
    int32_t core_temperature_mc;
    int32_t board_temperature_mc;
    int32_t temp2_temperature_mc;

    uint16_t input_voltage_raw;
    uint16_t input_current_raw;
    uint16_t output_voltage_raw;
    uint16_t output_current_raw;

    POWER_ctrlSettings_t settings;
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
} POWER_ctrlSnapshot_t;

void POWER_initAppCtrl(void);
void POWER_getAppSnapshot(POWER_ctrlSnapshot_t *snapshot);
void POWER_applyAppSettings(const POWER_ctrlSettings_t *settings);
void POWER_setAppEnabled(uint8_t enabled);
void POWER_runAppControlTick(void);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_POWER_CTRL_H */
