/**
 * @file power_ctrl.c
 * @brief Minimal power-control state backing for protocol integration.
 */

#include "power_ctrl.h"

#include <string.h>

static power_ctrl_settings_t g_power_ctrl_settings = {
    .set_voltage_mv = 5000U,
    .set_current_ma = 10000U,
    .otp_set_mc = 80000,
    .ovp_set_mv = 50000U,
    .ocp_set_ma = 10500U,
    .fan_set_permille = 0U,
};

static uint8_t g_power_ctrl_enabled = 0U;
static uint32_t g_power_ctrl_fault_flags = 0U;
static uint8_t g_power_ctrl_state_flag_bits = POWER_CTRL_STATE_FLAG_WAIT;
static uint8_t g_power_ctrl_stage_mode = POWER_CTRL_STAGE_NA;
static uint8_t g_power_ctrl_cvcc_mode = POWER_CTRL_CVCC_CV;

static uint32_t power_ctrl_float_to_u32_nonnegative(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled <= 0.0F)
    {
        return 0U;
    }

    return (uint32_t)(scaled + 0.5F);
}

static int32_t power_ctrl_float_to_i32(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled >= 0.0F)
    {
        return (int32_t)(scaled + 0.5F);
    }

    return (int32_t)(scaled - 0.5F);
}

void PowerCtrl_Init(void)
{
    g_power_ctrl_enabled = 0U;
    g_power_ctrl_fault_flags = 0U;
    g_power_ctrl_state_flag_bits = POWER_CTRL_STATE_FLAG_WAIT;
    g_power_ctrl_stage_mode = POWER_CTRL_STAGE_NA;
    g_power_ctrl_cvcc_mode = POWER_CTRL_CVCC_CV;
}

void PowerCtrl_GetSnapshot(power_ctrl_snapshot_t *snapshot)
{
    ADC_RESULT_t adc_result;
    POWER_MEASUREMENT_t measurement;

    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    BSP_Power_GetAdcResult(&adc_result);
    BSP_Power_ConvertToMeasurement(&adc_result, &measurement);

    snapshot->input_voltage_mv = power_ctrl_float_to_u32_nonnegative(measurement.vin_v, 1000.0F);
    snapshot->input_current_ma = power_ctrl_float_to_u32_nonnegative(measurement.iin_a, 1000.0F);
    snapshot->output_voltage_mv = power_ctrl_float_to_u32_nonnegative(measurement.vout_v, 1000.0F);
    snapshot->output_current_ma = power_ctrl_float_to_u32_nonnegative(measurement.iout_a, 1000.0F);
    snapshot->core_temperature_mc = power_ctrl_float_to_i32(measurement.die_temp_c, 1000.0F);
    snapshot->board_temperature_mc = power_ctrl_float_to_i32(measurement.temp1_v, 1000.0F);

    snapshot->input_voltage_raw = adc_result.vin_raw;
    snapshot->input_current_raw = adc_result.iin_raw;
    snapshot->output_voltage_raw = adc_result.vout_raw;
    snapshot->output_current_raw = adc_result.iout_raw;

    snapshot->settings = g_power_ctrl_settings;
    snapshot->power_enabled = g_power_ctrl_enabled;
    snapshot->fault_flags = g_power_ctrl_fault_flags;
    snapshot->state_flag_bits = g_power_ctrl_state_flag_bits;
    snapshot->stage_mode = g_power_ctrl_stage_mode;
    snapshot->cvcc_mode = g_power_ctrl_cvcc_mode;

    snapshot->otp_value_mc = g_power_ctrl_settings.otp_set_mc;
    snapshot->ovp_value_mv = g_power_ctrl_settings.ovp_set_mv;
    snapshot->ocp_value_ma = g_power_ctrl_settings.ocp_set_ma;

    snapshot->fan_speed_permille = g_power_ctrl_settings.fan_set_permille;
    snapshot->loop_current_feedback_ma = snapshot->output_current_ma;
    snapshot->loop_current_reference_ma = g_power_ctrl_settings.set_current_ma;
    snapshot->voltage_loop_reference_mv = g_power_ctrl_settings.set_voltage_mv;
}

void PowerCtrl_ApplySettings(const power_ctrl_settings_t *settings)
{
    if (settings == NULL)
    {
        return;
    }

    g_power_ctrl_settings = *settings;
}

void PowerCtrl_SetEnabled(uint8_t enabled)
{
    g_power_ctrl_enabled = (enabled != 0U) ? 1U : 0U;
    g_power_ctrl_state_flag_bits = (g_power_ctrl_enabled != 0U) ? POWER_CTRL_STATE_FLAG_RUN : POWER_CTRL_STATE_FLAG_WAIT;
}
