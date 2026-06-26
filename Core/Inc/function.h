/**
  ******************************************************************************
  * @file    function.h
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


#ifndef FUNCTION1_H
#define FUNCTION1_H
#include "stdint.h"
#include "hrtim.h"

#define RAMFUNC __attribute__((section(".RamFunc")))
#define CCRAM __attribute__((section(".ccmram_bss")))
#define CCRAM_BSS __attribute__((section(".ccmram_bss")))

#ifndef UF4_FORCEINLINE
#if defined(__GNUC__)
#define UF4_FORCEINLINE static inline __attribute__((always_inline))
#else
#define UF4_FORCEINLINE static inline
#endif
#endif

#define ADC_MAX_VALUE 4095.0F				   // ADC最大值
#define REF_3V3 3.3F						   // VREF参考电压

#define POWER_CTRL_DEFAULT_SET_VOLTAGE       5.0F   // 默认输出电压
#define POWER_CTRL_DEFAULT_SET_CURRENT       1.0F   // 默认输出电流
#define POWER_CTRL_DEFAULT_OTP_SET           80.0F  // 默认OTP值
#define POWER_CTRL_DEFAULT_OVP_SET           46.0F  // 默认过压值 45v
#define POWER_CTRL_DEFAULT_OCP_SET           11.1F  // 默认过流值 10A
#define POWER_CTRL_OTP_CONFIRM_COUNT         10U    // 5ms周期下连续50ms超温才置位
#define POWER_CTRL_OTP_CLEAR_HYSTERESIS_C    5.0F   // 低于阈值5度后允许清除OTP

#define POWER_CTRL_FAN_MIN_RUN_DUTY          5U     // 风扇非零运行时的最小占空比
#define POWER_CTRL_FAN_MAX_RUN_DUTY          95U    // 风扇运行时的最大安全占空比
#define POWER_CTRL_FAN_STARTUP_KICK_DUTY     95U    // 风扇启动强启占空比
#define POWER_CTRL_FAN_STARTUP_KICK_MS       300U   // 风扇启动强启持续时间


#define POWER_CTRL_SHORT_CURRENT          5.0F      // 短接电流阈值
#define POWER_CTRL_SHORT_VOLTAGE          0.5F      // 短接电压阈值

/*
 * 从当前 200 kHz HRTIM的参考项目中重新离散
 * 周期（27200 tick @ 5.44 GHz 等效 HRTIM 时钟）。
 */
#define POWER_CTRL_BUCK_PID_B0                  5795
#define POWER_CTRL_BUCK_PID_B1                 -11411
#define POWER_CTRL_BUCK_PID_B2                  5617

#define POWER_CTRL_BOOST_PID_B0                 8844
#define POWER_CTRL_BOOST_PID_B1                -17413
#define POWER_CTRL_BOOST_PID_B2                 8572


// 输入输出 sense resistor  默认15倍缩放
#define BSP_POWER_DIVIDER_SCALE(r_upper, r_lower) ((r_upper) / (r_lower))

#define BSP_POWER_VIN_R_UPPER_OHM            75000.0F
#define BSP_POWER_VIN_R_LOWER_OHM            5000.0F
#define BSP_POWER_VOUT_R_UPPER_OHM           75000.0F
#define BSP_POWER_VOUT_R_LOWER_OHM           5000.0F

#define BSP_POWER_VIN_SENSE_SCALE            BSP_POWER_DIVIDER_SCALE(BSP_POWER_VIN_R_UPPER_OHM, BSP_POWER_VIN_R_LOWER_OHM)
#define BSP_POWER_VOUT_SENSE_SCALE           BSP_POWER_DIVIDER_SCALE(BSP_POWER_VOUT_R_UPPER_OHM, BSP_POWER_VOUT_R_LOWER_OHM)

/*
 * 电流使用1.65中点偏置。中点以上的值为
 * 反向电流，且被剪辑为0安;1.65 V到0 V为正值
 * 当前。这里保留了针对电路板的分流/增益/修整。
 * 默认还是7mΩ的检流电阻，ina240A1 20倍放大
 */
#define BSP_POWER_CURRENT_SHUNT_OHM          0.007F // 检流电阻
#define BSP_POWER_CURRENT_AMP_GAIN           20.0F  // INA240A1放大倍数
#define BSP_POWER_CURRENT_BIAS_V             1.65F  // 中点偏置电压
#define BSP_POWER_CURRENT_COMP_BIAS_BOOST_DCM_V   1.63F   // BOOST轻载/DCM补偿偏置
#define BSP_POWER_CURRENT_COMP_BIAS_BOOST_CCM_V   1.622F  // BOOST重载/CCM补偿偏置，避免电流显示被过度抬高

