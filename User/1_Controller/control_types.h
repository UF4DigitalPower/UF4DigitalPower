/**
  ******************************************************************************
  * @file    control_types.h
  * @author  UF4
  * @date    26-9-26 下午5:24
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
#ifndef CONTROL_TYPES_H
#define CONTROL_TYPES_H

#include <stdint.h>

typedef enum {
    CONTROL_DIRECTION_FORWARD = 0u,
    CONTROL_DIRECTION_REVERSE = 1u
} control_direction_t;

typedef enum {
    CONTROL_REGION_BUCK = 0u,
    CONTROL_REGION_BOOST = 1u
} control_region_t;

typedef struct {
    uint32_t vin_mV;
    uint32_t vout_mV;
    uint16_t vref_adc;
    uint16_t adc_vdda_mV;
    int16_t b0_q15;
    uint16_t forward_iout_adc;
    uint16_t reverse_iout_adc;
    uint16_t duty_base_q15;
    uint16_t iref_mA;
    control_direction_t direction;
    control_region_t region;
    uint8_t enable;
} current_loop_sample_t;

typedef struct {
    uint16_t duty_q15;
    int16_t output_current_mA;
    int16_t error_mA;
    int16_t controller_q15;
    uint8_t saturated;
} current_loop_result_t;

typedef struct {
    uint32_t voltage_mV;
    uint32_t reference_mV;
    uint16_t current_limit_mA;
    uint8_t enable;
} voltage_loop_sample_t;

typedef struct {
    uint16_t current_reference_mA;
    int32_t voltage_error_mV;
    int32_t proportional_mA;
    int32_t integral_mA;
    uint8_t saturated;
} voltage_loop_result_t;

#endif /* CONTROL_TYPES_H */
