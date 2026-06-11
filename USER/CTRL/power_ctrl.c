/**
 * @file power_ctrl.c
 * @brief Buck-boost control algorithm migrated from the example project.
 */

#include "power_ctrl.h"
#include <string.h>
#include "adc.h"
#include "hrtim.h"
#include "main.h"
#include "tim.h"

#define POWER_CTRL_DEFAULT_SET_VOLTAGE_MV       5000U
#define POWER_CTRL_DEFAULT_SET_CURRENT_MA       10000U
#define POWER_CTRL_DEFAULT_OTP_SET_MC           80000
#define POWER_CTRL_DEFAULT_OVP_SET_MV           50000U
#define POWER_CTRL_DEFAULT_OCP_SET_MA           10500U

#define POWER_CTRL_VIN_UVLO_START_MV            5500U
#define POWER_CTRL_VIN_UVLO_STOP_MV             5200U
#define POWER_CTRL_VIN_OVP_MV                   50000U
#define POWER_CTRL_SHORT_CURRENT_MA             10100U
#define POWER_CTRL_SHORT_VOLTAGE_MV             500U
#define POWER_CTRL_WAIT_TICK_COUNT              200U
#define POWER_CTRL_SOFTSTART_WAIT_TICK_COUNT    5U
#define POWER_CTRL_OVP_HOLD_TICK_COUNT          2U
#define POWER_CTRL_OCP_HOLD_TICK_COUNT          10U
#define POWER_CTRL_FAULT_RETRY_TICK_COUNT       400U
#define POWER_CTRL_FAULT_RETRY_LIMIT            10U
#define POWER_CTRL_MODE_AVG_COUNT               5U
#define POWER_CTRL_FAST_LOOP_HZ                 200000U
#define POWER_CTRL_SLOW_LOOP_HZ                 200U
#define POWER_CTRL_SLOW_LOOP_DIVIDER            (POWER_CTRL_FAST_LOOP_HZ / POWER_CTRL_SLOW_LOOP_HZ)

/*
 * Re-discretized from the reference project for the current 200 kHz HRTIM
 * period (27200 ticks @ 5.44 GHz equivalent HRTIM clock).
 */
#define POWER_CTRL_BUCK_PID_B0                  5795
#define POWER_CTRL_BUCK_PID_B1                 -11411
#define POWER_CTRL_BUCK_PID_B2                  5617
#define POWER_CTRL_BOOST_PID_B0                 8844
#define POWER_CTRL_BOOST_PID_B1                -17413
#define POWER_CTRL_BOOST_PID_B2                 8572

#define POWER_CTRL_CURRENT_LOOP_KP              6
#define POWER_CTRL_CURRENT_LOOP_KI              3
#define POWER_CTRL_CURRENT_LOOP_KD              1
#define POWER_CTRL_BUCK_LIGHT_LOAD_ENTER_RAW    70
#define POWER_CTRL_BUCK_LIGHT_LOAD_EXIT_RAW     175
#define POWER_CTRL_FAN_AUTO_MAX_PERMILLE        1000U

typedef enum
{
    POWER_STATE_INIT = 0U,
    POWER_STATE_WAIT,
    POWER_STATE_RISE,
    POWER_STATE_RUN,
    POWER_STATE_ERR,
} POWER_state_t;

typedef enum
{
    POWER_SOFTSTART_INIT = 0U,
    POWER_SOFTSTART_WAIT,
    POWER_SOFTSTART_RUN,
} POWER_softstartState_t;

typedef struct
{
    int32_t vout_ref_raw;
    int32_t vout_softstart_ref_raw;
    int32_t vout_set_ref_raw;
    int32_t iout_ref_raw;
    int16_t buck_max_duty_tick;
    int16_t boost_max_duty_tick;
    int16_t buck_duty_tick;
    int16_t boost_duty_tick;
    int32_t current_loop_out;
} POWER_controlValue_t;

typedef struct
{
    uint16_t vin_raw_avg;
    uint16_t iin_raw_avg;
    uint16_t vout_raw_avg;
    uint16_t iout_raw_avg;
    BSP_powerMeasurement_t measurement;
} POWER_sample_t;

static POWER_ctrlSettings_t s_POWER_ctrlSettings = {
    .set_voltage_mv = POWER_CTRL_DEFAULT_SET_VOLTAGE_MV,
    .set_current_ma = POWER_CTRL_DEFAULT_SET_CURRENT_MA,
    .otp_set_mc = POWER_CTRL_DEFAULT_OTP_SET_MC,
    .ovp_set_mv = POWER_CTRL_DEFAULT_OVP_SET_MV,
    .ocp_set_ma = POWER_CTRL_DEFAULT_OCP_SET_MA,
    .fan_set_permille = 0U,
};

static POWER_controlValue_t s_POWER_controlValue;
static POWER_sample_t s_POWER_sample;
static BSP_adcResult_t s_POWER_fastAdcResult;

static POWER_state_t s_POWER_state = POWER_STATE_INIT;
static POWER_softstartState_t s_POWER_softstartState = POWER_SOFTSTART_INIT;
static uint8_t s_POWER_ctrlEnabled = 0U;
static uint32_t s_POWER_ctrlFaultFlags = POWER_CTRL_FAULT_NONE;
static uint8_t s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
static uint8_t s_POWER_ctrlCvccMode = POWER_CTRL_CVCC_CV;
static uint8_t s_POWER_stageModeChanged = 0U;
static uint8_t s_POWER_pwmEnabled = 0U;
static uint8_t s_POWER_adcAverageInitialized = 0U;
static uint8_t s_POWER_vinUvLocked = 1U;
static uint8_t s_POWER_buckLightLoad = 0U;
static uint32_t s_POWER_fanSpeedPermille = 0U;

