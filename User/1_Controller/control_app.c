#include "control_app.h"
#include "power_mode.h"
#include "pwm_control.h"
#include "protection_control.h"
#include "temperature_sensor.h"
#include "control_config.h"
#include "Loop/voltage_loop.h"
#include "Loop/current_loop.h"
#include "Loop/current_loop_schedule.h"
#include "../2_Storager/Driver/Flash_Init.h"
#include "../2_Storager/Param/param_manager.h"
#include "../2_Storager/Param/param_uf4com.h"
#include "../3_Communicator/uf4com/protocol/Inc/uf4com.h"
#include "../3_Communicator/uf4com/transport/uf4com_uart/Inc/uf4com_usrt.h"
#include "../3_Communicator/uf4com/protocol/Inc/uf4com_cmd.h"
#include "../3_Communicator/uf4com/register/Inc/uf4com_id_table.h"
#include "hrtim.h"
#include "usart.h"
#include "adc.h"
#include "tim.h"
#include "iwdg.h"
#include "stm32g4xx_hal.h"
#include <string.h>

typedef struct
{
    uint16_t vset_10mV, iset_10mA, ocp_10mA, otp_centi_c;
    uint16_t vin_ovp_10mV, vin_uvp_10mV, vout_ovp_10mV;
    uint16_t direction_mode, adc_vref_10mV;
    uint16_t vin_r_upper, vin_r_lower, vout_r_upper, vout_r_lower;
    uint16_t shunt_in, shunt_out, gain_in, gain_out, current_offset;
    uint16_t vin_gain, vin_offset, vout_gain, vout_offset;
    uint16_t iin_gain, iin_offset, iout_gain, iout_offset, fan_set_permille;
    uint8_t direction, enable;
} control_settings_t;
static control_settings_t s_settings;
static power_mode_t s_mode;
static volatile uint8_t s_save_pending;
static uint8_t s_rx_byte;
static uint8_t s_rx_ring[256];
static volatile uint8_t s_rx_head, s_rx_tail;
static uint16_t s_input_voltage_value, s_input_voltage_raw_value;
static uint16_t s_input_current_value, s_input_current_raw_value;
static uint16_t s_output_voltage_value, s_output_voltage_raw_value;
static uint16_t s_state_value, s_fault_value, s_flags_value, s_direction_value;
static uint16_t s_cc_cv_value, s_converter_mode_value;
static uint16_t s_fmac_first_value, s_fmac_second_value, s_fmac_stage_value, s_fmac_status_value;
static uint16_t s_dac3_value, s_dac4_value, s_fan_speed_value;
static uint16_t s_isr_last_value, s_isr_max_value, s_isr_overrun_value;
static uint16_t s_state_machine_value, s_self_check_value, s_latched_fault_value;
static uint16_t s_core_temperature_value, s_temp1_value, s_temp2_value;
static uint16_t s_input_current_raw_value, s_output_current_raw_value;
static uint16_t s_output_current_value, s_current_feedback_value;
static uint16_t s_current_reference_value, s_voltage_current_reference_value;
static uint16_t s_duty_command_value;
static uint8_t s_tx_buffer[UF4_MAX_FRAME_LEN];
static volatile uint16_t s_adc_samples[4];
static volatile uint16_t s_adc2_sample, s_adc3_sample, s_adc4_sample;
static volatile uint16_t s_adc5_samples[2];
static volatile uint32_t s_adc1_last_sample_tick;
static volatile uint32_t s_adc1_sample_count;
static volatile uint32_t s_adc5_sample_count, s_adc5_last_sample_tick;
static volatile uint32_t s_fast_tick_count, s_outer_tick_count;
static volatile uint16_t s_fast_early_max_cycles;
static volatile uint32_t s_input_voltage_mV, s_output_voltage_mV;
static volatile uint16_t s_current_reference_mA;
static volatile int16_t s_current_b0_q15;
static volatile uint16_t s_protection_zero_adc;
static uint16_t s_zero_candidate_adc;
static uint32_t s_zero_candidate_tick;
static volatile uint8_t s_control_ready, s_fast_enabled, s_adc5_ready;
static volatile power_fault_reason_t s_pending_fault;
static control_direction_t s_fast_direction;
static uint32_t s_last_frame_tick, s_adc5_start_tick;
static uint32_t s_adc5_processed_count;
static uint8_t s_previous_enable;

#define CONTROL_COMM_TIMEOUT_MS 500u
#define CONTROL_ADC_TIMEOUT_MS 20u
#define CONTROL_ZERO_DRIFT_LIMIT_ADC 64u
#define CONTROL_ZERO_STABLE_LIMIT_ADC 16u
#define CONTROL_ZERO_SETTLE_MS 50u
#define CONTROL_ISR_BUDGET_CYCLES (SystemCoreClock / CONTROL_CURRENT_LOOP_HZ)

static bool current_zero_stable(void)
{
    int32_t delta = (int32_t)s_adc4_sample - s_protection_zero_adc;
    return delta >= -(int32_t)CONTROL_ZERO_DRIFT_LIMIT_ADC &&
           delta <= (int32_t)CONTROL_ZERO_DRIFT_LIMIT_ADC;
}

static uint32_t voltage_mV(uint16_t raw, uint16_t upper, uint16_t lower,
                           uint16_t gain_centi, uint16_t offset_centi_v)
{
    uint64_t numerator = (uint64_t)raw * s_settings.adc_vref_10mV * 10u *
                         ((uint32_t)upper + lower) * gain_centi;
    return lower == 0u ? 0u :
        (uint32_t)(numerator / (4095ULL * lower * 100u)) +
        (uint32_t)offset_centi_v * 10u;
}

