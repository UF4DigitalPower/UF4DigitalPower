/**
  ******************************************************************************
  * @file    voltage_loop.c
  * @author  UF4
  * @date    26-9-26 下午5:22
  * @brief
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 UF4.
  * All rights reserved.
  *
  * This software is provided "as is", without warranty of any kind.
  *
  ******************************************************************************
  */
#include "voltage_loop.h"

typedef struct {
    int64_t integral_q16_mA;
    int32_t kp_q16_mA_per_mV;
    int32_t ki_ts_q30_mA_per_mV;
    uint16_t current_limit_mA;
} voltage_loop_state_t;

static voltage_loop_state_t s_voltage_loop;
static voltage_loop_result_t s_voltage_result;

CONTROL_FORCE_INLINE int32_t clamp_i32(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

CONTROL_FORCE_INLINE int64_t clamp_i64(int64_t value, int64_t low, int64_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

CONTROL_RAMFUNC void VoltageLoop_Init(void)
{
    s_voltage_loop.kp_q16_mA_per_mV = CONTROL_VOLTAGE_KP_Q16;
    s_voltage_loop.ki_ts_q30_mA_per_mV = CONTROL_VOLTAGE_KI_TS_Q30;
    s_voltage_loop.current_limit_mA = CONTROL_VOLTAGE_CURRENT_MAX_MA;
    VoltageLoop_Reset();
}

CONTROL_RAMFUNC void VoltageLoop_Reset(void)
{
    s_voltage_loop.integral_q16_mA = 0;
    s_voltage_result.current_reference_mA = 0u;
    s_voltage_result.voltage_error_mV = 0;
    s_voltage_result.proportional_mA = 0;
    s_voltage_result.integral_mA = 0;
    s_voltage_result.saturated = 0u;
}

CONTROL_RAMFUNC void VoltageLoop_SetGains_Q16_Q30(int32_t kp_q16,
                                                   int32_t ki_ts_q30)
{
    s_voltage_loop.kp_q16_mA_per_mV = (kp_q16 < 0) ? 0 : kp_q16;
    s_voltage_loop.ki_ts_q30_mA_per_mV = (ki_ts_q30 < 0) ? 0 : ki_ts_q30;
}

CONTROL_RAMFUNC void VoltageLoop_SetCurrentLimit_mA(uint16_t current_limit_mA)
{
    if (current_limit_mA > CONTROL_I_SOFT_LIMIT_MA) {
        current_limit_mA = CONTROL_I_SOFT_LIMIT_MA;
    }
    s_voltage_loop.current_limit_mA = current_limit_mA;
    s_voltage_loop.integral_q16_mA = clamp_i64(
        s_voltage_loop.integral_q16_mA, 0,
        (int64_t)current_limit_mA * 65536LL);
}

CONTROL_RAMFUNC uint16_t VoltageLoop_Update(const voltage_loop_sample_t *sample)
{
    int32_t error_mV;
    int64_t proportional_q16;
    int32_t proportional_mA;
    int64_t integral_candidate_q16;
    int64_t output_candidate_q16;
    int64_t current_limit_q16;
    int64_t integral_step_q16;
    uint8_t saturated;

    if (sample == 0) return s_voltage_result.current_reference_mA;

    if (!sample->enable) {
        VoltageLoop_Reset();
        return 0u;
    }

    if (sample->current_limit_mA != 0u) {
        VoltageLoop_SetCurrentLimit_mA(sample->current_limit_mA);
    }

    if (sample->reference_mV > CONTROL_VOUT_MAX_MV ||
        sample->reference_mV < CONTROL_VOUT_MIN_MV ||
        sample->voltage_mV > CONTROL_VOUT_MAX_MV) {
        VoltageLoop_Reset();
        return 0u;
    }

    error_mV = (int32_t)sample->reference_mV - (int32_t)sample->voltage_mV;
    proportional_q16 = (int64_t)s_voltage_loop.kp_q16_mA_per_mV * error_mV;
    proportional_mA = proportional_q16 / 65536L;
    current_limit_q16 = (int64_t)s_voltage_loop.current_limit_mA * 65536LL;

    /* Ki is stored as Ki*Ts in Q30; accumulate in Q16 mA. */
    integral_step_q16 = ((int64_t)s_voltage_loop.ki_ts_q30_mA_per_mV *
                         (int64_t)error_mV) / 16384LL;
    integral_candidate_q16 = clamp_i64(
        s_voltage_loop.integral_q16_mA + integral_step_q16,
        0, current_limit_q16);
    output_candidate_q16 = (int64_t)proportional_q16 +
                           integral_candidate_q16;
    saturated = (output_candidate_q16 < 0 ||
                 output_candidate_q16 > current_limit_q16) ? 1u : 0u;

    /* Conditional integration prevents windup at the 0 A and I-limit rails. */
    if (!saturated ||
        (output_candidate_q16 > current_limit_q16 && error_mV < 0) ||
        (output_candidate_q16 < 0 && error_mV > 0)) {
        s_voltage_loop.integral_q16_mA = integral_candidate_q16;
    }

    output_candidate_q16 = (int64_t)proportional_q16 +
                           s_voltage_loop.integral_q16_mA;
    saturated = (output_candidate_q16 < 0 ||
                 output_candidate_q16 > current_limit_q16) ? 1u : 0u;
    output_candidate_q16 = clamp_i64(output_candidate_q16, 0,
                                     current_limit_q16);

    s_voltage_result.current_reference_mA = (uint16_t)(output_candidate_q16 >> 16);
    s_voltage_result.voltage_error_mV = error_mV;
    s_voltage_result.proportional_mA = proportional_mA;
    s_voltage_result.integral_mA =
        (int32_t)(s_voltage_loop.integral_q16_mA >> 16);
    s_voltage_result.saturated = saturated;
    return s_voltage_result.current_reference_mA;
}

const voltage_loop_result_t *VoltageLoop_GetLastResult(void)
{
    return &s_voltage_result;
}
