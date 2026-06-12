/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fmac.c
  * @brief   This file provides code for the configuration
  *          of the FMAC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "fmac.h"

/* USER CODE BEGIN 0 */
#include "function.h"
#include <string.h>

#define POWER_FMAC_TAP_COUNT 8U
#define POWER_FMAC_INPUT_BUFFER_SIZE 16U
#define POWER_FMAC_OUTPUT_BUFFER_SIZE 2U
#define POWER_FMAC_COEFF_BASE 0U
#define POWER_FMAC_X1_BASE 32U
#define POWER_FMAC_Y_BASE 64U
#define POWER_FMAC_Q15_SHIFT 4U

typedef struct
{
  int16_t history[POWER_FMAC_TAP_COUNT - 1U];
  int16_t output[1];
  uint16_t output_size;
  uint8_t primed;
} POWER_FMAC_FilterState_t;

static const int16_t s_POWER_fmacFirCoeff[POWER_FMAC_TAP_COUNT] = {
    4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096
};

CCRAM_BSS static POWER_FMAC_FilterState_t s_POWER_fmacVinState;
CCRAM_BSS static POWER_FMAC_FilterState_t s_POWER_fmacIinState;
CCRAM_BSS static POWER_FMAC_FilterState_t s_POWER_fmacVoutState;
CCRAM_BSS static POWER_FMAC_FilterState_t s_POWER_fmacIoutState;

static HAL_StatusTypeDef s_POWER_FMAC_ConfigFir(void)
{
  FMAC_FilterConfigTypeDef config = {0};

  config.InputBaseAddress = POWER_FMAC_X1_BASE;
  config.InputBufferSize = POWER_FMAC_INPUT_BUFFER_SIZE;
  config.InputThreshold = FMAC_THRESHOLD_1;
  config.CoeffBaseAddress = POWER_FMAC_COEFF_BASE;
  config.CoeffBufferSize = POWER_FMAC_TAP_COUNT;
  config.OutputBaseAddress = POWER_FMAC_Y_BASE;
  config.OutputBufferSize = POWER_FMAC_OUTPUT_BUFFER_SIZE;
  config.OutputThreshold = FMAC_THRESHOLD_1;
  config.pCoeffA = NULL;
  config.CoeffASize = 0U;
  config.pCoeffB = (int16_t *)s_POWER_fmacFirCoeff;
  config.CoeffBSize = POWER_FMAC_TAP_COUNT;
  config.InputAccess = FMAC_BUFFER_ACCESS_POLLING;
  config.OutputAccess = FMAC_BUFFER_ACCESS_POLLING;
  config.Clip = FMAC_CLIP_ENABLED;
  config.Filter = FMAC_FUNC_CONVO_FIR;
  config.P = POWER_FMAC_TAP_COUNT;
  config.Q = 0U;
  config.R = POWER_FMAC_Q15_SHIFT;

  return HAL_FMAC_FilterConfig(&hfmac, &config);
}

static void s_POWER_FMAC_ResetState(POWER_FMAC_FilterState_t *state)
{
  memset(state, 0, sizeof(*state));
  state->output_size = 1U;
}

