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

#endif /* CONTROL_TYPES_H */