static int32_t current_mA(uint16_t raw, uint16_t vref, bool reverse,
                          uint16_t shunt, uint16_t gain)
{
    int32_t delta = reverse ? (int32_t)raw - vref : (int32_t)vref - raw;
    int64_t numerator = (int64_t)delta * s_settings.adc_vref_10mV * 10u * 100000000LL;
    int64_t denominator = (int64_t)4095 * shunt * 10u * gain;
    if (denominator == 0) return 0;
    numerator /= denominator;
    if (numerator > INT32_MAX) return INT32_MAX;
    if (numerator < INT32_MIN) return INT32_MIN;
    return (int32_t)numerator;
}

static uint16_t centi_unit(int32_t milli)
{
    if (milli <= 0) return 0u;
    if (milli >= 655350) return 65535u;
    return (uint16_t)(milli / 10);
}

static uint16_t calibrated_current_centi(int32_t current, uint16_t gain,
                                         uint16_t offset)
{
    int64_t adjusted = (int64_t)current * gain / 100u + (int32_t)offset * 10;
    if (adjusted <= 0) return 0u;
    if (adjusted >= 655350) return 65535u;
    return (uint16_t)(adjusted / 10);
}

static protection_settings_t protection_settings(void)
{
    protection_settings_t settings;
    settings.ocp_10mA=s_settings.ocp_10mA;
    settings.adc_vref_10mV=s_settings.adc_vref_10mV;
    settings.current_zero_adc=s_protection_zero_adc != 0u ?
                              s_protection_zero_adc : s_adc4_sample;
    settings.vin_ovp_10mV=s_settings.vin_ovp_10mV;
    settings.vout_ovp_10mV=s_settings.vout_ovp_10mV;
    settings.vin_r_upper_100ohm=s_settings.vin_r_upper;
    settings.vin_r_lower_100ohm=s_settings.vin_r_lower;
    settings.vout_r_upper_100ohm=s_settings.vout_r_upper;
    settings.vout_r_lower_100ohm=s_settings.vout_r_lower;
    settings.shunt_in_10uohm=s_settings.shunt_in;
    settings.shunt_out_10uohm=s_settings.shunt_out;
    settings.gain_in_centi=s_settings.gain_in;
    settings.gain_out_centi=s_settings.gain_out;
    return settings;
}

#define APP_RW_U16(id,name,group,min,max,def,ptr) \
    {id,name,PARAM_TYPE_U16,group,PARAM_FLAG_READ|PARAM_FLAG_WRITE|PARAM_FLAG_PERSISTENT,PARAM_STORAGE_INTERNAL,min,max,def,2,ptr,NULL,NULL,NULL}
#define APP_RO_U16(id,name,group,min,max,ptr) \
    {id,name,PARAM_TYPE_U16,group,PARAM_FLAG_READ|PARAM_FLAG_RUNTIME,PARAM_STORAGE_INTERNAL,min,max,0,2,ptr,NULL,NULL,NULL}
#define APP_RW_U8_RUNTIME(id,name,group,ptr) \
    {id,name,PARAM_TYPE_U8,group,PARAM_FLAG_READ|PARAM_FLAG_WRITE|PARAM_FLAG_RUNTIME,PARAM_STORAGE_INTERNAL,0,1,0,1,ptr,NULL,NULL,NULL}

