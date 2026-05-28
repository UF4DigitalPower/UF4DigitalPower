/**
 * @file power_ctrl.c
 * @brief Minimal power-control state backing for protocol integration.
 */

#include "power_ctrl.h"

#include <string.h>

static POWER_ctrlSettings_t s_POWER_ctrlSettings = {
    .set_voltage_mv = 5000U,
    .set_current_ma = 10000U,
    .otp_set_mc = 80000,
    .ovp_set_mv = 50000U,
    .ocp_set_ma = 10500U,
    .fan_set_permille = 0U,
};

static uint8_t s_POWER_ctrlEnabled = 0U;
static uint32_t s_POWER_ctrlFaultFlags = 0U;
static uint8_t s_POWER_ctrlStateFlagBits = POWER_CTRL_STATE_FLAG_WAIT;
static uint8_t s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
static uint8_t s_POWER_ctrlCvccMode = POWER_CTRL_CVCC_CV;

static uint32_t s_POWER_getAppU32Nonnegative(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled <= 0.0F)
    {
        return 0U;
    }

    return (uint32_t)(scaled + 0.5F);
}

static int32_t s_POWER_getAppI32(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled >= 0.0F)
    {
        return (int32_t)(scaled + 0.5F);
    }

    return (int32_t)(scaled - 0.5F);
}

void POWER_initAppCtrl(void)
{
    s_POWER_ctrlEnabled = 0U;
    s_POWER_ctrlFaultFlags = 0U;
    s_POWER_ctrlStateFlagBits = POWER_CTRL_STATE_FLAG_WAIT;
    s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
    s_POWER_ctrlCvccMode = POWER_CTRL_CVCC_CV;
}

void POWER_getAppSnapshot(POWER_ctrlSnapshot_t *snapshot)
{
    BSP_adcResult_t adc_result;
    BSP_powerMeasurement_t measurement;

    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    BSP_getAppAdcResult(&adc_result);
    BSP_getAppMeasurement(&adc_result, &measurement);

    snapshot->input_voltage_mv = s_POWER_getAppU32Nonnegative(measurement.vin_v, 1000.0F);
    snapshot->input_current_ma = s_POWER_getAppU32Nonnegative(measurement.iin_a, 1000.0F);
    snapshot->output_voltage_mv = s_POWER_getAppU32Nonnegative(measurement.vout_v, 1000.0F);
    snapshot->output_current_ma = s_POWER_getAppU32Nonnegative(measurement.iout_a, 1000.0F);
    snapshot->core_temperature_mc = s_POWER_getAppI32(measurement.die_temp_c, 1000.0F);
    snapshot->board_temperature_mc = s_POWER_getAppI32(measurement.temp1_v, 1000.0F);

    snapshot->input_voltage_raw = adc_result.vin_raw;
    snapshot->input_current_raw = adc_result.iin_raw;
    snapshot->output_voltage_raw = adc_result.vout_raw;
    snapshot->output_current_raw = adc_result.iout_raw;

    snapshot->settings = s_POWER_ctrlSettings;
    snapshot->power_enabled = s_POWER_ctrlEnabled;
    snapshot->fault_flags = s_POWER_ctrlFaultFlags;
    snapshot->state_flag_bits = s_POWER_ctrlStateFlagBits;
    snapshot->stage_mode = s_POWER_ctrlStageMode;
    snapshot->cvcc_mode = s_POWER_ctrlCvccMode;

    snapshot->otp_value_mc = s_POWER_ctrlSettings.otp_set_mc;
    snapshot->ovp_value_mv = s_POWER_ctrlSettings.ovp_set_mv;
    snapshot->ocp_value_ma = s_POWER_ctrlSettings.ocp_set_ma;

    snapshot->fan_speed_permille = s_POWER_ctrlSettings.fan_set_permille;
    snapshot->loop_current_feedback_ma = snapshot->output_current_ma;
    snapshot->loop_current_reference_ma = s_POWER_ctrlSettings.set_current_ma;
    snapshot->voltage_loop_reference_mv = s_POWER_ctrlSettings.set_voltage_mv;
}

void POWER_applyAppSettings(const POWER_ctrlSettings_t *settings)
{
    if (settings == NULL)
    {
        return;
    }

    s_POWER_ctrlSettings = *settings;
}

void POWER_setAppEnabled(uint8_t enabled)
{
    s_POWER_ctrlEnabled = (enabled != 0U) ? 1U : 0U;
    s_POWER_ctrlStateFlagBits = (s_POWER_ctrlEnabled != 0U) ? POWER_CTRL_STATE_FLAG_RUN : POWER_CTRL_STATE_FLAG_WAIT;
}
