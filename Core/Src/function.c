/**
  ******************************************************************************
  * @file    function.c
  * @author  UF4
  * @date    26-6-12 下午2:31
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


#include "function.h"
#include "adc.h"
#include "usart.h"
#include "tim.h"
#include "hrtim.h"
#include "W25Q64.h"
#include "temp.h"
#include <stdint.h>
#include <string.h>

volatile uint16_t ADC1_RESULT[4] = {0, 0, 0, 0};                   // ADC采样外设到内存的DMA数据保存寄存器

volatile float MAX_OTP_VAL;                                        // 过温保护阈值
volatile float MAX_VOUT_OVP_VAL;                                   // 输出过压保护阈值
volatile float MAX_VOUT_OCP_VAL;                                   // 输出过流保护阈值

struct _Ctr_value CtrValue =
    {
    0,
    0,
    0,
    0,
    BSP_POWER_BUCK_DUTY_MIN_TICK,
    0,
    0,
    0
}; // 控制参数

struct _FLAG DF =
    {
    0,
    0,
    0,
    0,
    0,
    0,
    0
    }; // 控制标志位

struct _ADI SADC =
    {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
    }; // 输入输出参数采样值和平均值

struct _SET_Value SET_Value =
    {
    0,
    0,
    0
    };   // 设置参数
SState_M STState = SSInit; // 软启动状态标志
volatile float VIN, VOUT, IIN, IOUT;                               // 电压电流实际值
volatile float Board1_TEMP, Board2_TEMP, CPU_TEMP;                 // 主板和CPU温度实际值
volatile float powerEfficiency = 0;                                // 电源转换效率
extern volatile int32_t VErr0, VErr1, VErr2; // 电压误差
extern volatile int32_t u0, u1;              // 电压环输出量
volatile uint8_t g_mode_switch_inject_valid = 0U;
volatile int16_t g_mode_switch_buck_duty = BSP_POWER_BUCK_DUTY_MIN_TICK;
volatile int16_t g_mode_switch_boost_duty = BSP_POWER_BOOST_DUTY_MIN_TICK;
volatile int32_t g_mode_switch_u_seed = 0;

#define BB_MODE_BUCK_TO_MIX_RATIO       0.95F
#define BB_MODE_MIX_TO_BUCK_RATIO       0.90F
#define BB_MODE_MIX_TO_BOOST_RATIO      1.10F
#define BB_MODE_BOOST_TO_MIX_RATIO      1.05F
#define BB_MODE_VIN_DROP_FILTER_SHIFT   3U

static int16_t s_PowerControl_ClampDutyTick(int32_t duty_tick, int16_t min_tick, int16_t max_tick)
{
    if (duty_tick < min_tick){
        return min_tick;
    }
    if (duty_tick > max_tick){
        return max_tick;
    }
    return (int16_t)duty_tick;
}

static int32_t s_PowerControl_DutyTickToLoopSeed(int16_t duty_tick)
{
    return (((int32_t)duty_tick / 3) << 8);
}

/**
 * @brief 依据目标模式预计算切模占空并注入到快环。
 * 按 TI 多模态控制思路，在切模前先算新模式所需 duty，
 * 让快环直接从新模式附近起步，而不是背着旧模式积分慢慢追。
 * @param target_mode 目标工作模式
 * @param vin_adc 当前输入电压 ADC 平均值
 * @param vout_adc 当前输出电压 ADC 校正值
 */
