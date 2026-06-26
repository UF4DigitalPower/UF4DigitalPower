/**
  ******************************************************************************
  * @file    pid.c
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
#include "stdint.h"
#include "pid.h"
#include "function.h"

extern volatile uint16_t ADC1_RESULT[4];                                 // ADC1 DMA采样结果：Vout, Iout, Vin, Iin
CCRAM volatile int32_t VErr0 = 0, VErr1 = 0, VErr2 = 0;                  // 电压误差
CCRAM volatile int32_t IErr0 = 0, IErr1 = 0;                             // 电流误差
CCRAM volatile int32_t u0 = 0, u1 = 0;                                   // 电压环输出量
CCRAM volatile int32_t i0 = 0, i1 = 0;                                   // 电流环输出量
CCRAM volatile _CVCC_Mode CVCC_Mode = CV;                                // 恒流恒压模式标志位
CCRAM volatile uint16_t g_adc_sample_tick = ADC_SAMPLE_TICK_DEFAULT;     // ADC采样时间

static CCRAM int32_t s_vout_pid_filter = 0;                              // 电压环积分量滤波量
static CCRAM uint8_t s_vout_pid_filter_valid = 0U;                       // 电压环积分量滤波量是否有效


UF4_FORCEINLINE int32_t s_VLoop_DutyMinToLoopLimit(int16_t duty_tick){  // 获取电压环输出最小值
    return ((((int32_t)duty_tick + 2) / 3) << 8);
}

UF4_FORCEINLINE int32_t s_VLoop_DutyMaxToLoopLimit(int16_t duty_tick){  // 获取电压环输出最大值
    return (((int32_t)duty_tick / 3) << 8);
}

UF4_FORCEINLINE void s_VLoop_ClampOutputState(int16_t min_duty, int16_t max_duty)  // 限制电压环输出
{
    int32_t min_u = s_VLoop_DutyMinToLoopLimit(min_duty);
    int32_t max_u = s_VLoop_DutyMaxToLoopLimit(max_duty);

    if (max_u < min_u){
        max_u = min_u;
    }
    if (u0 > max_u){
        u0 = max_u;
    }
    if (u0 < min_u){
        u0 = min_u;
    }
    u1 = u0;
}

UF4_FORCEINLINE int16_t s_VLoop_GetMixBoostDutyMax(void)
{
    int16_t max_duty = MIX_VLOOP_BOOST_DUTY_MAX_TICK;

    if (max_duty > CtrValue.BoostMaxDuty){
        max_duty = CtrValue.BoostMaxDuty;
    }
    if (max_duty < BSP_POWER_BOOST_DUTY_MIN_TICK){
        max_duty = BSP_POWER_BOOST_DUTY_MIN_TICK;
    }
    return max_duty;
}

UF4_FORCEINLINE uint16_t s_PowerControl_ClampAdcSampleTick(uint16_t tick)
{
    if (tick < ADC_SAMPLE_TICK_MIN){
        return ADC_SAMPLE_TICK_MIN;
    }
    if (tick > ADC_SAMPLE_TICK_MAX){
        return ADC_SAMPLE_TICK_MAX;
    }
    return tick;
}

void PowerControl_SetAdcSampleTick(uint16_t tick)
{
    g_adc_sample_tick = s_PowerControl_ClampAdcSampleTick(tick);
}

RAMFUNC uint16_t PowerControl_GetAdcSampleTick(void)
{
    return s_PowerControl_ClampAdcSampleTick(g_adc_sample_tick);
}

/**
 * @brief 初始化 PID 环路相关状态量。
 * 清零误差项、积分项和控制输出，避免上电时继承旧状态。
 */
void PID_Init(void)
{
  VErr0 = 0;
  VErr1 = 0;
  VErr2 = 0;
  u0 = 0;
  u1 = 0;
  i0 = 0;
  i1 = 0;
  IErr0 = 0;
  IErr1 = 0;
  CVCC_Mode = CV;
  s_vout_pid_filter = 0;
  s_vout_pid_filter_valid = 0U;
}


/**
 * @brief BuckBoost电压电流环路控制PID函数。
 * 该函数用于实现BuckBoost电压电流环路控制的PID算法。
 * 在stm32g4xx_it.c文件中的HRTIM1_TIMD_IRQHandler中断函数里调用此函数。
 */
