/**
 ******************************************************************************
 * @file           : bsp_temp.c
 * @brief          :
 * @author         : UF4OVER
 * @date           : 2025/10/29
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 UF4.
 * All rights reserved.
 *
 ******************************************************************************
 */
//
// Created by 33974 on 2025/10/29.
//

/* Includes ------------------------------------------------------------------*/
#include "bsp_temp.h"
/* USER CODE BEGIN Includes */

#include <math.h>

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief 将ADC值转换为温度值(°C)，精度0.1°C
 * 使用公式: Rt = R * EXP(B * (1/T1 - 1/T2))
 * 其中T1和T2是开尔文温度(K)，K = 273.15 + 摄氏度
 * @param adc_value ADC读数 (0-4095 对应 0-3.3V)
 * @return 温度值(°C)
 */
float BSP_tempAdcToTemperature(uint32_t adc_value) {
    float voltage;
    float ntc_resistance;
    float t2;
    float ln_rt_over_r;
    float t1;
    float temperature;

    if (adc_value >= 4095U) {
        return -40.0F;
    }

    voltage = (float)(4095U - adc_value) * NTC_VREF / 4095.0F;
    ntc_resistance = NTC_R * voltage / (NTC_VREF - voltage);

    t2 = ABSOLUTE_ZERO + 25.0F;
    ln_rt_over_r = logf(ntc_resistance / NTC_R25);
    t1 = 1.0F / ((1.0F / t2) + (ln_rt_over_r / NTC_BETA));
    temperature = t1 - ABSOLUTE_ZERO;

    if (temperature < -40.0F) {
        temperature = -40.0F;
    } else if (temperature > 120.0F) {
        temperature = 120.0F;
    }

    return temperature;
}

float adcToTemperature(uint32_t adc_value) {
    return BSP_tempAdcToTemperature(adc_value);
}
float GetTemperature(void){
    uint32_t adc_value = 0U;
//    HAL_ADC_Start_DMA(&hadc1, &adc_value, 1);
    return adcToTemperature(adc_value);
}

/* USER CODE END 0 */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Exported functions --------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
