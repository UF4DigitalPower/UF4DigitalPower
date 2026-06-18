/**
 ******************************************************************************
 * @file           : temp.c
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

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>
#include "temp.h"
#include "stdint.h"
#include "adc.h"
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
 * @brief 一阶低通滤波器。
 * 使用一阶低通滤波算法对输入信号进行滤波处理。
 * @param input 输入信号
 * @param alpha 滤波系数
 * @return 滤波后的输出信号
 */
static float one_order_lowpass_filter(float input, float alpha)
{
    static float prev_output = 0.0F;                             // 静态变量，用于保存上一次的输出值
    const float output = alpha * input + (1.0F - alpha) * prev_output; // 一阶低通滤波算法
    prev_output = output;                                        // 保存本次输出值，以备下一次使用
    return output;                                               // 返回滤波后的输出信号
}

/**
 * @brief 将ADC值转换为温度值(°C)，精度0.1°C
 * 使用公式: Rt = R * EXP(B * (1/T1 - 1/T2))
 * 其中T1和T2是开尔文温度(K)，K = 273.15 + 摄氏度
 * @param adc_value ADC读数 (0-4095 对应 0-3.3V)
 * @return 温度值(°C)
 */
float BSP_tempAdcToTemperature(uint32_t adc_value)
{
    if (adc_value >= 4095U) {
        return -40.0F;
    }

    const float voltage = (float) (4095U - adc_value) * NTC_VREF / 4095.0F;
    const float ntc_resistance = NTC_R * voltage / (NTC_VREF - voltage);

    const float t2 = ABSOLUTE_ZERO + 25.0F;
    const float ln_rt_over_r = logf(ntc_resistance / NTC_R25);
    const float t1 = 1.0F / (1.0F / t2 + (ln_rt_over_r / NTC_BETA));
    float temperature = t1 - ABSOLUTE_ZERO;

    if (temperature < -40.0F) {
        temperature = -40.0F;
    } else if (temperature > 120.0F) {
        temperature = 120.0F;
    }

    return one_order_lowpass_filter(temperature,0.1F);
}

float GET_NTC1_Temperature(void) {
    HAL_ADC_Start(&hadc2); // 启动ADC2采样，采样NTC温度
    // HAL_ADC_PollForConversion(&hadc2, 100); // 等待ADC采样结束
    const uint32_t TEMP_adcValue = HAL_ADC_GetValue(&hadc2);   // 读取ADC2采样结果
    return BSP_tempAdcToTemperature(TEMP_adcValue);
};

/**
 * @brief 读取 NTC2 通道温度。
 * 启动 ADC3 采样并将当前转换结果换算为摄氏度。
 */
float GET_NTC2_Temperature(void)
{
    HAL_ADC_Start(&hadc3); // 启动ADC2采样，采样NTC温度
    // HAL_ADC_PollForConversion(&hadc2, 100); // 等待ADC采样结束
    const uint32_t TEMP_adcValue = HAL_ADC_GetValue(&hadc3);   // 读取ADC2采样结果
    return BSP_tempAdcToTemperature(TEMP_adcValue);
};

/**
 * @brief 读取 MCU 内部温度。
 * 使用出厂校准点对 ADC5 结果做换算，并经过一阶低通滤波后返回。
 */
float GET_CPU_Temperature(void)
{
    HAL_ADC_Start(&hadc5); // 启动ADC5采样，采样单片机CPU温度
    // HAL_ADC_PollForConversion(&hadc5, 100); // 等待ADC采样结束
    const float Temp_Scale = (TS_CAL2_TEMP - TS_CAL1_TEMP) / (float)(TS_CAL2 - TS_CAL1); // 计算温度比例因子
    const float temperature = Temp_Scale * (HAL_ADC_GetValue(&hadc5) * (NTC_VREF / 3.0F) - TS_CAL1) + TS_CAL1_TEMP; // 计算温度
    return one_order_lowpass_filter(temperature, 0.1F); // 返回温度值
};
/* USER CODE END 0 */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Exported functions --------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
