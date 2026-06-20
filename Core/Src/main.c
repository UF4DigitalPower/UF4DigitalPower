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
#include "adc.h"
#include "dma.h"
#include "hrtim.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "function.h"
#include "pid.h"
#include "uf4_transport.h"
#include "status_led.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

volatile uint16_t ms_cnt_1 = 0; // 计时变量1
volatile uint16_t ms_cnt_2 = 0; // 计时变量2
volatile uint16_t ms_cnt_4 = 0; // 计时变量4

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC5_Init();
  MX_HRTIM1_Init();
  MX_SPI1_Init();
  MX_TIM8_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_ADC2_Init();
  MX_ADC3_Init();
  MX_USB_Device_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_TIM16_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

  DF.SMFlag = Init;                         // 初始化状态机

  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3); // 启动定时器8和通道3的PWM输出
  FAN_PWM_set(POWER_CTRL_FAN_MAX_RUN_DUTY); // 设置风扇转速到安全上限

  HAL_TIM_Base_Start_IT(&htim6);            // 启动定时器2和定时器中断，1kHz
  HAL_TIM_Base_Start_IT(&htim7);            // 启动定时器3和定时器中断，200Hz
  HAL_TIM_Base_Start_IT(&htim16);           // 启动定时器4和定时器中断，100Hz

  StatusLed_Init();                         // 初始化三色状态指示灯

  PID_Init();                               // PID初始化
  ValInit();                                // 先装载默认值，供 Flash 首次初始化和非法数据兜底使用
  Init_Flash();                             // Flash初始化
  Read_Flash();                             // 读取Flash数据
  UF4Transport_Init();                      // UF4COM初始化

  HAL_GPIO_WritePin(DIV_SW_GPIO_Port, DIV_SW_Pin, GPIO_PIN_SET);  // 启动栅极驱动器

  HAL_Delay(100);                            // 延时100ms，等待供电稳定

  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED); // 校准ADC1
  HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED); // 校准ADC2
  HAL_ADCEx_Calibration_Start(&hadc5, ADC_SINGLE_ENDED); // 校准ADC5

  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)(void *)ADC1_RESULT, 4); // 启动ADC1采样和DMA数据传送,采样输入输出电压电流
  HAL_ADC_Start(&hadc2);                                 // 启动ADC2采样，采样NTC1温度
  HAL_ADC_Start(&hadc3);                                 // 启动ADC3采样，采样NTC2温度
  HAL_ADC_Start(&hadc5);                                 // 启动ADC5采样，采样单片机CPU温度

  HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A);              // 开启HRTIM波形计数器
  HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_D);              // 开启HRTIM波形计数器
  __HAL_HRTIM_TIMER_ENABLE_IT(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_TIM_IT_REP); // 开启HRTIM定时器D的中断

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (ms_cnt_4 >= 50){     // 判断是否计时到50ms
      ms_cnt_4 = 0;          // 计时清零
      StatusLed_Update();    // 更新三色状态指示灯

      if (ms_cnt_1 >= 500){  // 判断是否计时到500ms
        ms_cnt_1 = 0;        // 计时清零
        Update_Flash();      // 更新Flash存储内容
      }
    }
    UF4Transport_RunTask();  // 通信后台任务，处理挂起发送和原始数据流

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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV5;
  RCC_OscInitStruct.PLL.PLLN = 68;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV8;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */

/**
 * @brief HAL_TIM_PeriodElapsedCallback函数,中断回调函数
 *
 * 当定时器周期结束时，该函数将被调用。
 *
 * @param htim TIM句柄指针
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6){ // 定时器TIM2，中断时间1ms
    ms_cnt_1++;
    ms_cnt_2++;
    ms_cnt_4++;
    HAL_IWDG_Refresh(&hiwdg); // 在固定节拍中断里喂狗，避免主循环被快环抢占时漏喂
  }
  if (htim->Instance == TIM7){ // 定时器TIM3，中断时间5ms

    ADCSample(); // ADC采样滤波函数
    ShortOff();  // 短路保护
    OTP();       // 过温保护
    OVP();       // 输出过压保护
    OCP();       // 输出过流保护
    StateM();    // 电源状态机函数
    BBMode();    // 运行模式判断
    // 流发送放在 TIM7 中断里（每 4 次 = 20ms），避免输出开启后主循环被
    // HRTIM PID 中断饿死导致 STREAM_DATA 断流。
    {
      static uint8_t stream_div = 0;
      stream_div++;
      if (stream_div >= 4U) {
        stream_div = 0U;
        UF4Transport_StreamTick();
      }
    }
  }
  if (htim->Instance == TIM16) // 定时器TIM4，中断时间10ms
  {
    /* 把测量量换算放到固定 10 ms 节拍里，避免输出开启后主循环被高频控制打断，
       导致 UF4COM 继续上报旧的 VIN/VOUT/IIN/IOUT 浮点值。 */
    ADC_calculate();
  }
}
/* USER CODE END 4 */

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