static const ParamDescriptor_t s_param_table[] = {
    APP_RW_U16(UF4_ID_SET_VOLTAGE_LIMIT,"set_voltage_10mV",PARAM_GROUP_OUTPUT,0,7000,500,&s_settings.vset_10mV),
    APP_RW_U16(UF4_ID_SET_CURRENT_LIMIT,"set_current_10mA",PARAM_GROUP_OUTPUT,0,1000,100,&s_settings.iset_10mA),
    APP_RW_U16(UF4_ID_OCP_SET_VALUE,"ocp_10mA",PARAM_GROUP_PROTECTION,1,1000,1000,&s_settings.ocp_10mA),
    APP_RW_U16(UF4_ID_OTP_SET_VALUE,"otp_centi_c",PARAM_GROUP_PROTECTION,4000,15000,12000,&s_settings.otp_centi_c),
    APP_RW_U16(UF4_ID_CFG_VIN_OVP,"vin_ovp_10mV",PARAM_GROUP_PROTECTION,1,7000,7000,&s_settings.vin_ovp_10mV),
    APP_RW_U16(UF4_ID_CFG_VIN_UVP,"vin_uvp_10mV",PARAM_GROUP_PROTECTION,0,7000,1200,&s_settings.vin_uvp_10mV),
    APP_RW_U16(UF4_ID_CFG_VOUT_OVP,"vout_ovp_10mV",PARAM_GROUP_PROTECTION,1,7000,7000,&s_settings.vout_ovp_10mV),
    APP_RW_U16(UF4_ID_CFG_POWER_DIRECTION_MODE,"direction_mode",PARAM_GROUP_OUTPUT,0,1,1,&s_settings.direction_mode),
    APP_RW_U16(UF4_ID_CFG_ADC_VREF,"adc_vref_10mV",PARAM_GROUP_CALIBRATION,250,360,330,&s_settings.adc_vref_10mV),
    APP_RW_U16(UF4_ID_CFG_VIN_R_UPPER,"vin_r_upper_100ohm",PARAM_GROUP_CALIBRATION,1,65535,750,&s_settings.vin_r_upper),
    APP_RW_U16(UF4_ID_CFG_VIN_R_LOWER,"vin_r_lower_100ohm",PARAM_GROUP_CALIBRATION,1,65535,30,&s_settings.vin_r_lower),
    APP_RW_U16(UF4_ID_CFG_VOUT_R_UPPER,"vout_r_upper_100ohm",PARAM_GROUP_CALIBRATION,1,65535,750,&s_settings.vout_r_upper),
    APP_RW_U16(UF4_ID_CFG_VOUT_R_LOWER,"vout_r_lower_100ohm",PARAM_GROUP_CALIBRATION,1,65535,30,&s_settings.vout_r_lower),
    APP_RW_U16(UF4_ID_CFG_SHUNT_IN,"shunt_in_10uohm",PARAM_GROUP_CALIBRATION,1,65535,800,&s_settings.shunt_in),
    APP_RW_U16(UF4_ID_CFG_SHUNT_OUT,"shunt_out_10uohm",PARAM_GROUP_CALIBRATION,1,65535,800,&s_settings.shunt_out),
    APP_RW_U16(UF4_ID_CFG_GAIN_IN,"gain_in_centi",PARAM_GROUP_CALIBRATION,2000,2000,2000,&s_settings.gain_in),
    APP_RW_U16(UF4_ID_CFG_GAIN_OUT,"gain_out_centi",PARAM_GROUP_CALIBRATION,2000,2000,2000,&s_settings.gain_out),
    APP_RW_U16(UF4_ID_CFG_CURRENT_OFFSET,"current_offset_centi",PARAM_GROUP_CALIBRATION,0,65535,0,&s_settings.current_offset),
    APP_RW_U16(UF4_ID_CFG_VIN_GAIN,"vin_gain_centi",PARAM_GROUP_CALIBRATION,10,300,100,&s_settings.vin_gain),
    APP_RW_U16(UF4_ID_CFG_VIN_OFFSET,"vin_offset_centi_v",PARAM_GROUP_CALIBRATION,0,65535,0,&s_settings.vin_offset),
    APP_RW_U16(UF4_ID_CFG_VOUT_GAIN,"vout_gain_centi",PARAM_GROUP_CALIBRATION,10,300,100,&s_settings.vout_gain),
    APP_RW_U16(UF4_ID_CFG_VOUT_OFFSET,"vout_offset_centi_v",PARAM_GROUP_CALIBRATION,0,65535,0,&s_settings.vout_offset),
    APP_RW_U16(UF4_ID_CFG_IIN_GAIN,"iin_gain_centi",PARAM_GROUP_CALIBRATION,10,300,100,&s_settings.iin_gain),
    APP_RW_U16(UF4_ID_CFG_IIN_OFFSET,"iin_offset_centi_a",PARAM_GROUP_CALIBRATION,0,65535,0,&s_settings.iin_offset),
    APP_RW_U16(UF4_ID_CFG_IOUT_GAIN,"iout_gain_centi",PARAM_GROUP_CALIBRATION,10,300,100,&s_settings.iout_gain),
    APP_RW_U16(UF4_ID_CFG_IOUT_OFFSET,"iout_offset_centi_a",PARAM_GROUP_CALIBRATION,0,65535,0,&s_settings.iout_offset),
    APP_RW_U16(UF4_ID_FAN_SET_VALUE,"fan_set_permille",PARAM_GROUP_SYSTEM,0,1000,1000,&s_settings.fan_set_permille),
    APP_RW_U8_RUNTIME(UF4_ID_POWER_DIRECTION,"power_direction",PARAM_GROUP_OUTPUT,&s_settings.direction),
    APP_RW_U8_RUNTIME(UF4_ID_OUTPUT_ENABLE,"output_enable",PARAM_GROUP_OUTPUT,&s_settings.enable),
    APP_RO_U16(UF4_ID_INPUT_VOLTAGE,"input_voltage_centi_v",PARAM_GROUP_SYSTEM,0,65535,&s_input_voltage_value),
    APP_RO_U16(UF4_ID_INPUT_VOLTAGE_RAW,"input_voltage_raw",PARAM_GROUP_SYSTEM,0,4095,&s_input_voltage_raw_value),
    APP_RO_U16(UF4_ID_INPUT_CURRENT,"input_current_centi_a",PARAM_GROUP_SYSTEM,0,65535,&s_input_current_value),
    APP_RO_U16(UF4_ID_INPUT_CURRENT_RAW,"input_current_raw",PARAM_GROUP_SYSTEM,0,4095,&s_input_current_raw_value),
    APP_RO_U16(UF4_ID_OUTPUT_VOLTAGE,"output_voltage_centi_v",PARAM_GROUP_SYSTEM,0,65535,&s_output_voltage_value),
    APP_RO_U16(UF4_ID_OUTPUT_VOLTAGE_RAW,"output_voltage_raw",PARAM_GROUP_SYSTEM,0,4095,&s_output_voltage_raw_value),
    APP_RO_U16(UF4_ID_OUTPUT_CURRENT,"output_current_centi_a",PARAM_GROUP_SYSTEM,0,65535,&s_output_current_value),
    APP_RO_U16(UF4_ID_OUTPUT_CURRENT_RAW,"output_current_raw",PARAM_GROUP_SYSTEM,0,4095,&s_output_current_raw_value),
    APP_RO_U16(UF4_ID_DUTY_CMD,"duty_permille",PARAM_GROUP_SYSTEM,0,1000,&s_duty_command_value),
    APP_RO_U16(UF4_ID_FMAC_SELFTEST_FIRST_RAW,"fmac_first_raw",PARAM_GROUP_SYSTEM,0,65535,&s_fmac_first_value),
    APP_RO_U16(UF4_ID_LOOP_CURRENT_FEEDBACK,"loop_current_feedback_centi_a",PARAM_GROUP_SYSTEM,0,65535,&s_current_feedback_value),
    APP_RO_U16(UF4_ID_LOOP_CURRENT_REFERENCE,"loop_current_reference_centi_a",PARAM_GROUP_SYSTEM,0,65535,&s_current_reference_value),
    APP_RO_U16(UF4_ID_VOLTAGE_LOOP_CURRENT_REFERENCE,"voltage_loop_reference_centi_a",PARAM_GROUP_SYSTEM,0,65535,&s_voltage_current_reference_value),
    APP_RO_U16(UF4_ID_FMAC_SELFTEST_SECOND_RAW,"fmac_second_raw",PARAM_GROUP_SYSTEM,0,65535,&s_fmac_second_value),
    APP_RO_U16(UF4_ID_FMAC_DIAGNOSTIC_STAGE,"fmac_diagnostic_stage",PARAM_GROUP_SYSTEM,0,65535,&s_fmac_stage_value),
    APP_RO_U16(UF4_ID_FMAC_DIAGNOSTIC_STATUS,"fmac_diagnostic_status",PARAM_GROUP_SYSTEM,0,65535,&s_fmac_status_value),
    APP_RO_U16(UF4_ID_DAC3_CH1_OUTPUT_CODE,"dac3_ch1_output",PARAM_GROUP_SYSTEM,0,65535,&s_dac3_value),
    APP_RO_U16(UF4_ID_DAC4_CH1_OUTPUT_CODE,"dac4_ch1_output",PARAM_GROUP_SYSTEM,0,65535,&s_dac4_value),
    APP_RO_U16(UF4_ID_SELF_CHECK_STATUS,"self_check_status",PARAM_GROUP_SYSTEM,0,65535,&s_self_check_value),
    APP_RO_U16(UF4_ID_LATCHED_FAULT_REASON,"latched_fault_reason",PARAM_GROUP_SYSTEM,0,65535,&s_latched_fault_value),
    APP_RO_U16(UF4_ID_POWER_STATE,"power_state",PARAM_GROUP_SYSTEM,0,65535,&s_state_value),
    APP_RO_U16(UF4_ID_FAULT_STATE,"fault_state",PARAM_GROUP_SYSTEM,0,65535,&s_fault_value),
    APP_RO_U16(UF4_ID_STATE_MACHINE_FLAG_BITS,"protection_status",PARAM_GROUP_SYSTEM,0,65535,&s_flags_value),
    APP_RO_U16(UF4_ID_CC_CV_MODE,"cc_cv_mode",PARAM_GROUP_SYSTEM,0,65535,&s_cc_cv_value),
    APP_RO_U16(UF4_ID_POWER_DIRECTION_STATUS,"direction_status",PARAM_GROUP_SYSTEM,0,65535,&s_direction_value),
    APP_RO_U16(UF4_ID_POWER_CONVERTER_MODE,"converter_mode",PARAM_GROUP_SYSTEM,0,65535,&s_converter_mode_value),
    APP_RO_U16(UF4_ID_CONTROL_ISR_LAST_CYCLES,"control_isr_last_cycles",PARAM_GROUP_SYSTEM,0,65535,&s_isr_last_value),
    APP_RO_U16(UF4_ID_CONTROL_ISR_MAX_CYCLES,"control_isr_max_cycles",PARAM_GROUP_SYSTEM,0,65535,&s_isr_max_value),
    APP_RO_U16(UF4_ID_CONTROL_ISR_OVERRUN_COUNT,"control_isr_overrun_count",PARAM_GROUP_SYSTEM,0,65535,&s_isr_overrun_value),
    APP_RO_U16(UF4_ID_FAN_SPEED,"fan_speed_rpm",PARAM_GROUP_SYSTEM,0,65535,&s_fan_speed_value),
    APP_RO_U16(UF4_ID_CORE_TEMPERATURE,"core_temperature_centi_c",PARAM_GROUP_SYSTEM,0,65535,&s_core_temperature_value),
    APP_RO_U16(UF4_ID_TEMP1_TEMPERATURE,"temp1_temperature_centi_c",PARAM_GROUP_SYSTEM,0,65535,&s_temp1_value),
    APP_RO_U16(UF4_ID_TEMP2_TEMPERATURE,"temp2_temperature_centi_c",PARAM_GROUP_SYSTEM,0,65535,&s_temp2_value)
};

