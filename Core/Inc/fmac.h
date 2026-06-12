/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fmac.h
  * @brief   This file contains all the function prototypes for
  *          the fmac.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FMAC_H__
#define __FMAC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>

/* USER CODE END Includes */

extern FMAC_HandleTypeDef hfmac;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_FMAC_Init(void);

/* USER CODE BEGIN Prototypes */
void POWER_FMAC_InitFilters(void);
uint16_t POWER_FMAC_FilterVin(uint16_t sample);
uint16_t POWER_FMAC_FilterIin(uint16_t sample);
uint16_t POWER_FMAC_FilterVout(uint16_t sample);
uint16_t POWER_FMAC_FilterIout(uint16_t sample);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __FMAC_H__ */

