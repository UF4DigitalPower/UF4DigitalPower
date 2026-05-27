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
#define ADC_VIN_Pin GPIO_PIN_0
#define ADC_VIN_GPIO_Port GPIOA
#define ADC_IIN_Pin GPIO_PIN_1
#define ADC_IIN_GPIO_Port GPIOA
#define ADC_VO_Pin GPIO_PIN_2
#define ADC_VO_GPIO_Port GPIOA
#define ADC_IO_Pin GPIO_PIN_3
#define ADC_IO_GPIO_Port GPIOA
#define SPI1_CS_Pin GPIO_PIN_0
#define SPI1_CS_GPIO_Port GPIOB
#define ADC_TEMP1_Pin GPIO_PIN_1
#define ADC_TEMP1_GPIO_Port GPIOB
#define ADC_TEMP2_Pin GPIO_PIN_2
#define ADC_TEMP2_GPIO_Port GPIOB
#define BLUE_Pin GPIO_PIN_11
#define BLUE_GPIO_Port GPIOB
#define GREEN_Pin GPIO_PIN_12
#define GREEN_GPIO_Port GPIOB
#define RED_Pin GPIO_PIN_13
#define RED_GPIO_Port GPIOB
#define PWM_L2_Pin GPIO_PIN_14
#define PWM_L2_GPIO_Port GPIOB
#define PWM_H2_Pin GPIO_PIN_15
#define PWM_H2_GPIO_Port GPIOB
#define PWM_L1_Pin GPIO_PIN_8
#define PWM_L1_GPIO_Port GPIOA
#define PWM_H1_Pin GPIO_PIN_9
#define PWM_H1_GPIO_Port GPIOA
#define FAN_PWM_Pin GPIO_PIN_9
#define FAN_PWM_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
