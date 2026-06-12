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
#include <math.h>
#include "adc.h"
#include "usart.h"
#include "tim.h"
#include "hrtim.h"
#include "fmac.h"
#include "W25Q64.h"
#include "temp.h"
#include <stdint.h>
#include <string.h>

// 数字后面加F表示使用单精度浮点数类型，C语言默认使用双精度浮点数类型，硬件浮点运算只支持单精度浮点数

volatile uint16_t ADC1_RESULT[4] = {0, 0, 0, 0};                   // ADC采样外设到内存的DMA数据保存寄存器
volatile uint8_t Encoder_Flag = 0;                                 // 编码器中断标志位
volatile uint8_t BUZZER_Short_Flag = 0;                            // 蜂鸣器短叫触发标志位
volatile uint8_t BUZZER_Middle_Flag = 0;                           // 蜂鸣器中等时间长度鸣叫触发标志位
volatile uint8_t BUZZER_Flag = 0;                                  // 蜂鸣器当前状态标志位
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
    0,
    0,
    0
    };   // 设置参数

SState_M STState = SSInit;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      // 软启动状态标志位

volatile float VIN, VOUT, IIN, IOUT;                               // 电压电流实际值
volatile float Board1_TEMP, Board2_TEMP, CPU_TEMP;                 // 主板和CPU温度实际值
volatile float powerEfficiency = 0;                                // 电源转换效率

extern volatile int32_t VErr0, VErr1, VErr2; // 电压误差
extern volatile int32_t u0, u1;              // 电压环输出量


CCRAM void ADCSample(void){
    // 从DMA缓冲器中获取数据
    SADC.Vin  = (uint32_t)ADC1_RESULT[0];
    SADC.Iin  = (uint32_t)ADC1_RESULT[1];
    SADC.Vout = (uint32_t)((ADC1_RESULT[2] * CAL_VOUT_K >> 12) + CAL_VOUT_B);
    SADC.Iout = (uint32_t)((ADC1_RESULT[3] * CAL_IOUT_K >> 12) + CAL_IOUT_B);

    if (SADC.Vin < 15) // 采样有零偏离，采样值很小时，直接为0
        SADC.Vin = 0;
    if (SADC.Vout < 15)
        SADC.Vout = 0;
    if (SADC.Iout < 16)
        SADC.Iout = 0;

    // 当前迁移到 FMAC FIR 做 8 tap 均值滤波。
    // 这样保留了和旧软件滑动平滑接近的低通效果，但后续可继续切到更严格的硬件数据流。
    SADC.VinAvg = POWER_FMAC_FilterVin((uint16_t)SADC.Vin);
    SADC.IinAvg = POWER_FMAC_FilterIin((uint16_t)SADC.Iin);
    SADC.VoutAvg = POWER_FMAC_FilterVout((uint16_t)SADC.Vout);
    SADC.IoutAvg = POWER_FMAC_FilterIout((uint16_t)SADC.Iout);
}

/**
 * @brief ADC数据计算转换成实际数值的浮点数
 *
 */
void ADC_calculate(void){
    VIN  = SADC.VinAvg  * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VIN_SENSE_SCALE;   // 计算ADC1通道0输入电压采样结果
    IIN  = (SADC.IinAvg * REF_3V3 / ADC_MAX_VALUE - BSP_POWER_CURRENT_BIAS_V) / BSP_POWER_CURRENT_SENSE_V_PER_A;

    VOUT = SADC.VoutAvg * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VOUT_SENSE_SCALE;  // 计算ADC1通道2输出电压采样结果
    IOUT = (SADC.IoutAvg * REF_3V3 / ADC_MAX_VALUE - BSP_POWER_CURRENT_BIAS_V) / BSP_POWER_CURRENT_SENSE_V_PER_A;

    Board1_TEMP = GET_NTC1_Temperature();  // 获取NTC1温度
    Board2_TEMP = GET_NTC2_Temperature();  // 获取NTC2温度
    CPU_TEMP = GET_CPU_Temperature();      // 获取单片机CPU温度
}


/*
 * @brief 状态机函数，在5ms中断中运行，5ms运行一次
 */
CCRAM void StateM(void){
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

        default:
            StateMErr();
    }
}

/*
 * @brief 初始化状态函数，参数初始化
 */
void StateMInit(void){
    ValInit();    // 相关参数初始化
    DF.SMFlag = Wait;    // 状态机跳转至等待软启状态
}



/*
 * @brief 相关参数初始化函数
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
/*
 * @brief 正常运行，主处理函数在中断中运行
 */
void StateMRun(void){}

/*
 * @brief 故障状态机
 */