void PowerControl_PrepareModeSwitch(BB_M target_mode, uint32_t vin_adc, int32_t vout_adc)
{
    float vin_f;
    float vout_f;
    float duty_ratio = 0.0F;
    int16_t buck_duty = CtrValue.BuckDuty;
    int16_t boost_duty = CtrValue.BoostDuty;
    int32_t u_seed = 0;

    if (vin_adc == 0U || vout_adc == 0U){
        g_mode_switch_inject_valid = 0U;
        return;
    }

    vin_f = (float)vin_adc;
    vout_f = (float)vout_adc;

    switch (target_mode){
        case Buck:
            duty_ratio = vout_f / vin_f;  // TI Eq.2: D = Vout / Vin
            buck_duty = s_PowerControl_ClampDutyTick((int32_t)(duty_ratio * BSP_POWER_HRTIM_PERIOD_TICK + 0.5F),
                                                     BSP_POWER_BUCK_DUTY_MIN_TICK,
                                                     CtrValue.BUCKMaxDuty);
            boost_duty = BSP_POWER_BOOST_DUTY_SYNC_MIN_TICK;
            u_seed = s_PowerControl_DutyTickToLoopSeed(buck_duty);
            break;

        case Mix:
            // MIX 入口保持贴近当前硬件状态：Boost 支路和 u_seed 都从当前 BoostDuty 起步，
            // 后续由 MIX 独立补偿器接管，避免 Buck/Boost/PID 三个状态互相不一致。
            boost_duty = s_PowerControl_ClampDutyTick(CtrValue.BoostDuty,
                                                      BSP_POWER_BOOST_DUTY_MIN_TICK,
                                                      CtrValue.BoostMaxDuty);
            buck_duty = CtrValue.BuckDuty;
            u_seed = s_PowerControl_DutyTickToLoopSeed(boost_duty);
            break;

        case Boost:
            duty_ratio = (vout_f - vin_f) / vout_f; // TI Eq.4: D = (Vout - Vin) / Vout
            if (duty_ratio < 0.0F){
                duty_ratio = 0.0F;
            }
            boost_duty = s_PowerControl_ClampDutyTick((int32_t)(duty_ratio * BSP_POWER_HRTIM_PERIOD_TICK + 0.5F),
                                                      BSP_POWER_BOOST_DUTY_MIN_TICK,
                                                      CtrValue.BoostMaxDuty);
            buck_duty = CtrValue.BuckDuty;
            u_seed = s_PowerControl_DutyTickToLoopSeed(boost_duty);
            break;

        case NA:
        default:
            g_mode_switch_inject_valid = 0U;
            return;
    }

    g_mode_switch_buck_duty = buck_duty;
    g_mode_switch_boost_duty = boost_duty;
    g_mode_switch_u_seed = u_seed;
    g_mode_switch_inject_valid = 1U;
}

/**
 * @brief 对 ADC DMA 采样结果做滤波与平均。
 * 更新输入输出电压电流的原始值和滑动平均值，供控制与上报使用。
 */
RAMFUNC void ADCSample(void){
    static uint32_t VinAvgSum = 0, IinAvgSum = 0, VoutAvgSum = 0, IoutAvgSum = 0;

    // 从DMA缓冲器中获取数据
    SADC.Vin  = (uint32_t)((ADC1_RESULT[0] * CAL_VIN_K >> 12) + CAL_VIN_B);
    SADC.Iin  = (uint32_t)ADC1_RESULT[1];
    SADC.Vout = (uint32_t)((ADC1_RESULT[2] * CAL_VOUT_K >> 12) + CAL_VOUT_B);
    SADC.Iout = (uint32_t)((ADC1_RESULT[3] * CAL_IOUT_K >> 12) + CAL_IOUT_B);

    if (SADC.Vin < 2)  // 仅在接近零码时清零，避免低压输入被直接抹掉
        SADC.Vin = 0;
    if (SADC.Vout < 2)
        SADC.Vout = 0;
    if (SADC.Iout < 2)
        SADC.Iout = 0;

    // 滑动平均：新增一个采样值，同时减去之前的平均值（IIR 一阶低通，等效 8 点平均）
    VinAvgSum   = VinAvgSum   + SADC.Vin  - (VinAvgSum   >> 3);
    SADC.VinAvg = VinAvgSum   >> 3;
    IinAvgSum   = IinAvgSum   + SADC.Iin  - (IinAvgSum   >> 3);
    SADC.IinAvg = IinAvgSum   >> 3;
    VoutAvgSum  = VoutAvgSum  + SADC.Vout - (VoutAvgSum  >> 3);
    SADC.VoutAvg = VoutAvgSum >> 3;
    IoutAvgSum  = IoutAvgSum  + SADC.Iout - (IoutAvgSum  >> 3);
    SADC.IoutAvg = IoutAvgSum >> 3;
}

/**
 * @brief ADC数据计算转换成实际数值的浮点数
 *
 */
void ADC_calculate(void){
    VIN  = SADC.VinAvg  * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VIN_SENSE_SCALE;   // 计算ADC1通道0输入电压采样结果
    IIN  = (BSP_POWER_CURRENT_BIAS_V - SADC.IinAvg * REF_3V3 / ADC_MAX_VALUE) / BSP_POWER_CURRENT_SENSE_V_PER_A;

    VOUT = SADC.VoutAvg * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VOUT_SENSE_SCALE;  // 计算ADC1通道2输出电压采样结果
    IOUT = (BSP_POWER_CURRENT_BIAS_V - SADC.IoutAvg * REF_3V3 / ADC_MAX_VALUE) / BSP_POWER_CURRENT_SENSE_V_PER_A;

    if (IIN < BSP_POWER_IIN_ZERO_DEADBAND_A)
        IIN = 0.0F;
    if (IOUT < BSP_POWER_IOUT_ZERO_DEADBAND_A)
        IOUT = 0.0F;

    Board1_TEMP = GET_NTC1_Temperature();  // 获取NTC1温度
    Board2_TEMP = GET_NTC2_Temperature();  // 获取NTC2温度
    CPU_TEMP = GET_CPU_Temperature();      // 获取单片机CPU温度
}

