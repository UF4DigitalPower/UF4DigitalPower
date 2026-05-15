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
#include "stm32h7xx_hal.h"

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
#define KEY_UP_Pin GPIO_PIN_2
#define KEY_UP_GPIO_Port GPIOE
#define KEY_DN_Pin GPIO_PIN_3
#define KEY_DN_GPIO_Port GPIOE
#define KEY_R_Pin GPIO_PIN_4
#define KEY_R_GPIO_Port GPIOE
#define KEY_L_Pin GPIO_PIN_5
#define KEY_L_GPIO_Port GPIOE
#define KEY_M_Pin GPIO_PIN_6
#define KEY_M_GPIO_Port GPIOE
#define KEY_I_A_Pin GPIO_PIN_0
#define KEY_I_A_GPIO_Port GPIOA
#define KEY_I_B_Pin GPIO_PIN_1
#define KEY_I_B_GPIO_Port GPIOA
#define KEY_I_PUSH_Pin GPIO_PIN_2
#define KEY_I_PUSH_GPIO_Port GPIOA
#define UART_TO_PC_RX_Pin GPIO_PIN_12
#define UART_TO_PC_RX_GPIO_Port GPIOB
#define UART_TO_PC_TX_Pin GPIO_PIN_13
#define UART_TO_PC_TX_GPIO_Port GPIOB
#define UART_TO_POWER_TX_Pin GPIO_PIN_14
#define UART_TO_POWER_TX_GPIO_Port GPIOB
#define UART_TO_POWER_RX_Pin GPIO_PIN_15
#define UART_TO_POWER_RX_GPIO_Port GPIOB
#define KEY_A_PUSH_Pin GPIO_PIN_11
#define KEY_A_PUSH_GPIO_Port GPIOD
#define KEY_V_A_Pin GPIO_PIN_12
#define KEY_V_A_GPIO_Port GPIOD
#define KEY_V_B_Pin GPIO_PIN_13
#define KEY_V_B_GPIO_Port GPIOD
#define LCD_TP_SDA_Pin GPIO_PIN_9
#define LCD_TP_SDA_GPIO_Port GPIOC
#define LCD_TP_SCL_Pin GPIO_PIN_8
#define LCD_TP_SCL_GPIO_Port GPIOA
#define LCD_TP_INT_Pin GPIO_PIN_9
#define LCD_TP_INT_GPIO_Port GPIOA
#define LCD_TP_RST_Pin GPIO_PIN_10
#define LCD_TP_RST_GPIO_Port GPIOA
#define UART_TO_WIFI_RX_Pin GPIO_PIN_11
#define UART_TO_WIFI_RX_GPIO_Port GPIOA
#define UART_TO_WIFI_TX_Pin GPIO_PIN_12
#define UART_TO_WIFI_TX_GPIO_Port GPIOA
#define LCD_SCK_Pin GPIO_PIN_3
#define LCD_SCK_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_4
#define LCD_CS_GPIO_Port GPIOB
#define LCD_SDA_Pin GPIO_PIN_5
#define LCD_SDA_GPIO_Port GPIOB
#define LCD_RST_Pin GPIO_PIN_7
#define LCD_RST_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#define LCD_SPI_CS_Pin LCD_CS_Pin
#define LCD_SPI_CS_GPIO_Port LCD_CS_GPIO_Port
#define LCD_SPI_RESET_Pin LCD_RST_Pin
#define LCD_SPI_RESET_GPIO_Port LCD_RST_GPIO_Port
#define LCD_TP_RESET_Pin LCD_TP_RST_Pin
#define LCD_TP_RESET_GPIO_Port LCD_TP_RST_GPIO_Port

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