void StateMErr(void){
    // 关闭PWM
    DF.PWMENFlag = 0;
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
    LED_R_ON;LED_G_OFF;LED_Y_OFF;
    DF.BBFlag = NA;                                                              // 切换运行模式
    // 若故障消除跳转至等待重新软启
    if (DF.ErrFlag == F_NOERR){
        DF.SMFlag = Wait;
        LED_R_OFF;LED_G_ON;LED_Y_OFF;
    }
}
/*
 * @brief 等待状态机
 */
void StateMWait(void){
    // 计数器定义
    static uint16_t CntS = 0;
    static uint32_t IinSum = 0, IoutSum = 0;
    DF.PWMENFlag = 0;    // 关PWM
    CntS++; // 计数器累加
    if (CntS > 200){    // 等待1S，进入启动状态
        CntS = 200;
        if (DF.ErrFlag == F_NOERR && DF.OUTPUT_Flag == 1){
            CntS = 0;            // 计数器清0
            IinSum = 0;
            IoutSum = 0;
            DF.SMFlag = Rise;            // 状态标志位跳转至等待状态
            STState = SSInit;            // 软启动子状态跳转至初始化状态
        }
    }
}


/*
 * @brief 软启动阶段
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
        CtrValue.Iout_ref    = (int32_t)(((BSP_POWER_CURRENT_BIAS_V + SET_Value.Iout * BSP_POWER_CURRENT_SENSE_V_PER_A) / REF_3V3) * ADC_MAX_VALUE);

        // Clamp to valid ADC range
        // if (CtrValue.Vout_SETref > (int32_t)ADC_MAX_VALUE) CtrValue.Vout_SETref = (int32_t)ADC_MAX_VALUE;
        // if (CtrValue.Vout_SETref < 0) CtrValue.Vout_SETref = 0;
        // if (CtrValue.Iout_ref > (int32_t)ADC_MAX_VALUE) CtrValue.Iout_ref = (int32_t)ADC_MAX_VALUE;
        // if (CtrValue.Iout_ref < 0) CtrValue.Iout_ref = 0;
        // 跳转至软启等待状态
        STState = SSWait;

        break;
    }
    // 等待软启动状态
    case SSWait:{
        // 计数器累加
        Cnt++;
        // 等待25ms
        if (Cnt > 5){
            // 计数器清0
            Cnt = 0;
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


void ShortOff(void){
    static int32_t RSCnt = 0;
    static uint8_t RSNum = 0;
    float Vout = SADC.Vout * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VOUT_SENSE_SCALE;
    float Iout = (SADC.Iout * REF_3V3 / ADC_MAX_VALUE - BSP_POWER_CURRENT_BIAS_V) / BSP_POWER_CURRENT_SENSE_V_PER_A;
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

    float Iout = (SADC.Iout * REF_3V3 / ADC_MAX_VALUE - BSP_POWER_CURRENT_BIAS_V) / BSP_POWER_CURRENT_SENSE_V_PER_A;

    // 当输出电流大于*A，且保持50ms
    if (Iout >= MAX_VOUT_OCP_VAL && DF.SMFlag == Run){
        OCPCnt++; // 条件保持计时
        if (OCPCnt > 10){ // 条件保持50ms，则认为过流发生
            OCPCnt = 0;// 计数器清0
            DF.PWMENFlag = 0; // 关闭PWM
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BUCK电路的PWM输出
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TF1 | HRTIM_OUTPUT_TF2); // 关闭BOOST电路的PWM输出
            // 故障标志位
            setRegBits(DF.ErrFlag, F_SW_IOUT_OCP);
            // 跳转至故障状态
            DF.SMFlag = Err;
        }
    }
    else
        // 计数器清0
        OCPCnt = 0;

    // 输出过流后恢复
    // 当发生输出软件过流保护，关机后等待4S后清楚故障信息，进入等待状态等待重启
    if (getRegBits(DF.ErrFlag, F_SW_IOUT_OCP)){
        // 等待故障清楚计数器累加
        RSCnt++;
        // 等待2S
        if (RSCnt > 400){
            RSCnt = 0;// 计数器清零
            RSNum++;// 过流重启计数器累加

            if (RSNum > 10){// 过流重启只重启10次，10次后不重启（严重故障）
                RSNum = 11;// 确保不清除故障，不重启
                DF.PWMENFlag = 0;// 关闭PWM
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BUCK电路的PWM输出
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TF1 | HRTIM_OUTPUT_TF2); // 关闭BOOST电路的PWM输出
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
    float TEMP1 = GET_NTC1_Temperature(); // 获取NTC1温度值
    float TEMP2 = GET_NTC2_Temperature(); // 获取NTC2温度值
    if (TEMP1 >= MAX_OTP_VAL || TEMP2 >= MAX_OTP_VAL){

        DF.SMFlag = Wait;
        DF.PWMENFlag = 0;      // 关闭PWM
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
        setRegBits(DF.ErrFlag, F_OTP);                                               // 故障标志位
        DF.SMFlag = Err;                                                             // 跳转至故障状态
    }
}

/**
 * @brief 运行模式判断。
 * BUCK模式：输出参考电压<0.8倍输入电压
 * BOOST模式：输出参考电压>1.2倍输入电压
 * MIX模式：1.15倍输入电压>输出参考电压>0.85倍输入电压
 * 当进入MIX（buck-boost）模式后，退出到BUCK或者BOOST时需要滞缓，防止在临界点来回振荡
 */