/**
 * @brief 执行一次主状态机调度。
 * 在 5ms 节拍中根据当前状态分发到对应的状态处理函数。
 */
RAMFUNC void StateM(void){
    // 判断状态类型
    switch (DF.SMFlag){
        // 初始化状态
        case Init:
            StateMInit();
        break;
        // 等待状态
        case Wait:
            StateMWait();
        break;
        // 软启动状态
        case Rise:
            StateMRise();
        break;
        // 运行状态
        case Run:
            StateMRun();
        break;
        // 故障状态
        case Err:
            StateMErr();
        break;
    }
}

/**
 * @brief 处理初始化状态。
 * 完成参数初始化后切换到等待状态。
 */
void StateMInit(void){
    DF.SMFlag = Wait;    // 状态机跳转至等待软启状态
}
/**
 * @brief 初始化默认参数与保护阈值。
 * 在上电和状态机复位时恢复控制上下文到安全默认值。
 */
void ValInit(void){
    DF.PWMENFlag = 0;    // 关闭PWM

    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出

    DF.BBFlag = NA;
    DF.ErrFlag = 0;    // 清除故障标志位
    CtrValue.Vout_ref = 0;    // 初始化电压参考量
    // 限制占空比
    CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
    CtrValue.BUCKMaxDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
    CtrValue.BoostDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
    CtrValue.BoostMaxDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
    // 环路计算变量初始化
    VErr0 = 0;
    VErr1 = 0;
    VErr2 = 0;
    u0 = 0;
    u1 = 0;
    // 设置值初始化
    SET_Value.Vout   = POWER_CTRL_DEFAULT_SET_VOLTAGE;
    SET_Value.Iout   = POWER_CTRL_DEFAULT_SET_CURRENT;
    MAX_OTP_VAL      = POWER_CTRL_DEFAULT_OTP_SET;      // 过温保护阈值
    MAX_VOUT_OVP_VAL = POWER_CTRL_DEFAULT_OVP_SET;      // 输出过压保护阈值
    MAX_VOUT_OCP_VAL = POWER_CTRL_DEFAULT_OCP_SET;      // 输出过流保护阈值
}
/**
 * @brief 运行态占位处理函数。
 * 当前主要闭环控制在快环中断中执行，此处保留运行态扩展入口。
 */
void StateMRun(void){}

/**
 * @brief 关闭输出并复位控制上下文。
 * 在上级关闭输出或进入等待态时清空模式与参考值，避免下次启动沿用旧状态。
 */
void PowerControl_DisableOutput(void){
    DF.PWMENFlag = 0;
    DF.BBFlag = NA;
    DF.BBModeChange = 0;
    CVCC_Mode = CV;

    CtrValue.Vout_ref = 0;
    CtrValue.Vout_SSref = 0;
    CtrValue.Vout_SETref = 0;
    CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
    CtrValue.BUCKMaxDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
    CtrValue.BoostDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
    CtrValue.BoostMaxDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;

    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2);

    __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, BSP_POWER_HRTIM_PERIOD_TICK);
    __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, BSP_POWER_HRTIM_PERIOD_TICK >> 1);
    __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1, BSP_POWER_BOOST_DUTY_MIN_TICK);
}

/**
 * @brief 处理故障状态。
 * 关闭 PWM 输出，并在故障清除后允许状态机回到等待态。
 */
void StateMErr(void)
{
    PowerControl_DisableOutput();
    if (DF.ErrFlag == F_NOERR)
    {    // 若故障消除跳转至等待重新软启
        DF.SMFlag = Wait;
    }
}
/**
 * @brief 处理等待状态。
 * 等待输出使能与无故障条件满足后进入软启动流程。
 */
void StateMWait(void){
    static uint16_t CntS = 0;  // 计数器定义
    PowerControl_DisableOutput();
    CntS++; // 计数器累加
    if (CntS > 200){    // 等待1S，进入启动状态
        CntS = 200;
        if (DF.ErrFlag == F_NOERR && DF.OUTPUT_Flag == 1)
        {
            CntS = 0;            // 计数器清0
            DF.SMFlag = Rise;    // 状态标志位跳转至等待状态
            STState = SSInit;    // 软启动子状态跳转至初始化状态
        }
    }
}
/**
 * @brief 处理软启动阶段。
 * 分阶段建立参考值和占空边界，降低启动时的电压电流冲击。
 */