static int32_t s_POWER_v_err0 = 0;
static int32_t s_POWER_v_err1 = 0;
static int32_t s_POWER_v_err2 = 0;
static int32_t s_POWER_i_err0 = 0;
static int32_t s_POWER_i_err1 = 0;
static int32_t s_POWER_u0 = 0;
static int32_t s_POWER_u1 = 0;
static int32_t s_POWER_i0 = 0;
static int32_t s_POWER_currentIntegral = 0;
static uint16_t s_POWER_slowLoopDivider = 0U;
static uint16_t s_POWER_ovpCount = 0U;
static uint16_t s_POWER_ocpCount = 0U;
static uint16_t s_POWER_shortRetryCount = 0U;
static uint16_t s_POWER_ocpRetryCount = 0U;
static uint8_t s_POWER_shortRetryNum = 0U;
static uint8_t s_POWER_ocpRetryNum = 0U;
static uint32_t s_POWER_modeVinSum = 0U;
static uint8_t s_POWER_modeVinCount = 0U;
static uint16_t s_POWER_waitCount = 0U;
static uint16_t s_POWER_softstartWaitCount = 0U;
static uint16_t s_POWER_buckMaxDutyCount = 0U;
static uint16_t s_POWER_boostMaxDutyCount = 0U;

static uint32_t s_POWER_getAppU32Nonnegative(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled <= 0.0F)
    {
        return 0U;
    }

    return (uint32_t)(scaled + 0.5F);
}

static int32_t s_POWER_getAppI32(float value, float scale)
{
    const float scaled = value * scale;

    if (scaled >= 0.0F)
    {
        return (int32_t)(scaled + 0.5F);
    }

    return (int32_t)(scaled - 0.5F);
}

static uint8_t s_POWER_getAppStateFlagBits(void)
{
    if (s_POWER_state == POWER_STATE_ERR)
    {
        return POWER_CTRL_STATE_FLAG_ERR;
    }

    if ((s_POWER_vinUvLocked != 0U) && (s_POWER_ctrlFaultFlags == POWER_CTRL_FAULT_NONE))
    {
        return POWER_CTRL_STATE_FLAG_WAIT;
    }

    switch (s_POWER_state)
    {
        case POWER_STATE_INIT:
            return POWER_CTRL_STATE_FLAG_INIT;

        case POWER_STATE_WAIT:
            return POWER_CTRL_STATE_FLAG_WAIT;

        case POWER_STATE_RISE:
            return POWER_CTRL_STATE_FLAG_RISE;

        case POWER_STATE_RUN:
            return POWER_CTRL_STATE_FLAG_RUN;

        case POWER_STATE_ERR:
        default:
            return POWER_CTRL_STATE_FLAG_ERR;
    }
}

static uint16_t s_POWER_getAppClampU16(uint32_t value)
{
    return (value > 0xFFFFU) ? 0xFFFFU : (uint16_t)value;
}

static int16_t s_POWER_getAppClampDuty(int32_t duty, uint32_t min_tick, uint32_t max_tick)
{
    if (duty < (int32_t)min_tick)
    {
        return (int16_t)min_tick;
    }

    if (duty > (int32_t)max_tick)
    {
        return (int16_t)max_tick;
    }

    return (int16_t)duty;
}

static uint16_t s_POWER_getAppVoutRawFromMillivolts(uint32_t mv)
{
    const float adc_v = ((float)mv / 1000.0F) / BSP_POWER_VOUT_SENSE_SCALE;
    const float raw = (adc_v * BSP_POWER_ADC1_FULL_SCALE_RAW) / BSP_POWER_ADC_VREF_V;
    return s_POWER_getAppClampU16((uint32_t)(raw + 0.5F));
}

static uint16_t s_POWER_getAppIoutRawFromMilliamps(uint32_t ma)
{
    const float sense_v = ((float)ma / 1000.0F) * BSP_POWER_CURRENT_SENSE_V_PER_A;
    const float raw = (sense_v * BSP_POWER_ADC1_FULL_SCALE_RAW) / BSP_POWER_ADC_VREF_V;
    return s_POWER_getAppClampU16((uint32_t)(raw + 0.5F));
}

static uint32_t s_POWER_getAppMillivoltsFromVoutRaw(int32_t raw)
{
    const float adc_v = ((float)raw * BSP_POWER_ADC_VREF_V) / BSP_POWER_ADC1_FULL_SCALE_RAW;
    const float vout_v = adc_v * BSP_POWER_VOUT_SENSE_SCALE;
    return s_POWER_getAppU32Nonnegative(vout_v, 1000.0F);
}

static uint16_t s_POWER_getAppForwardCurrentRaw(uint16_t raw)
{
    const float bias_raw_float = (BSP_POWER_CURRENT_BIAS_V * BSP_POWER_ADC1_FULL_SCALE_RAW) / BSP_POWER_ADC_VREF_V;
    const uint16_t bias_raw = s_POWER_getAppClampU16((uint32_t)(bias_raw_float + 0.5F));

    if (raw >= bias_raw)
    {
        return 0U;
    }

    return (uint16_t)(bias_raw - raw);
}

static uint16_t s_POWER_getAppInnerCurrentRaw(const BSP_adcResult_t *adc_result, uint8_t stage_mode)
{
    if (adc_result == NULL)
    {
        return 0U;
    }

    switch (stage_mode)
    {
        case POWER_CTRL_STAGE_BUCK:
            return s_POWER_getAppForwardCurrentRaw(adc_result->iout_raw);

        case POWER_CTRL_STAGE_BOOST:
        case POWER_CTRL_STAGE_MIX:
        default:
            return s_POWER_getAppForwardCurrentRaw(adc_result->iin_raw);
    }
}

static BSP_powerStageMode_t s_POWER_getAppBspStageMode(uint8_t stage_mode)
{
    switch (stage_mode)
    {
        case POWER_CTRL_STAGE_BUCK:
            return BSP_POWER_STAGE_MODE_BUCK;

        case POWER_CTRL_STAGE_BOOST:
            return BSP_POWER_STAGE_MODE_BOOST;

        case POWER_CTRL_STAGE_MIX:
        default:
            return BSP_POWER_STAGE_MODE_MIXED;
    }
}