CCRAM void BBMode(void){
    uint8_t PreBBFlag = 0;// 上一次模式状态量
    PreBBFlag = DF.BBFlag;// 暂存当前的模式状态量

    uint32_t VIN_ADC = ADC1_RESULT[0]; // 输入电压ADC采样值

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

    // 判断当前模块的工作模式
    switch (DF.BBFlag){
        // NA-初始化模式
        case NA:{
            if (CtrValue.Vout_ref < VIN_ADC * 0.8F)        // 输出参考电压小于0.8倍输入电压时
                DF.BBFlag = Buck;                          // 切换到buck模式
            else if (CtrValue.Vout_ref > VIN_ADC * 1.2F)   // 输出参考电压大于1.2倍输入电压时
                DF.BBFlag = Boost;                         // 切换到boost模式
            else
                DF.BBFlag = Mix; // buck-boost（MIX） mode
            break;
        }
        // BUCK模式
        case Buck:{
            if (CtrValue.Vout_ref > VIN_ADC * 1.2F)         // vout>1.2*vin
                DF.BBFlag = Boost;                          // boost mode
            else if (CtrValue.Vout_ref > VIN_ADC * 0.85F)   // 1.2*vin>vout>0.85*vin
                DF.BBFlag = Mix;                            // buck-boost（MIX） mode
            break;
        }
        // Boost模式
        case Boost:{
            if (CtrValue.Vout_ref < VIN_ADC * 0.8F)         // vout<0.8*vin
                DF.BBFlag = Buck;                           // buck mode
            else if (CtrValue.Vout_ref < VIN_ADC * 1.15F)   // 0.8*vin<vout<1.15*vin
                DF.BBFlag = Mix;                            // buck-boost（MIX） mode
            break;
        }
        // Mix模式
        case Mix:{
            if (CtrValue.Vout_ref < VIN_ADC * 0.8F)      // vout<0.8*vin
                DF.BBFlag = Buck;                          // buck mode
            else if (CtrValue.Vout_ref > VIN_ADC * 1.2F) // vout>1.2*vin
                DF.BBFlag = Boost;                         // boost mode
            break;
        }
    }

    // 当模式发生变换时（上一次和这一次不一样）,则标志位置位，标志位用以环路计算复位，保证模式切换过程不会有大的过冲
    if (PreBBFlag == DF.BBFlag)
        DF.BBModeChange = 0;
    else
        DF.BBModeChange = 1;
}

/**
 * @brief 设置风扇 PWM 值,
 * 根据给定的 PWM 值，设置风扇的 PWM 输出。
 * @param dutyCycle PWM 值，范围在 0 到 100 之间
 */
void FAN_PWM_set(uint16_t dutyCycle){
    if (dutyCycle > 100){
        dutyCycle = 100;
    }
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, dutyCycle * 10);
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
    SET_Value.Vout = bytes_to_float(VSETtemp); // 将字节序列转换为浮点数
    SET_Value.Iout = bytes_to_float(ISETtemp); // 将字节序列转换为浮点数
    MAX_OTP_VAL = bytes_to_float(OTPtemp);
    MAX_VOUT_OCP_VAL = bytes_to_float(OCPtemp);
    MAX_VOUT_OVP_VAL = bytes_to_float(OVPtemp);
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


/**
 * @brief 根据主板温度自动控制风扇转速
 */
void Auto_FAN(void){
    const float TEMP1 = GET_NTC1_Temperature(); // 获取NTC1温度值
    const float TEMP2 = GET_NTC2_Temperature(); // 获取NTC2温度值
    const float TEMP = TEMP1 * 0.6 + TEMP2 * 0.4; // 计算平均温度

    if (TEMP < 35){FAN_PWM_set(0);}
    else if (TEMP >= 35){FAN_PWM_set(35);}
    else if (TEMP >= 40){FAN_PWM_set(45);}
    else if (TEMP >= 45){FAN_PWM_set(60);}
    else if (TEMP >= 50){FAN_PWM_set(70);}
    else if (TEMP >= 55){FAN_PWM_set(80);}
    else if (TEMP >= 60){FAN_PWM_set(90);}
    else if (TEMP >= 65){FAN_PWM_set(100);}
}