void StateMRise(void){
    static uint16_t Cnt = 0;     // 计时器
    switch (STState){            // 判断软启状态
    case SSInit: {               // 初始化状态
        DF.PWMENFlag = 0;        // 关闭PWM
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
        // 软启中将运行限制占空比启动，从最小占空比开始启动
        CtrValue.BUCKMaxDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
        CtrValue.BoostMaxDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
        // 环路计算变量初始化
        VErr0 = 0;
        VErr1 = 0;
        VErr2 = 0;
        u0 = 0;
        u1 = 0;

        // 将设置值传到参考值
        CtrValue.Vout_SETref = (int32_t)((SET_Value.Vout / BSP_POWER_VOUT_SENSE_SCALE) / REF_3V3 * ADC_MAX_VALUE);
        CtrValue.Iout_ref    = (int32_t)(((BSP_POWER_CURRENT_BIAS_V - SET_Value.Iout * BSP_POWER_CURRENT_SENSE_V_PER_A) / REF_3V3) * ADC_MAX_VALUE);

        // 将钳位锁定在有效的ADC范围
        if (CtrValue.Iout_ref > (int32_t)ADC_MAX_VALUE) CtrValue.Iout_ref = (int32_t)ADC_MAX_VALUE;
        if (CtrValue.Iout_ref < 0) CtrValue.Iout_ref = 0;
        // 跳转至软启等待状态
        STState = SSWait;

        break;
    }
    // 等待软启动状态
    case SSWait:{
        Cnt++;  // 计数器累加
        if (Cnt > 5){  // 等待25ms
            Cnt = 0;   // 计数器清0
            // 限制启动占空比
            CtrValue.BuckDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
            CtrValue.BUCKMaxDuty = BSP_POWER_BUCK_DUTY_MIN_TICK;
            CtrValue.BoostDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
            CtrValue.BoostMaxDuty = BSP_POWER_BOOST_DUTY_MIN_TICK;
            // 环路计算变量初始化
            VErr0 = 0;
            VErr1 = 0;
            VErr2 = 0;
            u0 = 0;
            u1 = 0;
            CtrValue.Vout_SSref = CtrValue.Vout_SETref >> 1; // 输出参考电压从一半开始启动，避免过冲，然后缓慢上升
            STState = SSRun;                                 // 跳转至软启状态
        }
        break;
    }
    // 软启动状态
    case SSRun:{
        static uint16_t BUCKMaxDutyCnt = 0, BoostMaxDutyCnt = 0;
        if (DF.PWMENFlag == 0){// 正式发波前环路变量清0
            // 环路计算变量初始化
            VErr0 = 0;
            VErr1 = 0;
            VErr2 = 0;
            u0 = 0;
            u1 = 0;
            __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, BSP_POWER_HRTIM_PERIOD_TICK); // BUCK电路下管占空比拉满
            __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1, BSP_POWER_HRTIM_PERIOD_TICK); // BOOST电路下管占空比拉满
            HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2);           // 开启HRTIM的PWM输出
            HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);           // 开启HRTIM的PWM输出
        } // 发波标志位置位
        DF.PWMENFlag = 1; // 最大占空比限制逐渐增加
        BUCKMaxDutyCnt++;
        BoostMaxDutyCnt++; // 最大占空比限制累加
        CtrValue.BUCKMaxDuty = CtrValue.BUCKMaxDuty + BUCKMaxDutyCnt * 15;
        CtrValue.BoostMaxDuty = CtrValue.BoostMaxDuty + BoostMaxDutyCnt * 15;
        // 累加到最大值
        if (CtrValue.BUCKMaxDuty > BSP_POWER_BUCK_DUTY_MAX_TICK)
            CtrValue.BUCKMaxDuty = BSP_POWER_BUCK_DUTY_MAX_TICK;
        if (CtrValue.BoostMaxDuty > BSP_POWER_BOOST_DUTY_MAX_TICK)
            CtrValue.BoostMaxDuty = BSP_POWER_BOOST_DUTY_MAX_TICK;

        if (CtrValue.BUCKMaxDuty == BSP_POWER_BUCK_DUTY_MAX_TICK && CtrValue.BoostMaxDuty == BSP_POWER_BOOST_DUTY_MAX_TICK){
            DF.SMFlag = Run; // 状态机跳转至运行状态
            STState = SSInit;// 软启动子状态跳转至初始化状态
        }
        break;
    }
    default:
        break;
    }
}
/**
 * @brief 处理输出短路保护与自动重试。
 * 短路成立时立即关断输出，并在限定次数内按延时策略尝试恢复。
 */