#define BSP_POWER_CURRENT_SENSE_V_PER_A      (BSP_POWER_CURRENT_SHUNT_OHM * BSP_POWER_CURRENT_AMP_GAIN)  // 每安培对应的采样电压

#define BSP_POWER_IIN_SCALE                  1.0F
#define BSP_POWER_IOUT_SCALE                 1.0F
#define BSP_POWER_IIN_GAIN                   BSP_POWER_CURRENT_AMP_GAIN * BSP_POWER_IIN_SCALE
#define BSP_POWER_IOUT_GAIN                  BSP_POWER_CURRENT_AMP_GAIN * BSP_POWER_IOUT_SCALE
#define BSP_POWER_IIN_ZERO_DEADBAND_A        0.1F  // 输入电流零点死区，抑制空载零点抖动上报
#define BSP_POWER_IOUT_ZERO_DEADBAND_A       0.1F  // 输出电流零点死区，抑制空载零点抖动上报
#define BSP_POWER_DCM_ENTER_CURRENT_A        2.8F  // 3A中心，低于该值进入轻载DCM
#define BSP_POWER_CCM_ENTER_CURRENT_A        3.2F  // 3A中心，高于该值进入重载CCM

#define BSP_POWER_CONDUCTION_MODE_DCM        0U
#define BSP_POWER_CONDUCTION_MODE_CCM        1U

// 硬件定时器参数
#define BSP_POWER_HRTIM_PERIOD_TICK          27200U  // 27200 tick @ 5.44 GHz 等效 HRTIM 时钟

#define BSP_POWER_BUCK_DUTY_MIN_TICK         136U
#define BSP_POWER_BUCK_DUTY_MAX_TICK         25568U
#define BSP_POWER_BUCK_DUTY_BOOST_STEP_TICK  32U
#define BSP_POWER_BUCK_DUTY_SYNC_MAX_TICK    21760U
#define BSP_POWER_BUCK_DUTY_SYNC_STEP_TICK   32U
#define BSP_POWER_BUCK_DISCHARGE_CMP_TICK    1200U

#define BSP_POWER_BOOST_DUTY_MIN_TICK        136U
#define BSP_POWER_BOOST_DUTY_SYNC_MIN_TICK   1800U
#define BSP_POWER_BOOST_DUTY_MAX_TICK        17680U
#define BSP_POWER_BOOST_DUTY_SYNC_MAX_TICK   25568U

#define POWER_CTRL_DISCHARGE_ENTER_MARGIN_ADC 80U
#define POWER_CTRL_DISCHARGE_EXIT_MARGIN_ADC  16U
#define POWER_CTRL_DISCHARGE_TARGET_BUCK_RATIO 0.95F

#define MAX_SHORT_I 10.1F   // 短路电流判据
#define MIN_SHORT_V 0.5F    // 短路电压判据

#define CAL_VIN_K 3985  // 输入电压矫正K值
#define CAL_VIN_B 33    // 输入电压矫正B值

#define CAL_IIN_K  4095
# define CAL_IIN_B 1

#define CAL_VOUT_K 4059 // 输出电压矫正K值
#define CAL_VOUT_B 13	// 输出电压矫正B值

#define CAL_IOUT_K 4095 // 输出电流矫正K值
#define CAL_IOUT_B 1	// 输出电流矫正B值

/***************故障类型*****************/

#define F_NOERR       0x0000		 // 无故障
#define F_SW_VIN_UVP  0x0001	     // 输入欠压
#define F_SW_VIN_OVP  0x0002	     // 输入过压
#define F_SW_VOUT_UVP 0x0004         // 输出欠压
#define F_SW_VOUT_OVP 0x0008         // 输出过压
#define F_SW_IOUT_OCP 0x0010         // 输出过流
#define F_SW_SHORT    0x0020	     // 输出短路
#define F_OTP         0x0040		 // 温度过高

#define ADC1_RESULT_VOUT_INDEX 0U
#define ADC1_RESULT_IOUT_INDEX 1U
#define ADC1_RESULT_VIN_INDEX  2U
#define ADC1_RESULT_IIN_INDEX  3U
#define ADC1_RESULT_COUNT      4U

#define LED_R_ON     HAL_GPIO_WritePin(GPIOB, LED_R_Pin, GPIO_PIN_SET);
#define LED_R_OFF  HAL_GPIO_WritePin(GPIOB, LED_R_Pin, GPIO_PIN_RESET);