static void tx_uart(const uint8_t *data, uint16_t len, void *user)
{
    UART_HandleTypeDef *uart=(UART_HandleTypeDef *)user;
    if (uart && data && len && len <= sizeof(s_tx_buffer) &&
        HAL_UART_GetState(uart) == HAL_UART_STATE_READY)
    {
        memcpy(s_tx_buffer, data, len);
        (void)HAL_UART_Transmit_IT(uart, s_tx_buffer, len);
    }
}
static void save_request(void *user) { (void)user; s_save_pending=1u; }
static void write_applied(void *user)
{
    (void)user;
    const protection_settings_t settings=protection_settings();
    if (!ProtectionControl_Apply(&settings))
    {
        PowerMode_ReportFault(&s_mode,POWER_FAULT_EXTERNAL);
        return;
    }
    s_protection_zero_adc = settings.current_zero_adc;
    if (!ProtectionControl_HasCurrentHeadroom())
        PowerMode_ReportFault(&s_mode,POWER_FAULT_EXTERNAL);
    /* A fault requires an explicit enable 1 -> 0 command after the trip clears. */
    if (s_mode.state == POWER_MODE_FAULT && s_previous_enable != 0u &&
        s_settings.enable == 0u && ProtectionControl_HasCurrentHeadroom() &&
        current_zero_stable() && !ProtectionControl_IsTripActive() &&
        (uint32_t)(HAL_GetTick() - s_adc1_last_sample_tick) <= CONTROL_ADC_TIMEOUT_MS)
        PowerMode_ClearFault(&s_mode,HAL_GetTick());
    s_previous_enable = s_settings.enable;
    PowerMode_RequestEnable(&s_mode,s_settings.enable!=0u);
    PowerMode_RequestDirection(&s_mode,
        (s_settings.direction_mode == 0u) ? CONTROL_DIRECTION_FORWARD :
        (control_direction_t)s_settings.direction);
    VoltageLoop_SetCurrentLimit_mA((uint16_t)(s_settings.iset_10mA * 10u));
}