static void s_POWER_setAppBuckLightLoad(uint8_t enabled)
{
    s_POWER_buckLightLoad = (enabled != 0U) ? 1U : 0U;
}

static uint32_t s_POWER_getAppDutyCmdTick(void)
{
    switch (s_POWER_ctrlStageMode)
    {
        case POWER_CTRL_STAGE_BOOST:
        case POWER_CTRL_STAGE_MIX:
            return (uint32_t)s_POWER_controlValue.boost_duty_tick;

        case POWER_CTRL_STAGE_BUCK:
        case POWER_CTRL_STAGE_NA:
        default:
            return (uint32_t)s_POWER_controlValue.buck_duty_tick;
    }
}

static void s_POWER_resetAppPid(void)
{
    s_POWER_v_err0 = 0;
    s_POWER_v_err1 = 0;
    s_POWER_v_err2 = 0;
    s_POWER_i_err0 = 0;
    s_POWER_i_err1 = 0;
    s_POWER_u0 = 0;
    s_POWER_u1 = 0;
    s_POWER_i0 = 0;
    s_POWER_currentIntegral = 0;
    s_POWER_setAppBuckLightLoad(0U);
}

static void s_POWER_resetAppCounters(void)
{
    s_POWER_slowLoopDivider = 0U;
    s_POWER_ovpCount = 0U;
    s_POWER_ocpCount = 0U;
    s_POWER_shortRetryCount = 0U;
    s_POWER_ocpRetryCount = 0U;
    s_POWER_shortRetryNum = 0U;
    s_POWER_ocpRetryNum = 0U;
    s_POWER_modeVinSum = 0U;
    s_POWER_modeVinCount = 0U;
    s_POWER_waitCount = 0U;
    s_POWER_softstartWaitCount = 0U;
    s_POWER_buckMaxDutyCount = 0U;
    s_POWER_boostMaxDutyCount = 0U;
}

static void s_POWER_stopAppPwm(void)
{
    s_POWER_pwmEnabled = 0U;
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2);
    s_POWER_setAppBuckLightLoad(0U);
}

static void s_POWER_startAppPwm(void)
{
    __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, BSP_POWER_HRTIM_PERIOD_TICK);
    __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1, BSP_POWER_HRTIM_PERIOD_TICK);
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2);
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TD1 | HRTIM_OUTPUT_TD2);
    s_POWER_pwmEnabled = 1U;
    s_POWER_setAppBuckLightLoad(0U);
}

static void s_POWER_updateAppReferences(void)
{
    s_POWER_controlValue.vout_set_ref_raw = s_POWER_getAppVoutRawFromMillivolts(s_POWER_ctrlSettings.set_voltage_mv);
    s_POWER_controlValue.iout_ref_raw = s_POWER_getAppIoutRawFromMilliamps(s_POWER_ctrlSettings.set_current_ma);
}

static void s_POWER_setAppFan(uint32_t fan_permille)
{
    uint32_t compare;

    if (fan_permille > 1000U)
    {
        fan_permille = 1000U;
    }

    compare = ((uint32_t)(__HAL_TIM_GET_AUTORELOAD(&htim8) + 1U) * fan_permille) / 1000U;
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, compare);
    s_POWER_fanSpeedPermille = fan_permille;
}

static void s_POWER_updateAppFan(void)
{
    uint32_t fan_permille;
    const float board_temp_c = s_POWER_sample.measurement.temp1_c;

    if (s_POWER_ctrlSettings.fan_set_permille != 0U)
    {
        s_POWER_setAppFan(s_POWER_ctrlSettings.fan_set_permille);
        return;
    }

    if (board_temp_c < 35.0F)
    {
        fan_permille = 0U;
    }
    else if (board_temp_c < 40.0F)
    {
        fan_permille = 350U;
    }
    else if (board_temp_c < 45.0F)
    {
        fan_permille = 450U;
    }
    else if (board_temp_c < 50.0F)
    {
        fan_permille = 600U;
    }
    else if (board_temp_c < 55.0F)
    {
        fan_permille = 700U;
    }
    else if (board_temp_c < 60.0F)
    {
        fan_permille = 800U;
    }
    else if (board_temp_c < 65.0F)
    {
        fan_permille = 900U;
    }
    else
    {
        fan_permille = POWER_CTRL_FAN_AUTO_MAX_PERMILLE;
    }

    s_POWER_setAppFan(fan_permille);
}

static void s_POWER_updateAppStatusLed(void)
{
    if (s_POWER_ctrlFaultFlags != POWER_CTRL_FAULT_NONE)
    {
        HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_RESET);
        return;
    }

    if (s_POWER_state == POWER_STATE_RUN)
    {
        HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_RESET);
        return;
    }

    if ((s_POWER_state == POWER_STATE_RISE) || (s_POWER_vinUvLocked != 0U))
    {
        HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_SET);
        return;
    }

    HAL_GPIO_WritePin(RED_GPIO_Port, RED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GREEN_GPIO_Port, GREEN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BLUE_GPIO_Port, BLUE_Pin, GPIO_PIN_RESET);
}