#define LED_G_ON     HAL_GPIO_WritePin(GPIOB, LED_G_Pin, GPIO_PIN_SET);
#define LED_G_OFF  HAL_GPIO_WritePin(GPIOB, LED_G_Pin, GPIO_PIN_RESET);

#define LED_Y_ON     HAL_GPIO_WritePin(GPIOB, LED_Y_Pin, GPIO_PIN_SET);
#define LED_Y_OFF  HAL_GPIO_WritePin(GPIOB, LED_Y_Pin, GPIO_PIN_RESET);

struct _SET_Value
{
    volatile float SET_modified_flag; // 设置被修改标志位
    volatile float Vout;			  // 输出电压设置值
    volatile float Iout;			  // 输出电流设置值
};

// 控制参数结构体
struct _Ctr_value
{
    volatile int32_t Vout_ref;	   // 输出参考电压
    volatile int32_t Vout_SSref;   // 软启动时的输出参考电压
    volatile int32_t Vout_SETref;  // 设置的参考电压
    volatile int32_t Iout_ref;	   // 输出参考电流
    volatile int32_t I_Limit;	   // 限流参考电流
    volatile int16_t BUCKMaxDuty;  // Buck最大占空比
    volatile int16_t BoostMaxDuty; // Boost最大占空比
    volatile int16_t BuckDuty;	   // Buck控制占空比
    volatile int16_t BoostDuty;	   // Boost控制占空比
    volatile int32_t Ilimitout;	   // 电流环输出
};

// 标志位定义
struct _FLAG
{
    volatile uint16_t SMFlag;	   // 状态机标志位
    volatile uint16_t CtrFlag;	   // 控制标志位
    volatile uint16_t ErrFlag;	   // 故障标志位
    volatile uint8_t BBFlag;	   // 运行模式标志位，BUCK模式，BOOST模式，MIX混合模式
    volatile uint8_t PWMENFlag;	   // 启动标志位
    volatile uint8_t BBModeChange; // 工作模式切换标志位
    volatile uint8_t OUTPUT_Flag;  // 输出开关标志位, 0为关闭，1为开启
};

// 采样变量结构体
struct _ADI
{
    volatile uint32_t Iout;	   // 输出电流
    volatile uint32_t IoutAvg; // 输出电流平均值
    volatile uint32_t Vout;	   // 输出电电压
    volatile uint32_t VoutAvg; // 输出电电压平均值
    volatile uint32_t Iin;	   // 输出电流
    volatile uint32_t IinAvg;  // 输出电流平均值
    volatile uint32_t Vin;	   // 输出电电压
    volatile uint32_t VinAvg;  // 输出电电压平均值
};

// 状态机枚举量
typedef enum
{
    Init, // 初始化
    Wait, // 空闲等待
    Rise, // 软启
    Run,  // 正常运行
    Err	  // 故障
} STATE_M;

// 状态机枚举量
typedef enum
{
    NA,	   // 未定义
    Buck,  // BUCK模式
    Boost, // BOOST模式
    Mix	   // MIX混合模式
} BB_M;

// 软启动枚举变量
typedef enum
{
    SSInit, // 软启初始化
    SSWait, // 软启等待
    SSRun	// 开始软启
} SState_M;


typedef enum
{
    CV, // 恒压模式
    CC	// 恒流模式
} _CVCC_Mode;



extern volatile uint16_t ADC1_RESULT[4];		// ADC1 DMA采样结果：Vout, Iout, Vin, Iin
extern volatile uint8_t LED_Short_Flag;		    // 蜂鸣器短叫触发标志位
extern volatile uint8_t LED_Flag;			    // 蜂鸣器当前状态标志位
extern volatile uint8_t LED_Middle_Flag;		// 蜂鸣器中等时间长度鸣叫触发标志位

extern struct _Ctr_value CtrValue;				// 控制参数
extern struct _FLAG DF;							// 控制标志位
extern struct _ADI SADC;						// 采样变量
extern volatile _CVCC_Mode CVCC_Mode;			// 电源模式
extern struct _SET_Value SET_Value;				// 设置参数

extern volatile float VIN, VOUT, IIN, IOUT;		// 电压电流实际值
extern volatile float Board1_TEMP, Board2_TEMP,CPU_TEMP; // 主板和CPU温度实际值
extern volatile float MAX_OTP_VAL;              // 过温保护阈值
extern volatile float MAX_VOUT_OVP_VAL;         // 输出过压保护阈值
extern volatile float MAX_VOUT_OCP_VAL;         // 输出过流保护阈值
extern volatile float powerEfficiency;			// 电源转换效率
extern volatile uint8_t g_boost_conduction_mode; // BOOST轻载DCM/重载CCM模式
extern volatile uint8_t g_output_discharge_active; // 输出高于目标时主动泄放标志


