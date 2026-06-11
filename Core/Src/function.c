//
// Created by UF4 on 26-6-11.
//

#include "function.h" //功能函数头文件
#include <math.h>
#include "adc.h"
#include "usart.h"
#include "tim.h"
#include "hrtim.h"
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
volatile float Board1_TEMP, Board1_TEMP, CPU_TEMP;                 // 主板和CPU温度实际值
volatile float powerEfficiency = 0;                                // 电源转换效率

extern volatile int32_t VErr0, VErr1, VErr2; // 电压误差
extern volatile int32_t u0, u1;              // 电压环输出量


CCRAM void ADCSample(void){
    // 输入输出采样参数求和，用以计算平均值
    static uint32_t VinAvgSum = 0, IinAvgSum = 0, VoutAvgSum = 0, IoutAvgSum = 0;

    // 从DMA缓冲器中获取数据
    SADC.Vin = (uint32_t)ADC1_RESULT[0];
    SADC.Iin = (uint32_t)ADC1_RESULT[1];
    SADC.Vout = (uint32_t)((ADC1_RESULT[2] * CAL_VOUT_K >> 12) + CAL_VOUT_B);
    SADC.Iout = (uint32_t)((ADC1_RESULT[3] * CAL_IOUT_K >> 12) + CAL_IOUT_B);

    if (SADC.Vin < 15) // 采样有零偏离，采样值很小时，直接为0
        SADC.Vin = 0;
    if (SADC.Vout < 15)
        SADC.Vout = 0;
    if (SADC.Iout < 16)
        SADC.Iout = 0;

    // 计算各个采样值的平均值-滑动平均方式
    VinAvgSum = VinAvgSum + SADC.Vin - (VinAvgSum >> 3); // 求和，新增入一个新的采样值，同时减去之前的平均值。
    SADC.VinAvg = VinAvgSum >> 3;                        // 求平均
    IinAvgSum = IinAvgSum + SADC.Iin - (IinAvgSum >> 3);
    SADC.IinAvg = IinAvgSum >> 3;
    VoutAvgSum = VoutAvgSum + SADC.Vout - (VoutAvgSum >> 3);
    SADC.VoutAvg = VoutAvgSum >> 3;
    IoutAvgSum = IoutAvgSum + SADC.Iout - (IoutAvgSum >> 3);
    SADC.IoutAvg = IoutAvgSum >> 3;
}

/**
 * @brief ADC数据计算转换成实际数值的浮点数
 *
 */
void ADC_calculate(void){
    VIN  = SADC.VinAvg  * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VIN_SENSE_SCALE;   // 计算ADC1通道0输入电压采样结果
    IIN  = SADC.IinAvg  * REF_3V3 / ADC_MAX_VALUE / BSP_POWER_IIN_GAIN ;         // 计算ADC1通道1输入电流采样结果
    VOUT = SADC.VoutAvg * REF_3V3 / ADC_MAX_VALUE * BSP_POWER_VOUT_SENSE_SCALE;  // 计算ADC1通道2输出电压采样结果
    IOUT = SADC.IoutAvg * REF_3V3 / ADC_MAX_VALUE / BSP_POWER_IOUT_GAIN;         // 计算ADC1通道3输出电流采样结果

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
void StateMInit(void)
{
    // 相关参数初始化
    ValInit();
    // 状态机跳转至等待软启状态
    DF.SMFlag = Wait;
}



/*
 * @brief 相关参数初始化函数
 */
void ValInit(void)
{
    // 关闭PWM
    DF.PWMENFlag = 0;
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2); // 关闭BUCK电路的PWM输出
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2); // 关闭BOOST电路的PWM输出
    DF.BBFlag = NA;
    // 清除故障标志位
    DF.ErrFlag = 0;
    // 初始化电压参考量
    CtrValue.Vout_ref = 0;
    // 限制占空比
    CtrValue.BuckDuty = MIN_BUKC_DUTY;
    CtrValue.BUCKMaxDuty = MIN_BUKC_DUTY;
    CtrValue.BoostDuty = MIN_BOOST_DUTY;
    CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;
    // 环路计算变量初始化
    VErr0 = 0;
    VErr1 = 0;
    VErr2 = 0;
    u0 = 0;
    u1 = 0;
    // 设置值初始化
    SET_Value.Vout = 5.0;
    SET_Value.Iout = 10.0;
    MAX_OTP_VAL = 80.0F;      // 过温保护阈值
    MAX_VOUT_OVP_VAL = 50.0F; // 输出过压保护阈值
    MAX_VOUT_OCP_VAL = 10.5F; // 输出过流保护阈值
}