static void s_POWER_sampleAppAdc(void)
{
    BSP_adcResult_t adc_result = s_POWER_fastAdcResult;
    static uint32_t vin_avg_sum = 0U;
    static uint32_t iin_avg_sum = 0U;
    static uint32_t vout_avg_sum = 0U;
    static uint32_t iout_avg_sum = 0U;

    if (s_POWER_adcAverageInitialized == 0U)
    {
        vin_avg_sum = ((uint32_t)adc_result.vin_raw) << 3;
        iin_avg_sum = ((uint32_t)adc_result.iin_raw) << 3;
        vout_avg_sum = ((uint32_t)adc_result.vout_raw) << 3;
        iout_avg_sum = ((uint32_t)adc_result.iout_raw) << 3;
        s_POWER_adcAverageInitialized = 1U;
    }

    vin_avg_sum = vin_avg_sum + adc_result.vin_raw - (vin_avg_sum >> 3);
    iin_avg_sum = iin_avg_sum + adc_result.iin_raw - (iin_avg_sum >> 3);
    vout_avg_sum = vout_avg_sum + adc_result.vout_raw - (vout_avg_sum >> 3);
    iout_avg_sum = iout_avg_sum + adc_result.iout_raw - (iout_avg_sum >> 3);

    s_POWER_sample.vin_raw_avg = (uint16_t)(vin_avg_sum >> 3);
    s_POWER_sample.iin_raw_avg = (uint16_t)(iin_avg_sum >> 3);
    s_POWER_sample.vout_raw_avg = (uint16_t)(vout_avg_sum >> 3);
    s_POWER_sample.iout_raw_avg = (uint16_t)(iout_avg_sum >> 3);

    adc_result.vin_raw = s_POWER_sample.vin_raw_avg;
    adc_result.iin_raw = s_POWER_sample.iin_raw_avg;
    adc_result.vout_raw = s_POWER_sample.vout_raw_avg;
    adc_result.iout_raw = s_POWER_sample.iout_raw_avg;

    BSP_getAppMeasurement(&adc_result, &s_POWER_sample.measurement);
}

static void s_POWER_initAppValues(void)
{
    s_POWER_stopAppPwm();
    s_POWER_ctrlFaultFlags = POWER_CTRL_FAULT_NONE;
    s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
    s_POWER_stageModeChanged = 0U;

    s_POWER_controlValue.vout_ref_raw = 0;
    s_POWER_controlValue.vout_softstart_ref_raw = 0;
    s_POWER_controlValue.vout_set_ref_raw = 0;
    s_POWER_controlValue.iout_ref_raw = 0;
    s_POWER_controlValue.buck_duty_tick = BSP_POWER_BUCK_DUTY_MIN_TICK;
    s_POWER_controlValue.buck_max_duty_tick = BSP_POWER_BUCK_DUTY_MIN_TICK;
    s_POWER_controlValue.boost_duty_tick = BSP_POWER_BOOST_DUTY_MIN_TICK;
    s_POWER_controlValue.boost_max_duty_tick = BSP_POWER_BOOST_DUTY_MIN_TICK;
    s_POWER_controlValue.current_loop_out = 0;
    s_POWER_resetAppPid();
    s_POWER_updateAppReferences();
}

static void s_POWER_enterAppFault(uint32_t fault)
{
    s_POWER_ctrlFaultFlags |= fault;
    s_POWER_stopAppPwm();
    s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
    s_POWER_softstartState = POWER_SOFTSTART_INIT;
    s_POWER_controlValue.vout_softstart_ref_raw = 0;
    s_POWER_resetAppPid();
    s_POWER_state = POWER_STATE_ERR;
}

static void s_POWER_updateAppVinUvLock(void)
{
    const uint32_t vin_mv = s_POWER_getAppU32Nonnegative(s_POWER_sample.measurement.vin_v, 1000.0F);

    if (s_POWER_vinUvLocked != 0U)
    {
        if (vin_mv >= POWER_CTRL_VIN_UVLO_START_MV)
        {
            s_POWER_vinUvLocked = 0U;
            return;
        }
    }
    else if (vin_mv > POWER_CTRL_VIN_UVLO_STOP_MV)
    {
        return;
    }

    s_POWER_vinUvLocked = 1U;
    s_POWER_enterAppFault(POWER_CTRL_FAULT_VIN_UVP);
}

static void s_POWER_checkAppProtection(void)
{
    const uint32_t vout_mv = s_POWER_getAppU32Nonnegative(s_POWER_sample.measurement.vout_v, 1000.0F);
    const uint32_t iout_ma = s_POWER_getAppU32Nonnegative(s_POWER_sample.measurement.iout_a, 1000.0F);
    const uint32_t vin_mv = s_POWER_getAppU32Nonnegative(s_POWER_sample.measurement.vin_v, 1000.0F);
    const int32_t board_temp_mc = s_POWER_getAppI32(s_POWER_sample.measurement.temp1_c, 1000.0F);

    if (s_POWER_vinUvLocked != 0U)
    {
        s_POWER_ovpCount = 0U;
        s_POWER_ocpCount = 0U;
        return;
    }

    if (vin_mv >= POWER_CTRL_VIN_OVP_MV)
    {
        s_POWER_enterAppFault(POWER_CTRL_FAULT_VIN_OVP);
        return;
    }

    if ((iout_ma > POWER_CTRL_SHORT_CURRENT_MA) && (vout_mv < POWER_CTRL_SHORT_VOLTAGE_MV))
    {
        s_POWER_enterAppFault(POWER_CTRL_FAULT_SHORT);
        return;
    }

    if (board_temp_mc >= s_POWER_ctrlSettings.otp_set_mc)
    {
        s_POWER_enterAppFault(POWER_CTRL_FAULT_OTP);
        return;
    }

    if (vout_mv >= s_POWER_ctrlSettings.ovp_set_mv)
    {
        ++s_POWER_ovpCount;
        if (s_POWER_ovpCount > POWER_CTRL_OVP_HOLD_TICK_COUNT)
        {
            s_POWER_ovpCount = 0U;
            s_POWER_enterAppFault(POWER_CTRL_FAULT_VOUT_OVP);
            return;
        }
    }
    else
    {
        s_POWER_ovpCount = 0U;
    }

    if ((iout_ma >= s_POWER_ctrlSettings.ocp_set_ma) && (s_POWER_state == POWER_STATE_RUN))
    {
        ++s_POWER_ocpCount;
        if (s_POWER_ocpCount > POWER_CTRL_OCP_HOLD_TICK_COUNT)
        {
            s_POWER_ocpCount = 0U;
            s_POWER_enterAppFault(POWER_CTRL_FAULT_IOUT_OCP);
            return;
        }
    }
    else
    {
        s_POWER_ocpCount = 0U;
    }
}