RAMFUNC void BuckBoostVILoopCtlPID(void){
    static CCRAM int32_t I_Integral = 0; // 电流环路积分量
    static CCRAM int32_t i_vref_offset = 0; // 电流环累计拉低输出参考量
    static CCRAM uint8_t i_limit_active = 0U;
    static CCRAM uint16_t i_release_cnt = 0U;

    CtrValue.Vout_ref = CtrValue.Vout_SETref; // 输出参考电压设置为设置电压

    int32_t VoutRaw = (ADC1_RESULT[ADC1_RESULT_VOUT_INDEX] * CAL_VOUT_K >> 12) + CAL_VOUT_B; // 获取矫正后的输出电压
    int32_t IoutTemp = (ADC1_RESULT[ADC1_RESULT_IOUT_INDEX] * CAL_IOUT_K >> 12) + CAL_IOUT_B; // 获取矫正后的输出电流

    if (s_vout_pid_filter_valid == 0U || DF.PWMENFlag == 0U){
        s_vout_pid_filter = VoutRaw;
        s_vout_pid_filter_valid = 1U;
    }
    else{
        s_vout_pid_filter += (VoutRaw - s_vout_pid_filter) >> VLOOP_ADC_FILTER_SHIFT;
    }

    int32_t VoutTemp = s_vout_pid_filter;  // 获取电压环输出

    if (DF.PWMENFlag == 0U || DF.OUTPUT_Flag == 0U){
        I_Integral = 0;
        i_limit_active = 0U;
        i_release_cnt = 0U;
        i_vref_offset = 0;
    }

    // 输出电流采样为低于偏置代表正电流；低于参考码表示过流，需要降低电压参考。
    IErr0 = IoutTemp - CtrValue.Iout_ref;

    if (IoutTemp <= (CtrValue.Iout_ref - ILOOP_ENTER_MARGIN)){
        i_limit_active = 1U;
        i_release_cnt = 0U;
    }  // 输入电流过低，需要拉低输出参考量。
    else if (i_limit_active != 0U && IoutTemp >= (CtrValue.Iout_ref + ILOOP_RELEASE_MARGIN)){
        if (i_release_cnt < ILOOP_RELEASE_HOLD_CYCLES){
            i_release_cnt++;
        }
        else{
            i_limit_active = 0U;
            i_release_cnt = 0U;
            I_Integral = 0;
            i0 = 0;
        }
    }  // 输入电流过高，需要拉高输出参考量。
    else{
        i_release_cnt = 0U;
    }  // 输入电流正常，正常处理。

    if (i_limit_active == 0U){
        IErr1 = IErr0;
        I_Integral = 0;
        i0 = 0;
        if (i_vref_offset > 0){
            i_vref_offset -= ILOOP_RELEASE_STEP;
            if (i_vref_offset < 0){
                i_vref_offset = 0;
            }
        }
    }  // 输入电流正常，正常处理。
    else{
        // 电流环路输出= 积分量 + KP*误差量 + KD*当前误差减上次误差
        i0 = I_Integral + IErr0 * ILOOP_KP + (IErr0 - IErr1) * ILOOP_KD;
        // 积分量=积分量+KI*误差量
        I_Integral = I_Integral + IErr0 * ILOOP_KI;

        // 积分量双向限幅，避免过流后深度积分导致输出急拉和啸叫。
        if (I_Integral > ILOOP_INTEGRAL_LIMIT)
            I_Integral = ILOOP_INTEGRAL_LIMIT;
        if (I_Integral < -ILOOP_INTEGRAL_LIMIT)
            I_Integral = -ILOOP_INTEGRAL_LIMIT;

        if (i0 > ILOOP_VREF_STEP_LIMIT)
            i0 = ILOOP_VREF_STEP_LIMIT;
        if (i0 < -ILOOP_VREF_STEP_LIMIT)
            i0 = -ILOOP_VREF_STEP_LIMIT;

        i_vref_offset -= i0;
        if (i_vref_offset < 0)
            i_vref_offset = 0;
        if (i_vref_offset > ILOOP_VREF_OFFSET_LIMIT)
            i_vref_offset = ILOOP_VREF_OFFSET_LIMIT;

        IErr1 = IErr0;
    }  // 输入电流过高，需要拉高输出参考量。

    if (DF.SMFlag == Rise){ // 判断是否在软启动状态

        CtrValue.Vout_ref = CtrValue.Vout_ref - i_vref_offset;  // 输出参考电压减去电流环限流偏移
        CVCC_Mode = (i_vref_offset > 0) ? CC : CV;              // 恒流模式
        if (CtrValue.Vout_ref > CtrValue.Vout_SSref){ // 输出参考电压超过软启动设置电压时限制在软启动设置电压
            CtrValue.Vout_ref = CtrValue.Vout_SSref; // 限制输出参考电压
            CVCC_Mode = CV;                          // 恒压模式
        }
        if (CtrValue.Vout_ref < 0){ // 输出参考电压小于0时限制在0
            CtrValue.Vout_ref = 0;
        }
    }  // 软启动状态
    else{
        CtrValue.Vout_ref = CtrValue.Vout_ref - i_vref_offset;  // 输出参考电压减去电流环限流偏移
        CVCC_Mode = (i_vref_offset > 0) ? CC : CV;              // 恒流模式
        if (CtrValue.Vout_ref > CtrValue.Vout_SETref){          // 输出参考电压超过设置电压时限制在设置电压
            CtrValue.Vout_ref = CtrValue.Vout_SETref;           // 限制输出参考电压
            CVCC_Mode = CV;                                     // 恒压模式
        }
        if (CtrValue.Vout_ref < 0){                             // 输出参考电压小于0时限制在0
            CtrValue.Vout_ref = 0;
        }
    }  // 正常状态

    VErr0 = CtrValue.Vout_ref - VoutTemp; // 计算电压误差量，当参考电压大于输出电压，占空比增加，输出量增加

    // 当模式切换时，降低占空比，确保模式切换不过冲
    // BBModeChange为模式切换为，不同模式切换时，该位会被置1
    if (DF.BBModeChange){
        int32_t switch_v_err = CtrValue.Vout_ref - VoutRaw;
        u1 = 0;
        VErr0 = switch_v_err;
        VErr1 = switch_v_err;
        VErr2 = switch_v_err;
        I_Integral = 0;
        i_limit_active = 0U;
        i_release_cnt = 0U;
        i_vref_offset = 0;
        i0 = 0;
        IErr0 = 0;
        IErr1 = 0;
        CVCC_Mode = CV;
        s_vout_pid_filter = VoutRaw;
        s_vout_pid_filter_valid = 1U;
        if (g_mode_switch_inject_valid != 0U){
            CtrValue.BuckDuty = g_mode_switch_buck_duty;
            CtrValue.BoostDuty = g_mode_switch_boost_duty;
            u0 = g_mode_switch_u_seed;
            u1 = g_mode_switch_u_seed;
            if (DF.BBFlag == Mix){
                s_VLoop_ClampOutputState(BSP_POWER_BOOST_DUTY_MIN_TICK, s_VLoop_GetMixBoostDutyMax());
                CtrValue.BoostDuty = (u0 >> 8) * 3;
            }
            g_mode_switch_inject_valid = 0U;
        }
        DF.BBModeChange = 0;
    }

    // 判断工作模式，BUCK，BOOST，BUCK-BOOST
    switch (DF.BBFlag){
        case NA:{// 初始阶段
            VErr0 = 0;
            VErr1 = 0;
            VErr2 = 0;
            u0 = 0;
            u1 = 0;
            i0 = 0;
            i_vref_offset = 0;
            I_Integral = 0;
            i_limit_active = 0U;
            i_release_cnt = 0U;
            IErr0 = 0;
            IErr1 = 0;
            break;
        }
        case Buck:{ // BUCK模式

            u0 = u1 + VErr0 * BUCKPIDb0 + VErr1 * BUCKPIDb1 + VErr2 * BUCKPIDb2; // 计算电压环输出
            s_VLoop_ClampOutputState(BSP_POWER_BUCK_DUTY_MIN_TICK, CtrValue.BUCKMaxDuty);
            // 历史数据幅值
            VErr2 = VErr1;
            VErr1 = VErr0;

            // 环路输出赋值
            CtrValue.BoostDuty = BSP_POWER_BOOST_DUTY_SYNC_MIN_TICK; // 继承82c431b3：Buck下保留Boost同步支路，保证降设定时可快速回落
            CtrValue.BuckDuty = (u0 >> 8) * 3;    // 电压环占空比输出

            // 环路输出最大最小占空比限制
            if (CtrValue.BuckDuty > CtrValue.BUCKMaxDuty)
                CtrValue.BuckDuty = CtrValue.BUCKMaxDuty;
            if (CtrValue.BuckDuty < BSP_POWER_BUCK_DUTY_MIN_TICK)
                CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
            break;
        }
        case Boost:{ // Boost模式
            int32_t boost_buck_target = BSP_POWER_BUCK_DUTY_MAX_TICK;
            // 调用PID环路计算公式（参照PID环路计算文档）
            u0 = u1 + VErr0 * BOOSTPIDb0 + VErr1 * BOOSTPIDb1 + VErr2 * BOOSTPIDb2;
            s_VLoop_ClampOutputState(BSP_POWER_BOOST_DUTY_MIN_TICK, CtrValue.BoostMaxDuty);
            // 历史数据幅值
            VErr2 = VErr1;
            VErr1 = VErr0;

            // 环路输出赋值
            // BOOST 模式下不要一步把 Buck 支路硬切到 94%，否则进入 Boost 临界区时
            // 容易出现和 MIX 类似的输入瞬时下拉。
            if (CtrValue.BuckDuty < boost_buck_target){
                CtrValue.BuckDuty = (int16_t)(CtrValue.BuckDuty + BSP_POWER_BUCK_DUTY_BOOST_STEP_TICK);
                if (CtrValue.BuckDuty > boost_buck_target){
                    CtrValue.BuckDuty = (int16_t)boost_buck_target;
                }
            }
            else if (CtrValue.BuckDuty > boost_buck_target){
                CtrValue.BuckDuty = (int16_t)(CtrValue.BuckDuty - BSP_POWER_BUCK_DUTY_BOOST_STEP_TICK);
                if (CtrValue.BuckDuty < boost_buck_target){
                    CtrValue.BuckDuty = (int16_t)boost_buck_target;
                }
            }

            if (CtrValue.BuckDuty > CtrValue.BUCKMaxDuty){
                CtrValue.BuckDuty = CtrValue.BUCKMaxDuty;
            }
            if (CtrValue.BuckDuty < BSP_POWER_BUCK_DUTY_MIN_TICK){
                CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
            }

            CtrValue.BoostDuty = (u0 >> 8) * 3; // 电压环占空比输出

            // 环路输出最大最小占空比限制
            if (CtrValue.BoostDuty > CtrValue.BoostMaxDuty)
                CtrValue.BoostDuty = CtrValue.BoostMaxDuty;
            if (CtrValue.BoostDuty < BSP_POWER_BOOST_DUTY_MIN_TICK)
                CtrValue.BoostDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
            break;
        }
        case Mix:{ // Mix模式
            int32_t mix_buck_target = BSP_POWER_BUCK_DUTY_SYNC_MAX_TICK;
            int16_t mix_boost_max_duty = s_VLoop_GetMixBoostDutyMax();
            // 调用PID环路计算公式
            u0 = u1 + VErr0 * MIXPIDb0 + VErr1 * MIXPIDb1 + VErr2 * MIXPIDb2;
            s_VLoop_ClampOutputState(BSP_POWER_BOOST_DUTY_MIN_TICK, mix_boost_max_duty);
            // 历史数据幅值
            VErr2 = VErr1;
            VErr1 = VErr0;
            IErr1 = IErr0;

            // 环路输出赋值
            // MIX 模式下不要一步把 Buck 支路硬切到 80%，否则在临界区容易把输入瞬时拉垮。
            // 这里从当前占空平滑逼近目标，同时仍受软启动占空上限约束。
            if (CtrValue.BuckDuty < mix_buck_target){
                CtrValue.BuckDuty = (int16_t)(CtrValue.BuckDuty + BSP_POWER_BUCK_DUTY_SYNC_STEP_TICK);
                if (CtrValue.BuckDuty > mix_buck_target){
                    CtrValue.BuckDuty = (int16_t)mix_buck_target;
                }
            }
            else if (CtrValue.BuckDuty > mix_buck_target){
                CtrValue.BuckDuty = (int16_t)(CtrValue.BuckDuty - BSP_POWER_BUCK_DUTY_SYNC_STEP_TICK);
                if (CtrValue.BuckDuty < mix_buck_target){
                    CtrValue.BuckDuty = (int16_t)mix_buck_target;
                }
            }

            if (CtrValue.BuckDuty > CtrValue.BUCKMaxDuty){
                CtrValue.BuckDuty = CtrValue.BUCKMaxDuty;
            }
            if (CtrValue.BuckDuty < BSP_POWER_BUCK_DUTY_MIN_TICK){
                CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
            }

            CtrValue.BoostDuty = (u0 >> 8) * 3; // 电压环占空比输出

            // 环路输出最大最小占空比限制
            if (CtrValue.BoostDuty > mix_boost_max_duty)
                CtrValue.BoostDuty = mix_boost_max_duty;
            if (CtrValue.BoostDuty < BSP_POWER_BOOST_DUTY_MIN_TICK)
                CtrValue.BoostDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
            break;
        }
    }

    // PWMENFlag是PWM开启标志位，当该位为0时,buck的占空比为0，无输出;
    if (DF.PWMENFlag == 0)
        CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;

    // 更新对应寄存器
    // buck占空比
    PowerControl_HRTIM_SetBuckCompareFast(BSP_POWER_HRTIM_PERIOD_TICK - CtrValue.BuckDuty);
    // ADC触发采样点：Timer A CMP3 触发 HRTIM_TRG1，可运行时调节以避开开关噪声。
    PowerControl_HRTIM_SetAdcTriggerCompareFast(g_adc_sample_tick);
    // Boost占空比
    PowerControl_HRTIM_SetBoostCompareFast(CtrValue.BoostDuty);
}
