/**
  ******************************************************************************
  * @file    current_loop.c
  * @author  UF4
  * @date    26-9-26 下午5:23
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
#include "current_loop.h"
#include "control_config.h"
#include "current_loop_schedule.h"

typedef struct {
    int32_t error_q15;
    int32_t output_q15;
    uint16_t reference_mA;
    control_direction_t direction;
    control_region_t region;
} current_loop_state_t;

static current_loop_state_t s_current_loop;
static current_loop_result_t s_last_result;

CONTROL_FORCE_INLINE int32_t clamp_i32(int32_t value, int32_t low, int32_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

CONTROL_FORCE_INLINE int16_t adc_to_local_output_mA(const current_loop_sample_t *sample)
{
    int32_t delta;
    uint16_t adc = (sample->direction == CONTROL_DIRECTION_FORWARD) ?
                   sample->forward_iout_adc : sample->reverse_iout_adc;

    /* FWD sensor is positive below VREF; REV sensor is positive above VREF. */
    if (sample->direction == CONTROL_DIRECTION_FORWARD) {
        delta = (int32_t)sample->vref_adc - (int32_t)adc;
    } else {
        delta = (int32_t)adc - (int32_t)sample->vref_adc;
    }

    /* 3.3 V / 4095 counts / 0.160 V/A, expressed in mA. */
    delta = (delta * CONTROL_ADC_MA_PER_COUNT_NUM +
             (delta >= 0 ? CONTROL_ADC_MA_PER_COUNT_DEN / 2 :
                           -CONTROL_ADC_MA_PER_COUNT_DEN / 2)) /
            CONTROL_ADC_MA_PER_COUNT_DEN;
    return (int16_t)clamp_i32(delta, INT16_MIN, INT16_MAX);
}

CONTROL_FORCE_INLINE int32_t current_error_q15(int32_t reference_mA, int32_t measured_mA)
{
    int32_t error_mA = reference_mA - measured_mA;
    return clamp_i32((error_mA * 32768L) / CONTROL_I_BASE_MA, -32768, 32767);
}

CONTROL_RAMFUNC void CurrentLoop_Init(void)
{
    s_current_loop.direction = CONTROL_DIRECTION_FORWARD;
    s_current_loop.region = CONTROL_REGION_BUCK;
    s_current_loop.reference_mA = 0u;
    CurrentLoop_Reset();
}

CONTROL_RAMFUNC void CurrentLoop_Reset(void)
{
    s_current_loop.error_q15 = 0;
    s_current_loop.output_q15 = 0;
    s_last_result.duty_q15 = 0u;
    s_last_result.output_current_mA = 0;
    s_last_result.error_mA = 0;
    s_last_result.controller_q15 = 0;
    s_last_result.saturated = 0u;
}

CONTROL_RAMFUNC void CurrentLoop_SetMode(control_direction_t direction,
                                         control_region_t region)
{
    if ((direction != s_current_loop.direction) ||
        (region != s_current_loop.region)) {
        s_current_loop.direction = direction;
        s_current_loop.region = region;
        CurrentLoop_Reset();
    }
}

CONTROL_RAMFUNC void CurrentLoop_SetReference_mA(uint16_t reference_mA)
{
    s_current_loop.reference_mA =
        (reference_mA > CONTROL_I_SOFT_LIMIT_MA) ?
        CONTROL_I_SOFT_LIMIT_MA : reference_mA;
}

CONTROL_RAMFUNC uint16_t CurrentLoop_Update(const current_loop_sample_t *sample)
{
    int32_t measured_mA;
    int32_t error_q15;
    int32_t output_q15;
    int32_t controller_sum;
    int32_t duty_q15;
    int16_t b0;

    if (sample == 0) return s_last_result.duty_q15;

    CurrentLoop_SetMode(sample->direction, sample->region);
    CurrentLoop_SetReference_mA(sample->iref_mA);

    measured_mA = adc_to_local_output_mA(sample);
    error_q15 = current_error_q15((int32_t)s_current_loop.reference_mA,
                                  measured_mA);

    b0 = CurrentLoopSchedule_B0(sample->region, sample->vin_mV,
                                sample->vout_mV, s_current_loop.reference_mA);

    /* y[n] = 2^R * (B0*e[n] + B0*e[n-1] + A1*y[n-1]) / 32768. */
    controller_sum = (int32_t)b0 * error_q15 +
                     (int32_t)b0 * s_current_loop.error_q15 +
                     16384L * s_current_loop.output_q15;
    output_q15 = (controller_sum * 2L) / 32768L;
    output_q15 = clamp_i32(output_q15, -32768, 32767);
    duty_q15 = (int32_t)sample->duty_base_q15 +
               (output_q15 * CONTROL_DUTY_SCALE_Q15) / 32768;

    if (!sample->enable) {
        duty_q15 = sample->duty_base_q15;
        output_q15 = 0;
    }

    s_last_result.saturated =
        (duty_q15 <= CONTROL_DUTY_MIN_Q15 ||
         duty_q15 >= CONTROL_DUTY_MAX_Q15) ? 1u : 0u;
    duty_q15 = clamp_i32(duty_q15, CONTROL_DUTY_MIN_Q15,
                         CONTROL_DUTY_MAX_Q15);

    if (s_last_result.saturated) {
        /* Back-calculate the normalized controller state at the PWM limit. */
        output_q15 = ((duty_q15 - (int32_t)sample->duty_base_q15) * 32768) /
                     CONTROL_DUTY_SCALE_Q15;
        output_q15 = clamp_i32(output_q15, -32768, 32767);
    }

    s_current_loop.error_q15 = sample->enable ? error_q15 : 0;
    s_current_loop.output_q15 = output_q15;
    s_last_result.duty_q15 = (uint16_t)duty_q15;
    s_last_result.output_current_mA = (int16_t)clamp_i32(measured_mA,
                                                         INT16_MIN, INT16_MAX);
    s_last_result.error_mA = (int16_t)clamp_i32(
        (int32_t)s_current_loop.reference_mA - measured_mA,
        INT16_MIN, INT16_MAX);
    s_last_result.controller_q15 = (int16_t)output_q15;
    return s_last_result.duty_q15;
}

const current_loop_result_t *CurrentLoop_GetLastResult(void)
{
    return &s_last_result;
}