static void s_POWER_updateAppFaultRecovery(void)
{
    if ((s_POWER_ctrlFaultFlags & POWER_CTRL_FAULT_VIN_UVP) != 0U)
    {
        if (s_POWER_vinUvLocked == 0U)
        {
            s_POWER_ctrlFaultFlags &= ~POWER_CTRL_FAULT_VIN_UVP;
        }
    }

    if ((s_POWER_ctrlFaultFlags & POWER_CTRL_FAULT_SHORT) != 0U)
    {
        ++s_POWER_shortRetryCount;
        if (s_POWER_shortRetryCount > POWER_CTRL_FAULT_RETRY_TICK_COUNT)
        {
            s_POWER_shortRetryCount = 0U;
            if (s_POWER_shortRetryNum <= POWER_CTRL_FAULT_RETRY_LIMIT)
            {
                ++s_POWER_shortRetryNum;
                s_POWER_ctrlFaultFlags &= ~POWER_CTRL_FAULT_SHORT;
            }
            else
            {
                s_POWER_shortRetryNum = (uint8_t)(POWER_CTRL_FAULT_RETRY_LIMIT + 1U);
            }
        }
    }
    else
    {
        s_POWER_shortRetryCount = 0U;
    }

    if ((s_POWER_ctrlFaultFlags & POWER_CTRL_FAULT_IOUT_OCP) != 0U)
    {
        ++s_POWER_ocpRetryCount;
        if (s_POWER_ocpRetryCount > POWER_CTRL_FAULT_RETRY_TICK_COUNT)
        {
            s_POWER_ocpRetryCount = 0U;
            if (s_POWER_ocpRetryNum <= POWER_CTRL_FAULT_RETRY_LIMIT)
            {
                ++s_POWER_ocpRetryNum;
                s_POWER_ctrlFaultFlags &= ~POWER_CTRL_FAULT_IOUT_OCP;
            }
            else
            {
                s_POWER_ocpRetryNum = (uint8_t)(POWER_CTRL_FAULT_RETRY_LIMIT + 1U);
            }
        }
    }
    else
    {
        s_POWER_ocpRetryCount = 0U;
    }
}