static void update_protection_zero(uint32_t now)
{
    uint16_t zero = s_adc4_sample;
    int32_t candidate_delta = (int32_t)zero - s_zero_candidate_adc;
    uint32_t i;

    if (candidate_delta < -(int32_t)CONTROL_ZERO_STABLE_LIMIT_ADC ||
        candidate_delta > (int32_t)CONTROL_ZERO_STABLE_LIMIT_ADC)
    {
        s_zero_candidate_adc = zero;
        s_zero_candidate_tick = now;
        return;
    }
    if (s_settings.enable != 0u || PowerMode_IsPwmEnabled(&s_mode) ||
        zero == 0u || current_zero_stable() ||
        (uint32_t)(now - s_zero_candidate_tick) < CONTROL_ZERO_SETTLE_MS ||
        (uint32_t)(now - s_adc1_last_sample_tick) > CONTROL_ADC_TIMEOUT_MS)
        return;
    for (i = 0u; i < 4u; ++i)
    {
        int32_t delta = (int32_t)s_adc_samples[i] - zero;
        if (delta < -(int32_t)CONTROL_ZERO_DRIFT_LIMIT_ADC ||
            delta > (int32_t)CONTROL_ZERO_DRIFT_LIMIT_ADC)
            return;
    }
    {
        protection_settings_t settings = protection_settings();
        settings.current_zero_adc = zero;
        if (!ProtectionControl_Apply(&settings))
        {
            PowerMode_ReportFault(&s_mode, POWER_FAULT_EXTERNAL);
            return;
        }
        s_protection_zero_adc = zero;
    }
}
static void pwm_on(void *user)
{
    (void)user;
    if (!s_control_ready || !ProtectionControl_HasCurrentHeadroom() ||
        !current_zero_stable() || ProtectionControl_IsTripActive() ||
        s_input_voltage_mV < CONTROL_VIN_MIN_MV ||
        s_input_voltage_mV > CONTROL_VIN_MAX_MV ||
        s_output_voltage_mV > CONTROL_VOUT_MAX_MV ||
        s_current_b0_q15 == 0 ||
        s_temp1_value == TEMPERATURE_SENSOR_INVALID ||
        s_temp2_value == TEMPERATURE_SENSOR_INVALID ||
        s_temp1_value >= s_settings.otp_centi_c ||
        s_temp2_value >= s_settings.otp_centi_c ||
        (uint32_t)(HAL_GetTick() - s_adc1_last_sample_tick) > CONTROL_ADC_TIMEOUT_MS ||
        (uint32_t)(HAL_GetTick() - s_adc5_last_sample_tick) > 500u ||
        (uint32_t)(HAL_GetTick() - s_last_frame_tick) > CONTROL_COMM_TIMEOUT_MS)
    {
        PowerMode_ReportFault(&s_mode, POWER_FAULT_ADC);
        return;
    }
    if (PwmControl_Enable() != HAL_OK)
        PowerMode_ReportFault(&s_mode, POWER_FAULT_PWM);
}
static void pwm_off(void *user) { (void)user; (void)PwmControl_Disable(); }
static bool self_check(void *user)
{
    (void)user;
    return ProtectionControl_IsReady() && ProtectionControl_HasCurrentHeadroom() &&
           current_zero_stable() &&
           s_adc1_sample_count != 0u &&
           s_adc4_sample > 1000u && s_adc4_sample < 3000u &&
           s_adc5_sample_count != 0u &&
           s_temp1_value != TEMPERATURE_SENSOR_INVALID &&
           s_temp2_value != TEMPERATURE_SENSOR_INVALID &&
           s_temp1_value < s_settings.otp_centi_c &&
           s_temp2_value < s_settings.otp_centi_c &&
           !ProtectionControl_IsTripActive() &&
           (uint32_t)(HAL_GetTick()-s_adc1_last_sample_tick) <= CONTROL_ADC_TIMEOUT_MS;
}
static void update_telemetry(void)
{
    const current_loop_result_t *current=CurrentLoop_GetLastResult();
    const voltage_loop_result_t *voltage=VoltageLoop_GetLastResult();
    uint16_t input_index=(PowerMode_GetDirection(&s_mode)==CONTROL_DIRECTION_FORWARD)?0u:1u;
    uint16_t output_index=(PowerMode_GetDirection(&s_mode)==CONTROL_DIRECTION_FORWARD)?3u:2u;
    s_state_value=(uint16_t)PowerMode_GetState(&s_mode);
    s_fault_value=(uint16_t)PowerMode_GetFault(&s_mode);
    s_flags_value=(uint16_t)(PowerMode_GetFlagBits(&s_mode) |
                             (ProtectionControl_GetStatus() << 8));
    s_state_machine_value=s_state_value;
    s_direction_value=(uint16_t)PowerMode_GetDirection(&s_mode);
    s_self_check_value=s_mode.self_check_passed ? 1u : 0u;
    s_latched_fault_value=s_fault_value;
    s_input_voltage_raw_value=(PowerMode_GetDirection(&s_mode)==CONTROL_DIRECTION_FORWARD)?
        s_adc2_sample:s_adc3_sample;
    s_output_voltage_raw_value=(PowerMode_GetDirection(&s_mode)==CONTROL_DIRECTION_FORWARD)?
        s_adc3_sample:s_adc2_sample;
    s_input_voltage_value=centi_unit((int32_t)s_input_voltage_mV);
    s_output_voltage_value=centi_unit((int32_t)s_output_voltage_mV);
    s_input_current_raw_value=s_adc_samples[input_index];
    s_output_current_raw_value=s_adc_samples[output_index];
    {
        bool reverse = PowerMode_GetDirection(&s_mode)==CONTROL_DIRECTION_REVERSE;
        int32_t input = current_mA(s_input_current_raw_value,s_adc4_sample,reverse,
                                   s_settings.shunt_in,s_settings.gain_in);
        int32_t output = current_mA(s_output_current_raw_value,s_adc4_sample,reverse,
                                    s_settings.shunt_out,s_settings.gain_out);
        s_input_current_value=calibrated_current_centi(input,s_settings.iin_gain,
                                                       s_settings.iin_offset);
        s_output_current_value=calibrated_current_centi(output,s_settings.iout_gain,
                                                        s_settings.iout_offset);
    }
    s_current_feedback_value=s_output_current_value;
    s_current_reference_value=(uint16_t)((current->error_mA+current->output_current_mA) <= 0 ? 0 :
        (current->error_mA+current->output_current_mA) / 10);
    s_voltage_current_reference_value=(uint16_t)(voltage->current_reference_mA / 10u);
    s_duty_command_value=(uint16_t)(((uint32_t)current->duty_q15 * 1000u) / 32767u);
    s_dac3_value=ProtectionControl_GetDac3Ch1Code();
    s_dac4_value=ProtectionControl_GetDac4Ch1Code();
}
void ControlApp_Init(void)
{
    ParamStatus_t st;
    power_mode_config_t mode_config;
    uint32_t start_tick;
    CurrentLoop_Init();
    VoltageLoop_Init();
    (void)Flash_Init_RegisterInternal();
    st=Param_Init(s_param_table,sizeof(s_param_table)/sizeof(s_param_table[0]));
    if (st!=PARAM_OK) Error_Handler();
    st=Param_Load();
    if (st==PARAM_ERROR_NOT_FOUND) (void)Param_Commit();
    else if (st!=PARAM_OK) Error_Handler();
    if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK) Error_Handler();
    if (ParamUF4COM_Bind()!=PARAM_OK) Error_Handler();
    UF4_InitEx(NULL,tx_uart,&huart1);
    UF4_SetSaveRequestCallback(save_request,NULL);
    UF4_SetWriteApplyCallback(write_applied,NULL);
    s_control_ready=0u;
    s_fast_enabled=0u;
    s_pending_fault=POWER_FAULT_NONE;
    s_adc1_sample_count=0u;
    s_adc1_last_sample_tick=0u;
    s_adc5_ready=0u;
    s_adc5_sample_count=0u;
    s_previous_enable=0u;
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK ||
        HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK ||
        HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED) != HAL_OK ||
        HAL_ADCEx_Calibration_Start(&hadc4, ADC_SINGLE_ENDED) != HAL_OK ||
        HAL_ADCEx_Calibration_Start(&hadc5, ADC_SINGLE_ENDED) != HAL_OK)
        Error_Handler();
    if (HAL_ADC_Start_DMA(&hadc1,(uint32_t *)s_adc_samples,4u) != HAL_OK ||
        HAL_ADC_Start_DMA(&hadc2,(uint32_t *)&s_adc2_sample,1u) != HAL_OK ||
        HAL_ADC_Start_DMA(&hadc3,(uint32_t *)&s_adc3_sample,1u) != HAL_OK ||
        HAL_ADC_Start_DMA(&hadc4,(uint32_t *)&s_adc4_sample,1u) != HAL_OK)
        Error_Handler();
    __HAL_DMA_DISABLE_IT(&hdma_adc1,DMA_IT_HT);
    __HAL_DMA_DISABLE_IT(&hdma_adc2,DMA_IT_HT | DMA_IT_TC);
    __HAL_DMA_DISABLE_IT(&hdma_adc3,DMA_IT_HT | DMA_IT_TC);
    __HAL_DMA_DISABLE_IT(&hdma_adc4,DMA_IT_HT | DMA_IT_TC);
    if (HAL_ADC_Start_DMA(&hadc5,(uint32_t *)s_adc5_samples,2u) != HAL_OK)
        Error_Handler();
    __HAL_DMA_DISABLE_IT(&hdma_adc5,DMA_IT_HT);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    PwmControl_Init(&hhrtim1);
    start_tick=HAL_GetTick();
    while ((s_adc1_sample_count == 0u || s_adc5_sample_count == 0u ||
            s_adc4_sample == 0u) &&
           (uint32_t)(HAL_GetTick()-start_tick) < 20u)
        (void)HAL_IWDG_Refresh(&hiwdg);
    if (s_adc1_sample_count == 0u || s_adc5_sample_count == 0u ||
        s_adc4_sample == 0u) Error_Handler();
    {
        const protection_settings_t settings=protection_settings();
        if (!ProtectionControl_Init(&settings)) Error_Handler();
        s_protection_zero_adc = settings.current_zero_adc;
    }
    s_temp2_value=TemperatureSensor_FromAdc(s_adc5_samples[0],false);
    s_temp1_value=TemperatureSensor_FromAdc(s_adc5_samples[1],false);
    s_adc5_processed_count=s_adc5_sample_count;
    s_adc5_start_tick=HAL_GetTick();
    s_zero_candidate_adc=s_adc4_sample;
    s_zero_candidate_tick=HAL_GetTick();
    memset(&mode_config,0,sizeof(mode_config));
    mode_config.self_check_ms=50u;
    mode_config.soft_start_ms=100u;
    mode_config.stopping_ms=10u;
    mode_config.direction_deadtime_ms=20u;
    mode_config.pwm_enable=pwm_on;
    mode_config.pwm_disable=pwm_off;
    mode_config.self_check=self_check;
    PowerMode_Init(&s_mode,&mode_config,HAL_GetTick());
    write_applied(NULL);
    s_rx_byte=0u;
    s_rx_head=0u;
    s_rx_tail=0u;
    (void)HAL_UART_Receive_IT(&huart1,&s_rx_byte,1u);
    s_last_frame_tick=HAL_GetTick();
    s_control_ready=1u;
    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK) Error_Handler();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        uint8_t next=(uint8_t)(s_rx_head+1u);
        if (next != s_rx_tail)
        {
            s_rx_ring[s_rx_head]=s_rx_byte;
            s_rx_head=next;
        }
        (void)HAL_UART_Receive_IT(&huart1,&s_rx_byte,1u);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc == &hadc1)
    {
        s_adc1_last_sample_tick=HAL_GetTick();
        s_adc1_sample_count++;
    }
    else if (hadc == &hadc5)
    {
        s_adc5_ready=1u;
        s_adc5_sample_count++;
        s_adc5_last_sample_tick=HAL_GetTick();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
        (void)HAL_UART_Receive_IT(&huart1,&s_rx_byte,1u);
}

