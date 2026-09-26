/**
  ******************************************************************************
  * @file    control_config.h
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
#ifndef CONTROL_CONFIG_H
#define CONTROL_CONFIG_H

#include <stdint.h>

#if defined(__GNUC__)
#define CONTROL_RAMFUNC __attribute__((section(".RamFunc")))
#define CONTROL_FORCE_INLINE static inline __attribute__((always_inline))
#else
#define CONTROL_RAMFUNC
#define CONTROL_FORCE_INLINE static inline
#endif

#define CONTROL_PWM_HZ             200000u
#define CONTROL_CURRENT_LOOP_HZ    100000u
#define CONTROL_VOLTAGE_LOOP_HZ     10000u

#define CONTROL_I_BASE_MA          10000
#define CONTROL_I_SOFT_LIMIT_MA    10000
#define CONTROL_I_OCP_HW_MA        11000
#define CONTROL_ADC_FS             4095
#define CONTROL_ADC_MA_PER_COUNT_NUM 5037L
#define CONTROL_ADC_MA_PER_COUNT_DEN 1000L

#define CONTROL_DUTY_MIN_Q15       0
#define CONTROL_DUTY_MAX_Q15       32767
#define CONTROL_DUTY_SCALE_Q15     6554

#define CONTROL_VIN_MIN_MV         12000u
#define CONTROL_VIN_MAX_MV         70000u
#define CONTROL_VOUT_MIN_MV         1000u
#define CONTROL_VOUT_MAX_MV        70000u
#define CONTROL_BOOST_RATIO_MAX       2u

#endif /* CONTROL_CONFIG_H */
