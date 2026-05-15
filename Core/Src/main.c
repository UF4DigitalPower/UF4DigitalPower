/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma2d.h"
#include "i2c.h"
#include "ltdc.h"
#include "memorymap.h"
#include "quadspi.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "lvgl_port.h"
#include "st7701.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* 1: static diagnostic (no lv_timer_handler), 0: run LVGL benchmark normally */
#define TEAR_DIAG_STATIC_ONLY 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
static void LCD_ShowDemo(void);
static void LCD_UpdateRunMarker(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_QUADSPI_Init();
  MX_SPI6_Init();
  MX_TIM6_Init();
  MX_UART5_Init();
  MX_UART4_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  if (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
  {
    Error_Handler();
  }

  LCD_Init();
  st7701Init();
  HAL_LTDC_SetAddress(&hltdc, (uint32_t) ltdc_lcd_framebuf, 0);
  LCD_Display_Dir(0); //横屏

#if TEAR_DIAG_STATIC_ONLY
  LCD_ShowDemo();
#else
  LCD_Clear(BLACK);
  LVGL_Port_RunBenchmark();
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if TEAR_DIAG_STATIC_ONLY
    HAL_Delay(20);
#else
    uint32_t wait_ms = LVGL_Port_Task();
    if (wait_ms < 1U)
    {
      wait_ms = 1U;
    }
    else if (wait_ms > 5U)
    {
      wait_ms = 5U;
    }
    HAL_Delay(wait_ms);
#endif
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 15;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

static void LCD_ShowDemo(void)
{
  uint16_t i;

  LCD_Clear(BLACK);

  /* LANDSCAPE V3: top=red, right=green, bottom=blue, left=yellow. */
  LCD_Rect_Fill(0, 0, LCD_LOGICAL_WIDTH, 42, RED);
  LCD_Rect_Fill(LCD_LOGICAL_WIDTH - 42, 0, 42, LCD_LOGICAL_HEIGHT, GREEN);
  LCD_Rect_Fill(0, LCD_LOGICAL_HEIGHT - 42, LCD_LOGICAL_WIDTH, 42, BLUE);
  LCD_Rect_Fill(0, 0, 42, LCD_LOGICAL_HEIGHT, YELLOW);

  LCD_Rect_Fill(64, 64, 96, 96, RED);
  LCD_Rect_Fill(LCD_LOGICAL_WIDTH - 160, 64, 96, 96, GREEN);
  LCD_Rect_Fill(64, LCD_LOGICAL_HEIGHT - 160, 96, 96, BLUE);
  LCD_Rect_Fill(LCD_LOGICAL_WIDTH - 160, LCD_LOGICAL_HEIGHT - 160, 96, 96, WHITE);

  LCD_Rect_Fill(150, 215, 300, 50, WHITE);
  for (i = 0; i < 90; i += 6)
  {
    LCD_Rect_Fill(450 + (i / 2U), 170 + i, 6, 140 - i * 2U, WHITE);
  }

  LCD_Rect_Fill(210, 130, 40, 70, MAGENTA);
  LCD_Rect_Fill(210, 130, 140, 34, MAGENTA);
  LCD_Rect_Fill(310, 130, 40, 150, MAGENTA);
  LCD_Rect_Fill(210, 250, 140, 34, MAGENTA);

  POINT_COLOR = CYAN;
  LCD_DrawLine(0, 0, LCD_LOGICAL_WIDTH - 1, LCD_LOGICAL_HEIGHT - 1);
  LCD_DrawLine(LCD_LOGICAL_WIDTH - 1, 0, 0, LCD_LOGICAL_HEIGHT - 1);
  POINT_COLOR = WHITE;
  LCD_DrawRectangle(38, 38, LCD_LOGICAL_WIDTH - 39, LCD_LOGICAL_HEIGHT - 39);
}

static void LCD_UpdateRunMarker(void)
{
  static uint8_t state = 0;
  static const uint16_t colors[] = {RED, GREEN, BLUE, WHITE};

  LCD_Rect_Fill(LCD_LOGICAL_WIDTH - 112, LCD_LOGICAL_HEIGHT - 112, 64, 64, colors[state]);
  state = (uint8_t)((state + 1U) & 0x03U);
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