static void s_POWER_updateAppMode(void)
{
    uint16_t vin_raw = s_POWER_sample.vin_raw_avg;
    uint8_t previous_mode = s_POWER_ctrlStageMode;

    if (s_POWER_vinUvLocked != 0U)
    {
        s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
        s_POWER_stageModeChanged = (previous_mode == s_POWER_ctrlStageMode) ? 0U : 1U;
        return;
    }

    s_POWER_modeVinSum += s_POWER_sample.vin_raw_avg;
    ++s_POWER_modeVinCount;
    if (s_POWER_modeVinCount >= POWER_CTRL_MODE_AVG_COUNT)
    {
        vin_raw = (uint16_t)(s_POWER_modeVinSum / POWER_CTRL_MODE_AVG_COUNT);
        s_POWER_modeVinSum = 0U;
        s_POWER_modeVinCount = 0U;
    }

    switch (s_POWER_ctrlStageMode)
    {
        case POWER_CTRL_STAGE_NA:
            if (s_POWER_controlValue.vout_ref_raw < (int32_t)((float)vin_raw * 0.8F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_BUCK;
            }
            else if (s_POWER_controlValue.vout_ref_raw > (int32_t)((float)vin_raw * 1.2F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_BOOST;
            }
            else
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_MIX;
            }
            break;

        case POWER_CTRL_STAGE_BUCK:
            if (s_POWER_controlValue.vout_ref_raw > (int32_t)((float)vin_raw * 1.2F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_BOOST;
            }
            else if (s_POWER_controlValue.vout_ref_raw > (int32_t)((float)vin_raw * 0.85F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_MIX;
            }
            break;

        case POWER_CTRL_STAGE_BOOST:
            if (s_POWER_controlValue.vout_ref_raw < (int32_t)((float)vin_raw * 0.8F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_BUCK;
            }
            else if (s_POWER_controlValue.vout_ref_raw < (int32_t)((float)vin_raw * 1.15F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_MIX;
            }
            break;

        case POWER_CTRL_STAGE_MIX:
        default:
            if (s_POWER_controlValue.vout_ref_raw < (int32_t)((float)vin_raw * 0.8F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_BUCK;
            }
            else if (s_POWER_controlValue.vout_ref_raw > (int32_t)((float)vin_raw * 1.2F))
            {
                s_POWER_ctrlStageMode = POWER_CTRL_STAGE_BOOST;
            }
            break;
    }

    s_POWER_stageModeChanged = (previous_mode == s_POWER_ctrlStageMode) ? 0U : 1U;
}

static void s_POWER_runAppPid(void)
{
    const int32_t vout_temp = s_POWER_fastAdcResult.vout_raw;
    const int32_t iout_temp = s_POWER_getAppInnerCurrentRaw(&s_POWER_fastAdcResult, s_POWER_ctrlStageMode);
    const int32_t active_vout_limit_raw = ((s_POWER_state == POWER_STATE_RISE) &&
                                           (vout_temp < (s_POWER_controlValue.vout_set_ref_raw >> 1)))
                                              ? s_POWER_controlValue.vout_softstart_ref_raw
                                              : s_POWER_controlValue.vout_set_ref_raw;

    s_POWER_controlValue.vout_ref_raw = active_vout_limit_raw;
    s_POWER_i_err0 = s_POWER_controlValue.iout_ref_raw - iout_temp;
    s_POWER_i0 = s_POWER_currentIntegral +
                 (s_POWER_i_err0 * POWER_CTRL_CURRENT_LOOP_KP) +
                 ((s_POWER_i_err0 - s_POWER_i_err1) * POWER_CTRL_CURRENT_LOOP_KD);
    s_POWER_currentIntegral += s_POWER_i_err0 * POWER_CTRL_CURRENT_LOOP_KI;

    if (s_POWER_currentIntegral > (int32_t)BSP_POWER_ADC1_FULL_SCALE_RAW)
    {
        s_POWER_currentIntegral = (int32_t)BSP_POWER_ADC1_FULL_SCALE_RAW;
    }
    if (s_POWER_currentIntegral < 0)
    {
        s_POWER_currentIntegral = 0;
    }

    s_POWER_controlValue.vout_ref_raw += s_POWER_i0;
    s_POWER_ctrlCvccMode = POWER_CTRL_CVCC_CC;
    if (s_POWER_controlValue.vout_ref_raw > active_vout_limit_raw)
    {
        s_POWER_controlValue.vout_ref_raw = active_vout_limit_raw;
        s_POWER_ctrlCvccMode = POWER_CTRL_CVCC_CV;
    }

    if (s_POWER_controlValue.vout_ref_raw < 0)
    {
        s_POWER_controlValue.vout_ref_raw = 0;
    }

    s_POWER_v_err0 = s_POWER_controlValue.vout_ref_raw - vout_temp;

    if (s_POWER_stageModeChanged != 0U)
    {
        s_POWER_u1 = 0;
        s_POWER_i0 = 0;
        s_POWER_currentIntegral = 0;
        s_POWER_stageModeChanged = 0U;
    }

    switch (s_POWER_ctrlStageMode)
    {
        case POWER_CTRL_STAGE_NA:
            s_POWER_resetAppPid();
            break;

        case POWER_CTRL_STAGE_BUCK:
            s_POWER_u0 = s_POWER_u1 +
                         (s_POWER_v_err0 * POWER_CTRL_BUCK_PID_B0) +
                         (s_POWER_v_err1 * POWER_CTRL_BUCK_PID_B1) +
                         (s_POWER_v_err2 * POWER_CTRL_BUCK_PID_B2);
            s_POWER_v_err2 = s_POWER_v_err1;
            s_POWER_v_err1 = s_POWER_v_err0;
            s_POWER_u1 = s_POWER_u0;
            if (iout_temp <= POWER_CTRL_BUCK_LIGHT_LOAD_ENTER_RAW)
            {
                s_POWER_setAppBuckLightLoad(1U);
            }
            else if (iout_temp >= POWER_CTRL_BUCK_LIGHT_LOAD_EXIT_RAW)
            {
                s_POWER_setAppBuckLightLoad(0U);
            }
            s_POWER_controlValue.boost_duty_tick = (s_POWER_buckLightLoad != 0U)
                                                       ? BSP_POWER_BOOST_DUTY_MIN_TICK
                                                       : BSP_POWER_BOOST_DUTY_SYNC_MIN_TICK;
            s_POWER_controlValue.buck_duty_tick = s_POWER_getAppClampDuty((s_POWER_u0 >> 8) * 3,
                                                                          BSP_POWER_BUCK_DUTY_MIN_TICK,
                                                                          (uint32_t)s_POWER_controlValue.buck_max_duty_tick);
            break;

        case POWER_CTRL_STAGE_BOOST:
            s_POWER_setAppBuckLightLoad(0U);
            s_POWER_u0 = s_POWER_u1 +
                         (s_POWER_v_err0 * POWER_CTRL_BOOST_PID_B0) +
                         (s_POWER_v_err1 * POWER_CTRL_BOOST_PID_B1) +
                         (s_POWER_v_err2 * POWER_CTRL_BOOST_PID_B2);
            s_POWER_v_err2 = s_POWER_v_err1;
            s_POWER_v_err1 = s_POWER_v_err0;
            s_POWER_u1 = s_POWER_u0;
            s_POWER_controlValue.buck_duty_tick = BSP_POWER_BUCK_DUTY_MAX_TICK;
            s_POWER_controlValue.boost_duty_tick = s_POWER_getAppClampDuty((s_POWER_u0 >> 8) * 3,
                                                                           BSP_POWER_BOOST_DUTY_MIN_TICK,
                                                                           (uint32_t)s_POWER_controlValue.boost_max_duty_tick);
            break;

        case POWER_CTRL_STAGE_MIX:
        default:
            s_POWER_setAppBuckLightLoad(0U);
            s_POWER_u0 = s_POWER_u1 +
                         (s_POWER_v_err0 * POWER_CTRL_BOOST_PID_B0) +
                         (s_POWER_v_err1 * POWER_CTRL_BOOST_PID_B1) +
                         (s_POWER_v_err2 * POWER_CTRL_BOOST_PID_B2);
            s_POWER_v_err2 = s_POWER_v_err1;
            s_POWER_v_err1 = s_POWER_v_err0;
            s_POWER_u1 = s_POWER_u0;
            s_POWER_i_err1 = s_POWER_i_err0;
            s_POWER_controlValue.buck_duty_tick = BSP_POWER_BUCK_DUTY_SYNC_MAX_TICK;
            s_POWER_controlValue.boost_duty_tick = s_POWER_getAppClampDuty((s_POWER_u0 >> 8) * 3,
                                                                           BSP_POWER_BOOST_DUTY_MIN_TICK,
                                                                           (uint32_t)s_POWER_controlValue.boost_max_duty_tick);
            break;
    }

    if (s_POWER_pwmEnabled == 0U)
    {
        s_POWER_controlValue.buck_duty_tick = BSP_POWER_BUCK_DUTY_MIN_TICK;
    }

    s_POWER_controlValue.current_loop_out = s_POWER_i0;

    __HAL_HRTIM_SETCOMPARE(&hhrtim1,
                           HRTIM_TIMERINDEX_TIMER_A,
                           HRTIM_COMPAREUNIT_1,
                           BSP_POWER_HRTIM_PERIOD_TICK - (uint32_t)s_POWER_controlValue.buck_duty_tick);
    __HAL_HRTIM_SETCOMPARE(&hhrtim1,
                           HRTIM_TIMERINDEX_TIMER_A,
                           HRTIM_COMPAREUNIT_3,
                           __HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1) >> 1);
    __HAL_HRTIM_SETCOMPARE(&hhrtim1,
                           HRTIM_TIMERINDEX_TIMER_D,
                           HRTIM_COMPAREUNIT_1,
                           (uint32_t)s_POWER_controlValue.boost_duty_tick);
}

static void s_POWER_handleAppState(void)
{
    switch (s_POWER_state)
    {
        case POWER_STATE_INIT:
            s_POWER_initAppValues();
            s_POWER_state = POWER_STATE_WAIT;
            break;

        case POWER_STATE_WAIT:
            s_POWER_stopAppPwm();
            if (s_POWER_ctrlEnabled == 0U)
            {
                s_POWER_waitCount = 0U;
                break;
            }

            if (s_POWER_ctrlFaultFlags != POWER_CTRL_FAULT_NONE)
            {
                s_POWER_state = POWER_STATE_ERR;
                break;
            }

            if (s_POWER_vinUvLocked != 0U)
            {
                s_POWER_waitCount = 0U;
                break;
            }

            ++s_POWER_waitCount;
            if (s_POWER_waitCount > POWER_CTRL_WAIT_TICK_COUNT)
            {
                s_POWER_waitCount = 0U;
                s_POWER_buckMaxDutyCount = 0U;
                s_POWER_boostMaxDutyCount = 0U;
                s_POWER_softstartState = POWER_SOFTSTART_INIT;
                s_POWER_state = POWER_STATE_RISE;
            }
            break;

        case POWER_STATE_RISE:
            switch (s_POWER_softstartState)
            {
                case POWER_SOFTSTART_INIT:
                    s_POWER_stopAppPwm();
                    s_POWER_controlValue.buck_max_duty_tick = BSP_POWER_BUCK_DUTY_MIN_TICK;
                    s_POWER_controlValue.boost_max_duty_tick = BSP_POWER_BOOST_DUTY_MIN_TICK;
                    s_POWER_resetAppPid();
                    s_POWER_updateAppReferences();
                    s_POWER_controlValue.vout_softstart_ref_raw = s_POWER_controlValue.vout_set_ref_raw >> 1;
                    s_POWER_softstartState = POWER_SOFTSTART_WAIT;
                    break;

                case POWER_SOFTSTART_WAIT:
                    ++s_POWER_softstartWaitCount;
                    if (s_POWER_softstartWaitCount > POWER_CTRL_SOFTSTART_WAIT_TICK_COUNT)
                    {
                        s_POWER_softstartWaitCount = 0U;
                        s_POWER_controlValue.buck_duty_tick = BSP_POWER_BUCK_DUTY_MIN_TICK;
                        s_POWER_controlValue.buck_max_duty_tick = BSP_POWER_BUCK_DUTY_MIN_TICK;
                        s_POWER_controlValue.boost_duty_tick = BSP_POWER_BOOST_DUTY_MIN_TICK;
                        s_POWER_controlValue.boost_max_duty_tick = BSP_POWER_BOOST_DUTY_MIN_TICK;
                        s_POWER_resetAppPid();
                        s_POWER_controlValue.vout_softstart_ref_raw = s_POWER_controlValue.vout_set_ref_raw >> 1;
                        s_POWER_softstartState = POWER_SOFTSTART_RUN;
                    }
                    break;

                case POWER_SOFTSTART_RUN:
                default:
                    if (s_POWER_pwmEnabled == 0U)
                    {
                        s_POWER_resetAppPid();
                        s_POWER_startAppPwm();
                    }

                    ++s_POWER_buckMaxDutyCount;
                    ++s_POWER_boostMaxDutyCount;
                    s_POWER_controlValue.buck_max_duty_tick = s_POWER_getAppClampDuty(
                        s_POWER_controlValue.buck_max_duty_tick + ((int32_t)s_POWER_buckMaxDutyCount * 15),
                        BSP_POWER_BUCK_DUTY_MIN_TICK,
                        BSP_POWER_BUCK_DUTY_MAX_TICK);
                    s_POWER_controlValue.boost_max_duty_tick = s_POWER_getAppClampDuty(
                        s_POWER_controlValue.boost_max_duty_tick + ((int32_t)s_POWER_boostMaxDutyCount * 15),
                        BSP_POWER_BOOST_DUTY_MIN_TICK,
                        BSP_POWER_BOOST_DUTY_MAX_TICK);

                    if (((uint32_t)s_POWER_controlValue.buck_max_duty_tick == BSP_POWER_BUCK_DUTY_MAX_TICK) &&
                        ((uint32_t)s_POWER_controlValue.boost_max_duty_tick == BSP_POWER_BOOST_DUTY_MAX_TICK))
                    {
                        s_POWER_softstartState = POWER_SOFTSTART_INIT;
                        s_POWER_state = POWER_STATE_RUN;
                    }
                    break;
            }
            break;

        case POWER_STATE_RUN:
            if (s_POWER_ctrlEnabled == 0U)
            {
                s_POWER_state = POWER_STATE_WAIT;
            }
            break;

        case POWER_STATE_ERR:
        default:
            s_POWER_stopAppPwm();
            s_POWER_ctrlStageMode = POWER_CTRL_STAGE_NA;
            if (s_POWER_ctrlFaultFlags == POWER_CTRL_FAULT_NONE)
            {
                s_POWER_state = POWER_STATE_WAIT;
            }
            break;
    }
}

void POWER_initAppCtrl(void)
{
    memset(&s_POWER_controlValue, 0, sizeof(s_POWER_controlValue));
    memset(&s_POWER_sample, 0, sizeof(s_POWER_sample));
    memset(&s_POWER_fastAdcResult, 0, sizeof(s_POWER_fastAdcResult));
    s_POWER_ctrlSettings.set_voltage_mv = POWER_CTRL_DEFAULT_SET_VOLTAGE_MV;
    s_POWER_ctrlSettings.set_current_ma = POWER_CTRL_DEFAULT_SET_CURRENT_MA;
    s_POWER_ctrlSettings.otp_set_mc = POWER_CTRL_DEFAULT_OTP_SET_MC;
    s_POWER_ctrlSettings.ovp_set_mv = POWER_CTRL_DEFAULT_OVP_SET_MV;
    s_POWER_ctrlSettings.ocp_set_ma = POWER_CTRL_DEFAULT_OCP_SET_MA;
    s_POWER_ctrlSettings.fan_set_permille = 0U;
    s_POWER_setAppFan(s_POWER_ctrlSettings.fan_set_permille);
    s_POWER_ctrlEnabled = 0U;
    s_POWER_state = POWER_STATE_INIT;
    s_POWER_softstartState = POWER_SOFTSTART_INIT;
    s_POWER_ctrlCvccMode = POWER_CTRL_CVCC_CV;
    s_POWER_adcAverageInitialized = 0U;
    s_POWER_vinUvLocked = 1U;
    s_POWER_resetAppCounters();
    s_POWER_initAppValues();
}

void POWER_getAppSnapshot(POWER_ctrlSnapshot_t *snapshot)
{
    BSP_adcResult_t adc_result;
    BSP_powerMeasurement_t measurement;
    float inner_current_a = 0.0F;

    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    BSP_getAppAdcResult(&adc_result);
    if (s_POWER_adcAverageInitialized != 0U)
    {
        adc_result.vin_raw = s_POWER_sample.vin_raw_avg;
        adc_result.iin_raw = s_POWER_sample.iin_raw_avg;
        adc_result.vout_raw = s_POWER_sample.vout_raw_avg;
        adc_result.iout_raw = s_POWER_sample.iout_raw_avg;
        measurement = s_POWER_sample.measurement;
    }
    else
    {
        BSP_getAppMeasurement(&adc_result, &measurement);
    }

    snapshot->input_voltage_mv = s_POWER_getAppU32Nonnegative(measurement.vin_v, 1000.0F);
    snapshot->input_current_ma = s_POWER_getAppU32Nonnegative(measurement.iin_a, 1000.0F);
    snapshot->output_voltage_mv = s_POWER_getAppU32Nonnegative(measurement.vout_v, 1000.0F);
    snapshot->output_current_ma = s_POWER_getAppU32Nonnegative(measurement.iout_a, 1000.0F);
    snapshot->core_temperature_mc = s_POWER_getAppI32(measurement.die_temp_c, 1000.0F);
    snapshot->board_temperature_mc = s_POWER_getAppI32(measurement.temp1_c, 1000.0F);
    snapshot->temp2_temperature_mc = s_POWER_getAppI32(measurement.temp2_c, 1000.0F);

    snapshot->input_voltage_raw = adc_result.vin_raw;
    snapshot->input_current_raw = adc_result.iin_raw;
    snapshot->output_voltage_raw = adc_result.vout_raw;
    snapshot->output_current_raw = adc_result.iout_raw;

    snapshot->settings = s_POWER_ctrlSettings;
    snapshot->power_enabled = s_POWER_ctrlEnabled;
    snapshot->fault_flags = s_POWER_ctrlFaultFlags;
    snapshot->state_flag_bits = s_POWER_getAppStateFlagBits();
    snapshot->stage_mode = (s_POWER_vinUvLocked != 0U) ? POWER_CTRL_STAGE_NA : s_POWER_ctrlStageMode;
    snapshot->cvcc_mode = s_POWER_ctrlCvccMode;

    snapshot->otp_value_mc = snapshot->board_temperature_mc;
    snapshot->ovp_value_mv = snapshot->output_voltage_mv;
    snapshot->ocp_value_ma = snapshot->output_current_ma;

    snapshot->duty_cmd = s_POWER_getAppDutyCmdTick();
    snapshot->pwm_a_compare = __HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1);
    snapshot->pwm_d_compare = __HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1);
    snapshot->fan_speed_permille = s_POWER_fanSpeedPermille;
    if (snapshot->stage_mode != POWER_CTRL_STAGE_NA)
    {
        inner_current_a = BSP_getAppInnerCurrentA(&measurement, s_POWER_getAppBspStageMode(snapshot->stage_mode));
    }
    snapshot->loop_current_feedback_ma = s_POWER_getAppU32Nonnegative(inner_current_a, 1000.0F);
    snapshot->loop_current_reference_ma = s_POWER_ctrlSettings.set_current_ma;
    snapshot->voltage_loop_reference_mv = s_POWER_getAppMillivoltsFromVoutRaw(s_POWER_controlValue.vout_ref_raw);
}

void POWER_applyAppSettings(const POWER_ctrlSettings_t *settings)
{
    if (settings == NULL)
    {
        return;
    }

    s_POWER_ctrlSettings = *settings;
    if (s_POWER_ctrlSettings.fan_set_permille > 1000U)
    {
        s_POWER_ctrlSettings.fan_set_permille = 1000U;
    }
    s_POWER_updateAppReferences();
    s_POWER_setAppFan(s_POWER_ctrlSettings.fan_set_permille);
}

void POWER_setAppEnabled(uint8_t enabled)
{
    if (enabled != 0U)
    {
        s_POWER_ctrlEnabled = 1U;
        s_POWER_softstartState = POWER_SOFTSTART_INIT;

        if (s_POWER_ctrlFaultFlags == POWER_CTRL_FAULT_NONE)
        {
            s_POWER_state = POWER_STATE_WAIT;
            s_POWER_resetAppPid();
        }
        return;
    }

    s_POWER_ctrlEnabled = 0U;
    s_POWER_state = POWER_STATE_WAIT;
    s_POWER_stopAppPwm();
}

void POWER_runAppControlTick(void)
{
    BSP_getAppAdcResult(&s_POWER_fastAdcResult);

    ++s_POWER_slowLoopDivider;
    if (s_POWER_slowLoopDivider >= POWER_CTRL_SLOW_LOOP_DIVIDER)
    {
        s_POWER_slowLoopDivider = 0U;
        s_POWER_sampleAppAdc();
        s_POWER_updateAppVinUvLock();
        s_POWER_checkAppProtection();
        s_POWER_updateAppFaultRecovery();
        s_POWER_handleAppState();
        s_POWER_updateAppMode();
        s_POWER_updateAppFan();
        s_POWER_updateAppStatusLed();
    }

    s_POWER_runAppPid();
}
