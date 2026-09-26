/**
  ******************************************************************************
  * @file    current_loop.h
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
#ifndef CURRENT_LOOP_H
#define CURRENT_LOOP_H

#include "control_config.h"
#include "control_types.h"

void CurrentLoop_Init(void);
void CurrentLoop_Reset(void);
void CurrentLoop_SetMode(control_direction_t direction, control_region_t region);
void CurrentLoop_SetReference_mA(uint16_t reference_mA);

CONTROL_RAMFUNC uint16_t CurrentLoop_Update(const current_loop_sample_t *sample);
const current_loop_result_t *CurrentLoop_GetLastResult(void);

#endif /* CURRENT_LOOP_H */
