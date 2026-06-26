/**
  ******************************************************************************
  * @file    pid.h
  * @author  UF4
  * @date    26-6-12 下午2:59
  * @brief
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 UF4.
  * All rights reserved.
  *
  * This software is provided "as is", without warranty of any kind.
  *
  ******************************************************************************
  */
#ifndef PID_H
#define PID_H

// 环路的参数buck输出-恒压-PID型补偿器
#define BUCKPIDb0 5795
#define BUCKPIDb1 -11411
#define BUCKPIDb2 5617

// 环路的参数BOOST输出-恒压-PID型补偿器
#define BOOSTPIDb0 8844
#define BOOSTPIDb1 -17413
#define BOOSTPIDb2 8572

// 环路的参数MIX输出-恒压-PID型补偿器
// MIX 临界区功率级特性不同于纯 BOOST，先用较 BOOST 保守的初值，后续按实测单独整定。
#define MIXPIDb0 6633
#define MIXPIDb1 -13060
#define MIXPIDb2 6429

#define ILOOP_KP 6 // 电流环PID补偿器P值
#define ILOOP_KI 2 // 电流环PID补偿器I值
#define ILOOP_KD 1 // 电流环PID补偿器D值

#define ILOOP_VREF_STEP_LIMIT 8         // 单次电流环调节输出参考的最大ADC码
#define ILOOP_INTEGRAL_LIMIT 2048       // 电流环积分限幅，避免过流后深度积分啸叫
#define ILOOP_VREF_OFFSET_LIMIT 2048    // 限流时允许累计拉低输出参考的最大ADC码
#define ILOOP_RELEASE_STEP 2            // 退出限流时输出参考恢复步进
#define ILOOP_ENTER_MARGIN 4            // 高于限流点该ADC码数时进入电流环
#define ILOOP_RELEASE_MARGIN 16         // 低于限流点该ADC码数时释放电流环
#define ILOOP_RELEASE_HOLD_CYCLES 400U  // 释放限流前保持周期，400周期@200kHz约2ms

#define VLOOP_ADC_FILTER_SHIFT 2U                                                             // 电压环积分量滤波系数
#define MIX_VLOOP_BOOST_DUTY_MAX_TICK ((int16_t)((BSP_POWER_BOOST_DUTY_MAX_TICK * 3U) / 4U))  // 混合模式下，buck的输出占空比最大值
#define ADC_SAMPLE_TICK_DEFAULT (int16_t)(BSP_POWER_HRTIM_PERIOD_TICK * 0.7)                  // 默认ADC采样时间
#define ADC_SAMPLE_TICK_MARGIN  680U                                                          // 默认ADC采样时间边沿
#define ADC_SAMPLE_TICK_MIN     ADC_SAMPLE_TICK_MARGIN                                        // 默认ADC采样时间最大
#define ADC_SAMPLE_TICK_MAX     (BSP_POWER_HRTIM_PERIOD_TICK - ADC_SAMPLE_TICK_MARGIN)        // 默认ADC采样时间最大

void PID_Init(void);                                // PID初始化
void BuckBoostVILoopCtlPID(void);                   // 恒压-PID型电压环控制
void PowerControl_SetAdcSampleTick(uint16_t tick);  // 设置ADC采样时间
uint16_t PowerControl_GetAdcSampleTick(void);       // 获取ADC采样时间

#endif //PID_H