void HAL_HRTIM_RepetitionEventCallback(HRTIM_HandleTypeDef *hrtim, uint32_t timer_index)
{
    static uint32_t previous_sample_count;
    static uint8_t missing_samples;
    current_loop_sample_t sample;
    uint32_t start_cycles, vin, vout;
    uint16_t duty;

    if (hrtim != &hhrtim1 || timer_index != HRTIM_TIMERINDEX_MASTER || !s_control_ready)
        return;
    start_cycles = DWT->CYCCNT;
    s_fast_tick_count++;
    if (s_adc1_sample_count == previous_sample_count)
    {
        if (missing_samples < 3u) missing_samples++;
        if (missing_samples >= 3u && s_fast_enabled)
        {
            s_pending_fault = POWER_FAULT_ADC;
            s_fast_enabled = 0u;
            (void)PwmControl_Disable();
        }
    }
    else missing_samples = 0u;
    previous_sample_count = s_adc1_sample_count;

    vin = s_input_voltage_mV;
    vout = s_output_voltage_mV;
    if (vin < CONTROL_VIN_MIN_MV || vin > CONTROL_VIN_MAX_MV ||
        vout > CONTROL_VOUT_MAX_MV || !current_zero_stable() ||
        s_adc4_sample < 1000u || s_adc4_sample > 3000u)
    {
        if (s_fast_enabled)
        {
            s_pending_fault = POWER_FAULT_ADC;
            s_fast_enabled = 0u;
            (void)PwmControl_Disable();
        }
        uint32_t cycles = DWT->CYCCNT - start_cycles;
        if (cycles > s_fast_early_max_cycles)
            s_fast_early_max_cycles = cycles > 65535u ? 65535u : (uint16_t)cycles;
        if (cycles > CONTROL_ISR_BUDGET_CYCLES && s_isr_overrun_value != 65535u)
            s_isr_overrun_value++;
        return;
    }
    sample.direction = s_fast_direction;
    sample.region = (vout <= vin) ? CONTROL_REGION_BUCK : CONTROL_REGION_BOOST;
    sample.vin_mV = vin;
    sample.vout_mV = vout;
    sample.vref_adc = s_adc4_sample;
    sample.adc_vdda_mV = (uint16_t)(s_settings.adc_vref_10mV * 10u);
    sample.b0_q15 = s_current_b0_q15;
    sample.forward_iout_adc = s_adc_samples[3];
    sample.reverse_iout_adc = s_adc_samples[2];
    sample.iref_mA = s_current_reference_mA;
    sample.duty_base_q15 = (uint16_t)((sample.region == CONTROL_REGION_BUCK) ?
        ((uint64_t)vout * 32767u / vin) :
        ((uint64_t)(vout - vin) * 32767u / vout));
    sample.enable = s_fast_enabled && s_pending_fault == POWER_FAULT_NONE;
    PwmControl_SetMode(sample.direction, sample.region);
    duty = sample.enable ? CurrentLoop_Update(&sample) : sample.duty_base_q15;
    if (PwmControl_SetDutyQ15(duty) != HAL_OK && sample.enable)
        s_pending_fault = POWER_FAULT_PWM;
    {
        uint32_t cycles = DWT->CYCCNT - start_cycles;
        s_isr_last_value = (cycles > 65535u) ? 65535u : (uint16_t)cycles;
        if (cycles > s_isr_max_value) s_isr_max_value = s_isr_last_value;
        if (cycles > CONTROL_ISR_BUDGET_CYCLES && s_isr_overrun_value != 65535u)
            s_isr_overrun_value++;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *tim)
{
    voltage_loop_sample_t sample;
    uint32_t vin_pin, vout_pin, elapsed;
    uint16_t current_limit;
    if (tim != &htim6 || !s_control_ready) return;
    s_outer_tick_count++;

    if (ProtectionControl_IsTripActive() && s_fast_enabled)
    {
        uint16_t status = ProtectionControl_GetStatus();
        s_pending_fault = (status & 0x000au) ? POWER_FAULT_OVERVOLTAGE :
                                           POWER_FAULT_OVERCURRENT;
        s_fast_enabled = 0u;
        (void)PwmControl_Disable();
    }
    vin_pin = voltage_mV(s_adc2_sample,s_settings.vin_r_upper,s_settings.vin_r_lower,
                        s_settings.vin_gain,s_settings.vin_offset);
    vout_pin = voltage_mV(s_adc3_sample,s_settings.vout_r_upper,s_settings.vout_r_lower,
                         s_settings.vout_gain,s_settings.vout_offset);
    if (s_fast_direction == CONTROL_DIRECTION_FORWARD)
    {
        s_input_voltage_mV = vin_pin;
        s_output_voltage_mV = vout_pin;
    }
    else
    {
        s_input_voltage_mV = vout_pin;
        s_output_voltage_mV = vin_pin;
    }
    current_limit = (uint16_t)(s_settings.iset_10mA * 10u);
    if (s_mode.state == POWER_MODE_SOFT_START)
    {
        elapsed = HAL_GetTick() - s_mode.state_tick;
        if (elapsed < 100u)
            current_limit = (uint16_t)((uint32_t)current_limit * elapsed / 100u);
    }
    sample.voltage_mV = s_output_voltage_mV;
    sample.reference_mV = (uint32_t)s_settings.vset_10mV * 10u;
    sample.current_limit_mA = current_limit;
    sample.enable = s_fast_enabled && current_limit != 0u &&
                    s_pending_fault == POWER_FAULT_NONE;
    s_current_reference_mA = VoltageLoop_Update(&sample);
    if (s_input_voltage_mV >= CONTROL_VIN_MIN_MV &&
        s_input_voltage_mV <= CONTROL_VIN_MAX_MV &&
        s_output_voltage_mV <= CONTROL_VOUT_MAX_MV)
        s_current_b0_q15 = CurrentLoopSchedule_B0(
            (s_output_voltage_mV <= s_input_voltage_mV) ? CONTROL_REGION_BUCK : CONTROL_REGION_BOOST,
            s_input_voltage_mV, s_output_voltage_mV, s_current_reference_mA);
    else
        s_current_b0_q15 = 0;
}

void ControlApp_Process(void)
{
    static uint8_t was_enabled;
    uint32_t now=HAL_GetTick();
    while (s_rx_tail != s_rx_head)
    {
        if (UF4_InputByte(s_rx_ring[s_rx_tail++]) != 0u)
            s_last_frame_tick = now;
    }
    UF4_ProcessMs(now);
    if (s_adc5_processed_count != s_adc5_sample_count)
    {
        s_temp2_value=TemperatureSensor_FromAdc(s_adc5_samples[0],false);
        s_temp1_value=TemperatureSensor_FromAdc(s_adc5_samples[1],false);
        s_adc5_processed_count=s_adc5_sample_count;
    }
    update_protection_zero(now);
    if (s_pending_fault != POWER_FAULT_NONE)
    {
        PowerMode_ReportFault(&s_mode,s_pending_fault);
        s_pending_fault=POWER_FAULT_NONE;
    }
    if (PowerMode_IsPwmEnabled(&s_mode) &&
        (uint32_t)(now - s_last_frame_tick) > CONTROL_COMM_TIMEOUT_MS)
        PowerMode_ReportFault(&s_mode,POWER_FAULT_COMMUNICATION);
    if (PowerMode_IsPwmEnabled(&s_mode) &&
        (uint32_t)(now - s_adc1_last_sample_tick) > CONTROL_ADC_TIMEOUT_MS)
        PowerMode_ReportFault(&s_mode,POWER_FAULT_ADC);
    if (PowerMode_IsPwmEnabled(&s_mode) && !current_zero_stable())
        PowerMode_ReportFault(&s_mode,POWER_FAULT_ADC);
    if (PowerMode_IsPwmEnabled(&s_mode) &&
        ((uint32_t)(now - s_adc5_last_sample_tick) > 500u ||
         s_temp1_value == TEMPERATURE_SENSOR_INVALID ||
         s_temp2_value == TEMPERATURE_SENSOR_INVALID))
        PowerMode_ReportFault(&s_mode,POWER_FAULT_ADC);
    if (PowerMode_IsPwmEnabled(&s_mode) &&
        (s_temp1_value >= s_settings.otp_centi_c ||
         s_temp2_value >= s_settings.otp_centi_c))
        PowerMode_ReportFault(&s_mode,POWER_FAULT_OVERTEMPERATURE);
    if (PowerMode_IsPwmEnabled(&s_mode) && ProtectionControl_IsTripActive())
    {
        uint16_t status=ProtectionControl_GetStatus();
        PowerMode_ReportFault(&s_mode,(status & 0x000au) ?
            POWER_FAULT_OVERVOLTAGE : POWER_FAULT_OVERCURRENT);
    }
    PowerMode_Process(&s_mode,now);
    s_fast_enabled=PowerMode_IsPwmEnabled(&s_mode);
    s_fast_direction=PowerMode_GetDirection(&s_mode);
    if (was_enabled && !s_fast_enabled)
    {
        CurrentLoop_Reset();
        s_current_reference_mA=0u;
    }
    was_enabled=s_fast_enabled;
    if (s_adc5_ready && (uint32_t)(now-s_adc5_start_tick) >= 100u)
    {
        int32_t core_c;
        s_adc5_ready=0u;
        s_adc5_start_tick=now;
        if (HAL_ADCEx_InjectedStart(&hadc5) == HAL_OK)
        {
            if (HAL_ADCEx_InjectedPollForConversion(&hadc5,2u) == HAL_OK)
            {
                core_c = __HAL_ADC_CALC_TEMPERATURE(
                    (uint32_t)s_settings.adc_vref_10mV * 10u,
                    HAL_ADCEx_InjectedGetValue(&hadc5,ADC_INJECTED_RANK_1),
                    ADC_RESOLUTION_12B);
                s_core_temperature_value = core_c < 0 ? 0u :
                    (core_c > 655 ? 65535u : (uint16_t)(core_c * 100));
            }
            (void)HAL_ADCEx_InjectedStop(&hadc5);
        }
        if (HAL_ADC_Start_DMA(&hadc5,(uint32_t *)s_adc5_samples,2u) != HAL_OK)
            s_adc5_ready=1u;
        else
            __HAL_DMA_DISABLE_IT(&hdma_adc5,DMA_IT_HT);
    }
    update_telemetry();
    if (s_save_pending)
    {
        uf4_error_t result=(Param_Commit()==PARAM_OK)?UF4_OK:UF4_ERR_INTERNAL;
        s_save_pending=0u; UF4_CompleteSave(result);
    }
    (void)HAL_IWDG_Refresh(&hiwdg);
}
