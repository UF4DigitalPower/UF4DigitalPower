/**
  ******************************************************************************
  * @file    voltage_loop.h
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
#ifndef VOLTAGE_LOOP_H
#define VOLTAGE_LOOP_H

#include "control_config.h"
#include "control_types.h"

void VoltageLoop_Init(void);
void VoltageLoop_Reset(void);
void VoltageLoop_SetGains_Q16_Q30(int32_t kp_q16, int32_t ki_ts_q30);
void VoltageLoop_SetCurrentLimit_mA(uint16_t current_limit_mA);

CONTROL_RAMFUNC uint16_t VoltageLoop_Update(const voltage_loop_sample_t *sample);
const voltage_loop_result_t *VoltageLoop_GetLastResult(void);

#endif /* VOLTAGE_LOOP_H */
