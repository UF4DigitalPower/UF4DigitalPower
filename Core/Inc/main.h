/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FORWARD_IIN_Pin GPIO_PIN_0
#define FORWARD_IIN_GPIO_Port GPIOA
#define REVERSE_IIN_Pin GPIO_PIN_1
#define REVERSE_IIN_GPIO_Port GPIOA
#define FAN_CESU_Pin GPIO_PIN_5
#define FAN_CESU_GPIO_Port GPIOA
#define FAN_PWN_Pin GPIO_PIN_6
#define FAN_PWN_GPIO_Port GPIOA
#define VIN_Pin GPIO_PIN_7
#define VIN_GPIO_Port GPIOA
#define VOUT_Pin GPIO_PIN_0
#define VOUT_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_1
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_2
#define LED2_GPIO_Port GPIOB
#define REVERSE_IOUT_Pin GPIO_PIN_11
#define REVERSE_IOUT_GPIO_Port GPIOB
#define VREF165_Pin GPIO_PIN_12
#define VREF165_GPIO_Port GPIOB
#define FORWARD_IOUT_Pin GPIO_PIN_14
#define FORWARD_IOUT_GPIO_Port GPIOB
#define TEMP2_Pin GPIO_PIN_8
#define TEMP2_GPIO_Port GPIOA
#define TEMP1_Pin GPIO_PIN_9
#define TEMP1_GPIO_Port GPIOA
#define FLASH_SCLK_Pin GPIO_PIN_10
#define FLASH_SCLK_GPIO_Port GPIOC
#define FLASH_MISO_Pin GPIO_PIN_11
#define FLASH_MISO_GPIO_Port GPIOC
#define FLASH_MOSI_Pin GPIO_PIN_12
#define FLASH_MOSI_GPIO_Port GPIOC
#define FLASH_NSS_Pin GPIO_PIN_2
#define FLASH_NSS_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