void ShortOff(void){
    static int32_t RSCnt = 0;
    static uint8_t RSNum = 0;
    float Vout = SADC.Vout * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VOUT_SENSE_SCALE;
    float Iout = (BSP_POWER_CURRENT_BIAS_V - SADC.Iout * REF_3V3 / ADC_MAX_VALUE) / BSP_POWER_CURRENT_SENSE_V_PER_A;
    // 当输出电流大于 *A，且电压小于*V时，可判定为发生短路保护
    if (Iout > POWER_CTRL_SHORT_CURRENT && Vout < POWER_CTRL_SHORT_VOLTAGE){
        DF.PWMENFlag = 0; // 关闭PWM

        HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 开启HRTIM的PWM输出
        HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 开启HRTIM的PWM输出

        setRegBits(DF.ErrFlag, F_SW_SHORT);        // 故障标志位
        DF.SMFlag = Err;        // 跳转至故障状态
    }
    // 输出短路保护恢复
    // 当发生输出短路保护，关机后等待4S后清楚故障信息，进入等待状态等待重启
    if (getRegBits(DF.ErrFlag, F_SW_SHORT)){

        RSCnt++;// 等待故障清楚计数器累加
        // 等待2S
        if (RSCnt > 400){    // 计数器清零
            RSCnt = 0;       // 短路重启只重启10次，10次后不重启
            if (RSNum > 10){ // 确保不清除故障，不重启
                RSNum = 11;
                DF.PWMENFlag = 0;   // 关闭PWM
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 开启HRTIM的PWM输出
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 开启HRTIM的PWM输出
            }
            else{
                RSNum++;    // 短路重启计数器累加
                clrRegBits(DF.ErrFlag, F_SW_SHORT);  // 清除过流保护故障标志位
            }
        }
    }
}

/**
 * @brief OVP 输出过压保护函数
 * OVP 函数用于处理输出电压过高的情况。
 * 函数需放5ms中断里执行。
 */
void OVP(void){
    static uint16_t OVPCnt = 0; // 过压保护判据保持计数器定义

    float Vout = SADC.Vout * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VOUT_SENSE_SCALE;
    // 当输出电压大于50V，且保持10ms
    if (Vout >= MAX_VOUT_OVP_VAL){
        OVPCnt++;// 条件保持计时
        if (OVPCnt > 2){ // 条件保持10ms
            OVPCnt = 0;       // 计时器清零
            DF.PWMENFlag = 0; // 关闭PWM
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
            setRegBits(DF.ErrFlag, F_SW_VOUT_OVP);// 故障标志位
            DF.SMFlag = Err;// 跳转至故障状态
        }
    }
    else
        OVPCnt = 0;
}

/**
 * @brief OCP 输出过流保护函数
 * OCP 函数用于处理输出电流过高的情况。
 * 函数需放5ms中断里执行。
 */
void OCP(void){
    static uint16_t OCPCnt = 0;    // 过流保护判据保持计数器定义
    static uint16_t RSCnt = 0;    // 故障清楚保持计数器定义
    static uint16_t RSNum = 0;    // 保留保护重启计数器

    float Iout = (BSP_POWER_CURRENT_BIAS_V - SADC.Iout * REF_3V3 / ADC_MAX_VALUE) / BSP_POWER_CURRENT_SENSE_V_PER_A;

    // 当输出电流大于*A，且保持50ms
    if (Iout >= MAX_VOUT_OCP_VAL && DF.SMFlag == Run){
        OCPCnt++; // 条件保持计时
        if (OCPCnt > 10){ // 条件保持50ms，则认为过流发生
            OCPCnt = 0;// 计数器清0
            DF.PWMENFlag = 0; // 关闭PWM
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
            setRegBits(DF.ErrFlag, F_SW_IOUT_OCP); // 故障标志位
            DF.SMFlag = Err;  // 跳转至故障状态
        }
    }
    else
        // 计数器清0
        OCPCnt = 0;

    // 输出过流后恢复
    // 当发生输出软件过流保护，关机后等待4S后清除故障信息，进入等待状态等待重启
    if (getRegBits(DF.ErrFlag, F_SW_IOUT_OCP)){
        RSCnt++; // 等待故障清除计数器累加
        // 等待2S
        if (RSCnt > 400){
            RSCnt = 0;  // 计数器清零
            RSNum++;    // 过流重启计数器累加

            if (RSNum > 10){// 过流重启只重启10次，10次后不重启（严重故障）
                RSNum = 11;// 确保不清除故障，不重启
                DF.PWMENFlag = 0;// 关闭PWM
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
            }
            else{
                // 清除过流保护故障标志位
                clrRegBits(DF.ErrFlag, F_SW_IOUT_OCP);
            }
        }
    }
}

