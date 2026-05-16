/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bdma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
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
#include "bdma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
DMA_HandleTypeDef hdma_memtomem_bdma_channel0;

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_bdma_channel0
  */
void MX_BDMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_BDMA_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_bdma_channel0 on BDMA_Channel0 */
  hdma_memtomem_bdma_channel0.Instance = BDMA_Channel0;
  hdma_memtomem_bdma_channel0.Init.Request = BDMA_REQUEST_MEM2MEM;
  hdma_memtomem_bdma_channel0.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_bdma_channel0.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_bdma_channel0.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_bdma_channel0.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_memtomem_bdma_channel0.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_memtomem_bdma_channel0.Init.Mode = DMA_NORMAL;
  hdma_memtomem_bdma_channel0.Init.Priority = DMA_PRIORITY_MEDIUM;
  if (HAL_DMA_Init(&hdma_memtomem_bdma_channel0) != HAL_OK)
  {
    Error_Handler();
  }

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