extern volatile uint8_t g_mode_switch_inject_valid;   // 模式切换占空注入有效标志
extern volatile int16_t g_mode_switch_buck_duty;      // 模式切换预置 Buck 占空
extern volatile int16_t g_mode_switch_boost_duty;     // 模式切换预置 Boost 占空
extern volatile int32_t g_mode_switch_u_seed;         // 模式切换预置环路输出种子

/*
 * 设置寄存器的位
 * 参数：
 *   reg: 要操作的寄存器
 *   mask: 指定要设置的位掩码
 * 返回值：无
 */
#define setRegBits(reg, mask) (reg |= (unsigned int)(mask))

/*
 * 清除寄存器的位
 * 参数：
 *   reg: 要操作的寄存器
 *   mask: 指定要清除的位掩码
 * 返回值：无
 */
#define clrRegBits(reg, mask) (reg &= (unsigned int)(~(unsigned int)(mask)))

/*
 * 获取寄存器中指定位的值
 * 参数：
 *   reg: 要操作的寄存器
 *   mask: 指定要获取的位掩码
 * 返回值：掩码中为1的位的值
 */
#define getRegBits(reg, mask) (reg & (unsigned int)(mask))

/*
 * 获取寄存器的值
 * 参数：
 *   reg: 要获取的寄存器
 * 返回值：寄存器的当前值
 */
#define getReg(reg) (reg)

UF4_FORCEINLINE void PowerControl_HRTIM_SetBuckCompareFast(uint32_t compare_tick){
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = compare_tick;
}

UF4_FORCEINLINE void PowerControl_HRTIM_SetAdcTriggerCompareFast(uint32_t compare_tick){
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP3xR = compare_tick;
}

UF4_FORCEINLINE void PowerControl_HRTIM_SetBoostCompareFast(uint32_t compare_tick){
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_D].CMP1xR = compare_tick;
}

UF4_FORCEINLINE void PowerControl_HRTIM_OutputStartFast(uint32_t output_mask){
    hhrtim1.Instance->sCommonRegs.OENR |= output_mask;
}

UF4_FORCEINLINE void PowerControl_HRTIM_OutputStopFast(uint32_t output_mask){
    hhrtim1.Instance->sCommonRegs.ODISR |= output_mask;
}

UF4_FORCEINLINE uint32_t PowerControl_HRTIM_TimerARepPendingFast(void){
    return ((hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxISR & HRTIM_TIMISR_REP) != 0U);
}

UF4_FORCEINLINE void PowerControl_HRTIM_ClearTimerARepFast(void){
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].TIMxICR = HRTIM_TIMICR_REPC;
}

UF4_FORCEINLINE void PowerControl_HRTIM_EnableBuckLowSideDischargeFast(void){
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].SETx2R = HRTIM_SET2R_CMP1;
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].RSTx2R = HRTIM_RST2R_PER;
}

UF4_FORCEINLINE void PowerControl_HRTIM_DisableBuckLowSideDischargeFast(void){
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].SETx2R = 0U;
    hhrtim1.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].RSTx2R = 0U;
}

UF4_FORCEINLINE float PowerControl_GetBoostCurrentCompBias(void){
    return (g_boost_conduction_mode == BSP_POWER_CONDUCTION_MODE_CCM) ?
           BSP_POWER_CURRENT_COMP_BIAS_BOOST_CCM_V :
           BSP_POWER_CURRENT_COMP_BIAS_BOOST_DCM_V;
}

void ADCSample(void);
void ADC_calculate(void);

void StateM(void);
void StateMInit(void);
void StateMWait(void);
void StateMRise(void);
void StateMRun(void);
void StateMErr(void);
void BBMode(void);
void PowerControl_PrepareModeSwitch(BB_M target_mode, uint32_t vin_adc, int32_t vout_ref);
void PowerControl_DisableOutput(void);
void PowerControl_UpdateConductionMode(void);
void PowerControl_ApplyBoostConductionMode(void);
void PowerControl_UpdateDischargeMode(void);
void PowerControl_ApplyBuckDischargeMode(void);

void ValInit(void);
void OTP(void);
void OVP(void);
void OCP(void);
void ShortOff(void);

void FAN_PWM_set(uint16_t pwm);
void Init_Flash(void);
void Update_Flash(void);
void Read_Flash(void);
void float_to_bytes(float value, uint8_t *bytes);
float bytes_to_float(uint8_t *bytes);


#endif //FUNCTION1_H