/**
 * @brief OTP 过温保护函数
 * OTP 函数用于处理温度过高的情况。
 * 函数需放5ms中断里执行。
 */
void OTP(void){
    const float TEMP1 = GET_NTC1_Temperature(); // 获取NTC1温度值
    const float TEMP2 = GET_NTC2_Temperature(); // 获取NTC2温度值
    if (TEMP1 >= MAX_OTP_VAL || TEMP2 >= MAX_OTP_VAL){

        DF.SMFlag = Wait;
        DF.PWMENFlag = 0;      // 关闭PWM
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
        setRegBits(DF.ErrFlag, F_OTP);                                               // 故障标志位
        DF.SMFlag = Err;                                                             // 跳转至故障状态
        FAN_PWM_set(POWER_CTRL_FAN_MAX_RUN_DUTY);                          // 风扇散热
    }
}

/**
 * @brief 运行模式判断。
 * BUCK模式：实际输出电压低于输入电压，Buck 仍有足够调节余量
 * BOOST模式：实际输出电压明显高于输入电压
 * MIX模式：实际输出电压接近输入电压，用回差防止在临界点来回振荡
 */
RAMFUNC void BBMode(void){
    uint8_t PreBBFlag = 0;// 上一次模式状态量
    PreBBFlag = DF.BBFlag;// 暂存当前的模式状态量

    uint32_t VIN_ADC = ADC1_RESULT[0]; // 输入电压ADC采样值
    uint32_t VOUT_ADC = (uint32_t)((ADC1_RESULT[2] * CAL_VOUT_K >> 12) + CAL_VOUT_B); // 输出电压ADC校正值
    static uint32_t VIN_MODE_REF = 0;

    // 对输入电压ADC采样值累计取平均值
    static uint32_t VIN_ADC_SUM = 0;
    static uint8_t VIN_ADC_Count = 0;

    if (VIN_ADC_Count < 5){
        VIN_ADC_SUM += ADC1_RESULT[0];
        VIN_ADC_Count++;
    }
    if (VIN_ADC_Count == 5){
        VIN_ADC = VIN_ADC_SUM / 5;
        VIN_ADC_SUM = 0;
        VIN_ADC_Count = 0;
    }

    if (VIN_MODE_REF == 0U){
        VIN_MODE_REF = VIN_ADC;
    }
    else if (VIN_ADC > VIN_MODE_REF){
        VIN_MODE_REF = VIN_ADC;
    }
    else{
        VIN_MODE_REF -= (VIN_MODE_REF - VIN_ADC) >> BB_MODE_VIN_DROP_FILTER_SHIFT;
    }

    if (DF.OUTPUT_Flag == 0U || DF.PWMENFlag == 0U || CtrValue.Vout_ref <= 0){
        DF.BBFlag = Buck;
        DF.BBModeChange = 0;
        VIN_MODE_REF = VIN_ADC;
        return;
    }

    // 判断当前模块的工作模式
    switch (DF.BBFlag){
        // NA-初始化模式
        case NA:{
            if (VOUT_ADC < VIN_MODE_REF * BB_MODE_MIX_TO_BUCK_RATIO)
                DF.BBFlag = Buck;                          // 切换到buck模式
            else if (VOUT_ADC > VIN_MODE_REF * BB_MODE_MIX_TO_BOOST_RATIO)
                DF.BBFlag = Boost;                         // 切换到boost模式
            else
                DF.BBFlag = Mix; // buck-boost（MIX） mode
            break;
        }
        // BUCK模式
        case Buck:{
            if (VOUT_ADC > VIN_MODE_REF * BB_MODE_MIX_TO_BOOST_RATIO)
                DF.BBFlag = Boost;                          // boost mode
            else if (VOUT_ADC > VIN_MODE_REF * BB_MODE_BUCK_TO_MIX_RATIO)
                DF.BBFlag = Mix;                            // buck-boost（MIX） mode
            break;
        }
        // Boost模式
        case Boost:{
            if (VOUT_ADC < VIN_MODE_REF * BB_MODE_MIX_TO_BUCK_RATIO)
                DF.BBFlag = Buck;                           // buck mode
            else if (VOUT_ADC < VIN_MODE_REF * BB_MODE_BOOST_TO_MIX_RATIO)
                DF.BBFlag = Mix;                            // buck-boost（MIX） mode
            break;
        }
        // Mix模式
        case Mix:{
            if (VOUT_ADC < VIN_MODE_REF * BB_MODE_MIX_TO_BUCK_RATIO)
                DF.BBFlag = Buck;                          // buck mode
            else if (VOUT_ADC > VIN_MODE_REF * BB_MODE_MIX_TO_BOOST_RATIO)
                DF.BBFlag = Boost;                         // boost mode
            break;
        }
    }

    // 当模式发生变换时（上一次和这一次不一样）,则标志位置位，标志位用以环路计算复位，保证模式切换过程不会有大的过冲
    if (PreBBFlag == DF.BBFlag)
        DF.BBModeChange = 0;
    else{
        PowerControl_PrepareModeSwitch((BB_M)DF.BBFlag, VIN_ADC, (int32_t)VOUT_ADC);
        DF.BBModeChange = 1;
    }
}