static uint16_t s_POWER_FMAC_FilterSample(POWER_FMAC_FilterState_t *state, uint16_t sample)
{
  int16_t input_vector[POWER_FMAC_TAP_COUNT];
  uint16_t input_size = 1U;
  uint32_t i;

  if (sample > 0x7FFFU)
  {
    sample = 0x7FFFU;
  }

  if (HAL_FMAC_FilterStop(&hfmac) != HAL_OK)
  {
    return sample;
  }

  if (s_POWER_FMAC_ConfigFir() != HAL_OK)
  {
    return sample;
  }

  if (state->primed == 0U)
  {
    for (i = 0U; i < (POWER_FMAC_TAP_COUNT - 1U); ++i)
    {
      state->history[i] = (int16_t)sample;
    }
    state->primed = 1U;
  }

  for (i = 0U; i < (POWER_FMAC_TAP_COUNT - 1U); ++i)
  {
    input_vector[i] = state->history[i];
  }
  input_vector[POWER_FMAC_TAP_COUNT - 1U] = (int16_t)sample;

  if (HAL_FMAC_FilterPreload(&hfmac, input_vector, POWER_FMAC_TAP_COUNT - 1U, NULL, 0U) != HAL_OK)
  {
    return sample;
  }

  state->output_size = 1U;
  if (HAL_FMAC_FilterStart(&hfmac, state->output, &state->output_size) != HAL_OK)
  {
    return sample;
  }

  if (HAL_FMAC_AppendFilterData(&hfmac, &input_vector[POWER_FMAC_TAP_COUNT - 1U], &input_size) != HAL_OK)
  {
    return sample;
  }

  if (HAL_FMAC_PollFilterData(&hfmac, 1U) != HAL_OK)
  {
    return sample;
  }

  memmove(&state->history[0], &input_vector[1], sizeof(state->history));

  if (state->output[0] < 0)
  {
    return 0U;
  }

  return (uint16_t)state->output[0];
}

/* USER CODE END 0 */

FMAC_HandleTypeDef hfmac;

/* FMAC init function */
void MX_FMAC_Init(void)
{

  /* USER CODE BEGIN FMAC_Init 0 */

  /* USER CODE END FMAC_Init 0 */

  /* USER CODE BEGIN FMAC_Init 1 */

  /* USER CODE END FMAC_Init 1 */
  hfmac.Instance = FMAC;
  if (HAL_FMAC_Init(&hfmac) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FMAC_Init 2 */

  /* USER CODE END FMAC_Init 2 */

}

void HAL_FMAC_MspInit(FMAC_HandleTypeDef* fmacHandle)
{

  if(fmacHandle->Instance==FMAC)
  {
  /* USER CODE BEGIN FMAC_MspInit 0 */

  /* USER CODE END FMAC_MspInit 0 */
    /* FMAC clock enable */
    __HAL_RCC_FMAC_CLK_ENABLE();
  /* USER CODE BEGIN FMAC_MspInit 1 */

  /* USER CODE END FMAC_MspInit 1 */
  }
}

void HAL_FMAC_MspDeInit(FMAC_HandleTypeDef* fmacHandle)
{

  if(fmacHandle->Instance==FMAC)
  {
  /* USER CODE BEGIN FMAC_MspDeInit 0 */

  /* USER CODE END FMAC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_FMAC_CLK_DISABLE();
  /* USER CODE BEGIN FMAC_MspDeInit 1 */

  /* USER CODE END FMAC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void POWER_FMAC_InitFilters(void)
{
  s_POWER_FMAC_ResetState(&s_POWER_fmacVinState);
  s_POWER_FMAC_ResetState(&s_POWER_fmacIinState);
  s_POWER_FMAC_ResetState(&s_POWER_fmacVoutState);
  s_POWER_FMAC_ResetState(&s_POWER_fmacIoutState);

  if (s_POWER_FMAC_ConfigFir() != HAL_OK)
  {
    Error_Handler();
  }
}

uint16_t POWER_FMAC_FilterVin(uint16_t sample)
{
  return s_POWER_FMAC_FilterSample(&s_POWER_fmacVinState, sample);
}

uint16_t POWER_FMAC_FilterIin(uint16_t sample)
{
  return s_POWER_FMAC_FilterSample(&s_POWER_fmacIinState, sample);
}

uint16_t POWER_FMAC_FilterVout(uint16_t sample)
{
  return s_POWER_FMAC_FilterSample(&s_POWER_fmacVoutState, sample);
}

uint16_t POWER_FMAC_FilterIout(uint16_t sample)
{
  return s_POWER_FMAC_FilterSample(&s_POWER_fmacIoutState, sample);
}

/* USER CODE END 1 */