/**
 * @brief 设置风扇 PWM 值,
 * 根据给定的 PWM 值，设置风扇的 PWM 输出。
 * @param dutyCycle PWM 值，范围在 0 到 100 之间
 */
void FAN_PWM_set(uint16_t dutyCycle){
    static uint8_t fan_active = 0U;
    static uint32_t fan_kick_until = 0U;
    uint32_t tick_now = HAL_GetTick();
    uint32_t compare_value;

    if (dutyCycle == 0U){
        fan_active = 0U;
        fan_kick_until = 0U;
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, 0U);
        return;
    }

    if (dutyCycle < POWER_CTRL_FAN_MIN_RUN_DUTY){
        dutyCycle = POWER_CTRL_FAN_MIN_RUN_DUTY;
    }
    if (dutyCycle > POWER_CTRL_FAN_MAX_RUN_DUTY){
        dutyCycle = POWER_CTRL_FAN_MAX_RUN_DUTY;
    }

    if (fan_active == 0U){
        fan_active = 1U;
        fan_kick_until = tick_now + POWER_CTRL_FAN_STARTUP_KICK_MS;
    }

    if ((int32_t)(fan_kick_until - tick_now) > 0){
        compare_value = POWER_CTRL_FAN_STARTUP_KICK_DUTY * 10U;
        if (compare_value > htim8.Init.Period){
            compare_value = htim8.Init.Period;
        }
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, compare_value);
        return;
    }

    compare_value = dutyCycle * 10U;
    if (compare_value > htim8.Init.Period){
        compare_value = htim8.Init.Period;
    }
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, compare_value);
}

/**
 * @brief 初始化Flash。
 * 检查Flash中存储的数据是否有效，如果不有效则初始化。
 */
void Init_Flash(void)
{
    uint8_t Flash_flag[1];
    W25Q64_ReadData(0x000000, Flash_flag, 1); // 读取Flash中0x000000地址处的数据，这个地址存储标志位，0x00表示已经有数据
    if (Flash_flag[0] != 0x00){                // 如果读取的数据不为0，说明FLash中没有存储数据，需要初始化
        W25Q64_SectorErase(0x000000); // 擦除0x000000地址处的扇区
        uint8_t Flash_data[21];
        uint8_t VSETtemp[4], ISETtemp[4], OTPtemp[4], OCPtemp[4], OVPtemp[4];
        Flash_data[0] = 0x00;                     // 设置标志位
        float_to_bytes(SET_Value.Vout, VSETtemp); // 将浮点数转换为字节序列
        float_to_bytes(SET_Value.Iout, ISETtemp); // 将浮点数转换为字节序列
        float_to_bytes(MAX_OTP_VAL, OTPtemp);
        float_to_bytes(MAX_VOUT_OCP_VAL, OCPtemp);
        float_to_bytes(MAX_VOUT_OVP_VAL, OVPtemp);
        for (uint8_t i = 0; i < 4; i++){
            Flash_data[i + 1] = VSETtemp[i];
            Flash_data[i + 5] = ISETtemp[i];
            Flash_data[i + 9] = OTPtemp[i];
            Flash_data[i + 13] = OCPtemp[i];
            Flash_data[i + 17] = OVPtemp[i];
        }
        W25Q64_PageProgram(0x000000, Flash_data, 21); // 将设置数据写入Flash中
    }
}

/**
 * @brief 读取Flash中存储的数据。
 * 将设置信息从Flash中读取出来。
 */
void Read_Flash(void){
    uint8_t Flash_data[20];
    W25Q64_ReadData(0x000001, Flash_data, 20); // 读取Flash中0x000001地址处开始的8字节数据
    uint8_t VSETtemp[4], ISETtemp[4], OTPtemp[4], OCPtemp[4], OVPtemp[4];
    for (uint8_t i = 0; i < 4; i++){
        VSETtemp[i] = Flash_data[i];
        ISETtemp[i] = Flash_data[i + 4];
        OTPtemp[i] = Flash_data[i + 8];
        OCPtemp[i] = Flash_data[i + 12];
        OVPtemp[i] = Flash_data[i + 16];
    }

    /* Flash 首次使用或扇区被擦除时读出全 0xFF/0x00，bytes_to_float 会得到 NaN 或 0，
       直接覆盖默认值会导致保护阈值变成 0、保护立即误触发。
       这里对每个值做合理性校验，非法时保留 ValInit() 设的默认值。 */
    float fv = bytes_to_float(VSETtemp);
    float fi = bytes_to_float(ISETtemp);
    float fotp = bytes_to_float(OTPtemp);
    float focp = bytes_to_float(OCPtemp);
    float fovp = bytes_to_float(OVPtemp);

    if (fv == fv && fv > 0.0F && fv <= 100.0F){
        SET_Value.Vout = fv;
    }
    if (fi == fi && fi >= 0.0F && fi <= 50.0F){
        SET_Value.Iout = fi;
    }
    /* OTP：温度阈值合理范围 10~150℃；空 Flash 读出 0 或 NaN 时保持默认 80℃ */
    if (fotp == fotp && fotp >= 10.0F && fotp <= 150.0F){
        MAX_OTP_VAL = fotp;
    }
    /* OCP：电流阈值合理范围 0.1~50A */
    if (focp == focp && focp >= 0.1F && focp <= 50.0F){
        MAX_VOUT_OCP_VAL = focp;
    }
    /* OVP：电压阈值合理范围 1~100V；空 Flash 读出 0 时保持默认 45V，
       否则 Vout>=0 永远成立、上电即误触发过压保护 */
    if (fovp == fovp && fovp >= 1.0F && fovp <= 100.0F){
        MAX_VOUT_OVP_VAL = fovp;
    }
}

/**
 * @brief 更新Flash中存储的数据。
 * 将设置信息存储到Flash中。
 */
void Update_Flash(void){
    if (SET_Value.SET_modified_flag == 1){
        W25Q64_SectorErase(0x000000); // 擦除0x000000地址处的扇区
        uint8_t Flash_data[21];
        uint8_t VSETtemp[4], ISETtemp[4], OTPtemp[4], OCPtemp[4], OVPtemp[4];
        Flash_data[0] = 0x00;                     // 设置标志位
        float_to_bytes(SET_Value.Vout, VSETtemp); // 将浮点数转换为字节序列
        float_to_bytes(SET_Value.Iout, ISETtemp); // 将浮点数转换为字节序列
        float_to_bytes(MAX_OTP_VAL, OTPtemp);
        float_to_bytes(MAX_VOUT_OCP_VAL, OCPtemp);
        float_to_bytes(MAX_VOUT_OVP_VAL, OVPtemp);
        for (uint8_t i = 0; i < 4; i++){
            Flash_data[i + 1] = VSETtemp[i];
            Flash_data[i + 5] = ISETtemp[i];
            Flash_data[i + 9] = OTPtemp[i];
            Flash_data[i + 13] = OCPtemp[i];
            Flash_data[i + 17] = OVPtemp[i];
        }
        W25Q64_PageProgram(0x000000, Flash_data, 21); // 将设置数据写入Flash中
        SET_Value.SET_modified_flag = 0;
    }
}

/**
 * 将浮点数转换为字节序列的辅助函数。
 * 该函数将一个浮点数转换为其对应的字节序列，通过内存拷贝的方式将浮点数的二进制表示复制到指定的字节数组中。
 * @param value 需要转换为字节序列的浮点数。
 * @param bytes 指向接收浮点数字节序列的字节数组的指针。
 * @note 该函数依赖于特定平台的浮点数和整数类型大小以及内存对齐规则。
 */
void float_to_bytes(float value, uint8_t *bytes){
    memcpy(bytes, &value, sizeof(float)); // 将浮点数转换为字节序列
}

/**
 * 将字节序列转换为浮点数的辅助函数
 * @param bytes 指向包含浮点数的字节序列的指针
 * @return 转换后的浮点数值
 */
float bytes_to_float(uint8_t *bytes){
    float value;
    memcpy(&value, bytes, sizeof(float));
    return value;
}


