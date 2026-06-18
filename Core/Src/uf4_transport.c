/**
  ******************************************************************************
  * @file    uf4_transport.c
  * @brief   UF4COM transport adapter and full data-ID binding.
  ******************************************************************************
  */
#include "uf4_transport.h"

#include <string.h>

#include "function.h"
#include "hrtim.h"
#include "tim.h"
#include "uf4com.h"
#include "uf4com_id_table.h"
#include "usart.h"
#include "usbd_cdc_if.h"

#define UF4_TRANSPORT_CHANNEL_COUNT 3U
#define UF4_TRANSPORT_STREAM_PERIOD_MS 20U
#define UF4_TRANSPORT_UART1_CDC_MIRROR 1U

typedef struct
{
    UF4Transport_Port port;
    uint8_t tx_buffer[UF4_TRANSPORT_MAX_FRAME_SIZE];
    uint8_t pending_buffer[UF4_TRANSPORT_MAX_FRAME_SIZE];
    uint16_t pending_len;
    uint8_t tx_busy;
} UF4Transport_ChannelContext;

typedef struct
{
    UF4Transport_Port selected_port;
    uint8_t uart1_rx_buffer[UF4_TRANSPORT_UART_RX_DMA_SIZE];
    uint8_t uart2_rx_buffer[UF4_TRANSPORT_UART_RX_DMA_SIZE];
    uint16_t uart1_rx_last_pos;
    uint16_t uart2_rx_last_pos;
    uint8_t fan_manual_enable;
    uint16_t fan_set_permille;
    uint32_t last_fan_apply_tick;
    uint32_t last_stream_tick;
    UF4Transport_ChannelContext channels[UF4_TRANSPORT_CHANNEL_COUNT];
} UF4Transport_Context;

static UF4Transport_Context s_uf4_transport;

static uint16_t s_id_input_voltage;
static uint16_t s_id_input_current;
static uint16_t s_id_output_voltage;
static uint16_t s_id_output_current;
static uint16_t s_id_core_temperature;
static uint16_t s_id_temp1_temperature;
static uint16_t s_id_temp2_temperature;
static uint16_t s_id_set_voltage_limit;
static uint16_t s_id_set_current_limit;
static uint16_t s_id_cc_cv_mode;
static uint16_t s_id_power_state;
static uint16_t s_id_fault_state;
static uint16_t s_id_state_machine_flag_bits;
static uint16_t s_id_state_machine_state;
static uint16_t s_id_input_voltage_raw;
static uint16_t s_id_input_current_raw;
static uint16_t s_id_output_voltage_raw;
static uint16_t s_id_output_current_raw;
static uint16_t s_id_otp_value;
static uint16_t s_id_otp_set_value;
static uint16_t s_id_ovp_value;
static uint16_t s_id_ovp_set_value;
static uint16_t s_id_ocp_value;
static uint16_t s_id_ocp_set_value;
static uint16_t s_id_duty_cmd;
static uint16_t s_id_pwm_a_compare;
static uint16_t s_id_pwm_d_compare;
static uint16_t s_id_fan_speed;
static uint16_t s_id_fan_set_value;
static uint16_t s_id_loop_current_feedback;
static uint16_t s_id_loop_current_reference;
static uint16_t s_id_voltage_loop_current_reference;

static uf4_id_t s_uf4_id_table[] =
{
    {UF4_ID_INPUT_VOLTAGE, &s_id_input_voltage, 0U},
    {UF4_ID_INPUT_CURRENT, &s_id_input_current, 0U},
    {UF4_ID_OUTPUT_VOLTAGE, &s_id_output_voltage, 0U},
    {UF4_ID_OUTPUT_CURRENT, &s_id_output_current, 0U},
    {UF4_ID_CORE_TEMPERATURE, &s_id_core_temperature, 0U},
    {UF4_ID_TEMP1_TEMPERATURE, &s_id_temp1_temperature, 0U},
    {UF4_ID_TEMP2_TEMPERATURE, &s_id_temp2_temperature, 0U},
    {UF4_ID_SET_VOLTAGE_LIMIT, &s_id_set_voltage_limit, 1U},
    {UF4_ID_SET_CURRENT_LIMIT, &s_id_set_current_limit, 1U},
    {UF4_ID_CC_CV_MODE, &s_id_cc_cv_mode, 0U},
    {UF4_ID_POWER_STATE, &s_id_power_state, 1U},
    {UF4_ID_FAULT_STATE, &s_id_fault_state, 0U},
    {UF4_ID_STATE_MACHINE_FLAG_BITS, &s_id_state_machine_flag_bits, 0U},
    {UF4_ID_STATE_MACHINE_STATE, &s_id_state_machine_state, 0U},
    {UF4_ID_INPUT_VOLTAGE_RAW, &s_id_input_voltage_raw, 0U},
    {UF4_ID_INPUT_CURRENT_RAW, &s_id_input_current_raw, 0U},
    {UF4_ID_OUTPUT_VOLTAGE_RAW, &s_id_output_voltage_raw, 0U},
    {UF4_ID_OUTPUT_CURRENT_RAW, &s_id_output_current_raw, 0U},
    {UF4_ID_OTP_VALUE, &s_id_otp_value, 0U},
    {UF4_ID_OTP_SET_VALUE, &s_id_otp_set_value, 1U},
    {UF4_ID_OVP_VALUE, &s_id_ovp_value, 0U},
    {UF4_ID_OVP_SET_VALUE, &s_id_ovp_set_value, 1U},
    {UF4_ID_OCP_VALUE, &s_id_ocp_value, 0U},
    {UF4_ID_OCP_SET_VALUE, &s_id_ocp_set_value, 1U},
    {UF4_ID_DUTY_CMD, &s_id_duty_cmd, 1U},
    {UF4_ID_PWM_A_COMPARE, &s_id_pwm_a_compare, 0U},
    {UF4_ID_PWM_D_COMPARE, &s_id_pwm_d_compare, 0U},
    {UF4_ID_FAN_SPEED, &s_id_fan_speed, 0U},
    {UF4_ID_FAN_SET_VALUE, &s_id_fan_set_value, 1U},
    {UF4_ID_LOOP_CURRENT_FEEDBACK, &s_id_loop_current_feedback, 0U},
    {UF4_ID_LOOP_CURRENT_REFERENCE, &s_id_loop_current_reference, 0U},
    {UF4_ID_VOLTAGE_LOOP_CURRENT_REFERENCE, &s_id_voltage_loop_current_reference, 0U},
};

/**
 * @brief 获取当前编译时固定启用的传输端口。
 * 返回由宏配置选中的 UF4 传输物理端口。
 */
static UF4Transport_Port s_UF4Transport_GetFixedPort(void)
{
    return (UF4Transport_Port)UF4_TRANSPORT_FIXED_PORT;
}

/**
 * @brief 判断指定端口是否被当前固件启用。
 * 只有与固定端口一致的通道会参与收发处理。
 */
static uint8_t s_UF4Transport_IsPortEnabled(UF4Transport_Port port)
{
    return (uint8_t)(port == s_UF4Transport_GetFixedPort());
}

static UF4Transport_ChannelContext *s_UF4Transport_GetChannel(UF4Transport_Port port)
{
    if ((s_UF4Transport_IsPortEnabled(port) == 0U) || ((uint32_t)port >= UF4_TRANSPORT_CHANNEL_COUNT))
    {
        return NULL;
    }

    return &s_uf4_transport.channels[(uint32_t)port];
}

static uint16_t s_UF4Transport_ClampU16(uint32_t value)
{
    return (value > 0xFFFFU) ? 0xFFFFU : (uint16_t)value;
}

static uint16_t s_UF4Transport_FloatToMilliU16(float value)
{
    if(value <= 0.0F)
    {
        return 0U;
    }

    return s_UF4Transport_ClampU16((uint32_t)(value * 1000.0F + 0.5F));
}

static uint16_t s_UF4Transport_FloatToCentiU16(float value)
{
    int32_t centi;

    if(value >= 0.0F)
    {
        centi = (int32_t)(value * 100.0F + 0.5F);
    }
    else
    {
        centi = (int32_t)(value * 100.0F - 0.5F);
    }

    if(centi < 0)
    {
        return 0U;
    }
    return s_UF4Transport_ClampU16((uint32_t)centi);
}

static uint16_t s_UF4Transport_GetStateFlagBits(void)
{
    switch(DF.SMFlag)
    {
        case Init:
            return 0x0001U;
        case Wait:
            return 0x0002U;
        case Rise:
            return 0x0004U;
        case Run:
            return 0x0008U;
        case Err:
        default:
            return 0x000FU;
    }
}

static uint16_t s_UF4Transport_GetStateMachineState(void)
{
    switch(DF.BBFlag)
    {
        case Buck:
            return 1U;
        case Boost:
            return 2U;
        case Mix:
            return 3U;
        case NA:
        default:
            return 0U;
    }
}

static uint16_t s_UF4Transport_GetCvccMode(void)
{
    return (CVCC_Mode == CC) ? 0U : 1U;
}

static uint16_t s_UF4Transport_GetDutyCmd(void)
{
    if(DF.BBFlag == Buck)
    {
        return (uint16_t)CtrValue.BuckDuty;
    }
    return (uint16_t)CtrValue.BoostDuty;
}

static uint16_t s_UF4Transport_GetVoltageLoopReferenceMv(void)
{
    if(CtrValue.Vout_ref <= 0)
    {
        return 0U;
    }
    return s_UF4Transport_FloatToMilliU16((float)CtrValue.Vout_ref * REF_3V3 * BSP_POWER_VOUT_SENSE_SCALE / ADC_MAX_VALUE);
}

static uint16_t s_UF4Transport_GetCurrentLoopReferenceMa(void)
{
    return s_UF4Transport_FloatToMilliU16(SET_Value.Iout);
}

static uint16_t s_UF4Transport_GetLoopFeedbackMa(void)
{
    if(IOUT > 0.0F)
    {
        return s_UF4Transport_FloatToMilliU16(IOUT);
    }
    if(IIN > 0.0F)
    {
        return s_UF4Transport_FloatToMilliU16(IIN);
    }
    return 0U;
}

static uint16_t s_UF4Transport_GetFanSpeedPermille(void)
{
    uint32_t compare_value = __HAL_TIM_GET_COMPARE(&htim8, TIM_CHANNEL_3);

    if(compare_value > 1000U)
    {
        compare_value = 1000U;
    }
    return (uint16_t)compare_value;
}

static uint16_t s_UF4Transport_GetFanSetPermille(void)
{
    return s_uf4_transport.fan_set_permille;
}

static void s_UF4Transport_UpdateControlReferenceFromSetting(void)
{
    CtrValue.Vout_SETref = (int32_t)((SET_Value.Vout / BSP_POWER_VOUT_SENSE_SCALE) / REF_3V3 * ADC_MAX_VALUE);
    CtrValue.Iout_ref = (int32_t)(((BSP_POWER_CURRENT_BIAS_V - SET_Value.Iout * BSP_POWER_CURRENT_SENSE_V_PER_A) / REF_3V3) * ADC_MAX_VALUE);
    if(CtrValue.Iout_ref > (int32_t)ADC_MAX_VALUE)
    {
        CtrValue.Iout_ref = (int32_t)ADC_MAX_VALUE;
    }
    if(CtrValue.Iout_ref < 0)
    {
        CtrValue.Iout_ref = 0;
    }
}

static void s_UF4Transport_UpdateReadRegisters(void)
{
    float hottest_temp = Board1_TEMP;

    if(Board2_TEMP > hottest_temp)
    {
        hottest_temp = Board2_TEMP;
    }

    s_id_input_voltage = s_UF4Transport_FloatToMilliU16(VIN);
    s_id_input_current = s_UF4Transport_FloatToMilliU16(IIN);
    s_id_output_voltage = s_UF4Transport_FloatToMilliU16(VOUT);
    s_id_output_current = s_UF4Transport_FloatToMilliU16(IOUT);
    s_id_core_temperature = s_UF4Transport_FloatToCentiU16(CPU_TEMP);
    s_id_temp1_temperature = s_UF4Transport_FloatToCentiU16(Board1_TEMP);
    s_id_temp2_temperature = s_UF4Transport_FloatToCentiU16(Board2_TEMP);
    s_id_set_voltage_limit = s_UF4Transport_FloatToMilliU16(SET_Value.Vout);
    s_id_set_current_limit = s_UF4Transport_FloatToMilliU16(SET_Value.Iout);
    s_id_cc_cv_mode = s_UF4Transport_GetCvccMode();
    s_id_power_state = (uint16_t)(DF.OUTPUT_Flag != 0U ? 1U : 0U);
    s_id_fault_state = DF.ErrFlag;
    s_id_state_machine_flag_bits = s_UF4Transport_GetStateFlagBits();
    s_id_state_machine_state = s_UF4Transport_GetStateMachineState();
    s_id_input_voltage_raw = s_UF4Transport_ClampU16(SADC.Vin);
    s_id_input_current_raw = s_UF4Transport_ClampU16(SADC.Iin);
    s_id_output_voltage_raw = s_UF4Transport_ClampU16(SADC.Vout);
    s_id_output_current_raw = s_UF4Transport_ClampU16(SADC.Iout);
    s_id_otp_value = s_UF4Transport_FloatToCentiU16(hottest_temp);
    s_id_otp_set_value = s_UF4Transport_FloatToCentiU16(MAX_OTP_VAL);
    s_id_ovp_value = s_UF4Transport_FloatToMilliU16(VOUT);
    s_id_ovp_set_value = s_UF4Transport_FloatToMilliU16(MAX_VOUT_OVP_VAL);
    s_id_ocp_value = s_UF4Transport_FloatToMilliU16(IOUT);
    s_id_ocp_set_value = s_UF4Transport_FloatToMilliU16(MAX_VOUT_OCP_VAL);
    s_id_duty_cmd = s_UF4Transport_GetDutyCmd();
    s_id_pwm_a_compare = s_UF4Transport_ClampU16(__HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1));
    s_id_pwm_d_compare = s_UF4Transport_ClampU16(__HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1));
    s_id_fan_speed = s_UF4Transport_GetFanSpeedPermille();
    s_id_fan_set_value = s_UF4Transport_GetFanSetPermille();
    s_id_loop_current_feedback = s_UF4Transport_GetLoopFeedbackMa();
    s_id_loop_current_reference = s_UF4Transport_GetCurrentLoopReferenceMa();
    s_id_voltage_loop_current_reference = s_UF4Transport_GetVoltageLoopReferenceMv();
}

/**
 * @brief 将可写 UF4 寄存器值应用到控制状态。
 * 把上位机写入的设定值、保护值、输出开关和风扇目标同步到固件变量。
 */
static void s_UF4Transport_ApplyWriteRegisters(void)
{
    float set_voltage = (float)s_id_set_voltage_limit / 1000.0F;
    float set_current = (float)s_id_set_current_limit / 1000.0F;
    float otp = (float)s_id_otp_set_value / 100.0F;
    float ovp = (float)s_id_ovp_set_value / 1000.0F;
    float ocp = (float)s_id_ocp_set_value / 1000.0F;
    uint16_t fan_set = s_id_fan_set_value;

    if(fan_set > (POWER_CTRL_FAN_MAX_RUN_DUTY * 10U))
    {
        fan_set = POWER_CTRL_FAN_MAX_RUN_DUTY * 10U;
    }

    SET_Value.Vout = set_voltage;
    SET_Value.Iout = set_current;
    /* 保护阈值只在合法范围内接受，避免上位机写 0 导致 Vout>=0 永远成立、
       上电即误触发保护。OTP 10~150℃、OVP 1~100V、OCP 0.1~50A。 */
    if(otp >= 10.0F && otp <= 150.0F)
    {
        MAX_OTP_VAL = otp;
    }
    if(ovp >= 1.0F && ovp <= 100.0F)
    {
        MAX_VOUT_OVP_VAL = ovp;
    }
    if(ocp >= 0.1F && ocp <= 50.0F)
    {
        MAX_VOUT_OCP_VAL = ocp;
    }
    SET_Value.SET_modified_flag = 1.0F;
    s_UF4Transport_UpdateControlReferenceFromSetting();

    DF.OUTPUT_Flag = (uint8_t)(s_id_power_state != 0U ? 1U : 0U);
    if(DF.OUTPUT_Flag == 0U)
    {
        PowerControl_DisableOutput();
        if(DF.SMFlag != Err)
        {
            DF.SMFlag = Wait;
        }
    }

    s_uf4_transport.fan_manual_enable = 1U;
    s_uf4_transport.fan_set_permille = fan_set;
    FAN_PWM_set((uint16_t)(fan_set / 10U));
    s_uf4_transport.last_fan_apply_tick = HAL_GetTick();
}

static HAL_StatusTypeDef s_UF4Transport_SendOnChannel(UF4Transport_ChannelContext *channel, const uint8_t *data, uint16_t len)
{
    if((channel == NULL) || (data == NULL) || (len == 0U) || (len > UF4_TRANSPORT_MAX_FRAME_SIZE))
    {
        return HAL_ERROR;
    }

    if(channel->tx_busy != 0U)
    {
        if((channel->port == UF4_TRANSPORT_PORT_USART1) && (huart1.gState == HAL_UART_STATE_READY))
        {
            channel->tx_busy = 0U;
        }
        else if((channel->port == UF4_TRANSPORT_PORT_USART2) && (huart2.gState == HAL_UART_STATE_READY))
        {
            channel->tx_busy = 0U;
        }
    }

    if(channel->tx_busy != 0U)
    {
        return HAL_BUSY;
    }

    memcpy(channel->tx_buffer, data, len);

    switch(channel->port)
    {
        case UF4_TRANSPORT_PORT_USART1:
            if(HAL_UART_Transmit_DMA(&huart1, channel->tx_buffer, len) != HAL_OK)
            {
                return HAL_BUSY;
            }
            channel->tx_busy = 1U;
            return HAL_OK;

        case UF4_TRANSPORT_PORT_USART2:
            if(HAL_UART_Transmit_DMA(&huart2, channel->tx_buffer, len) != HAL_OK)
            {
                return HAL_BUSY;
            }
            channel->tx_busy = 1U;
            return HAL_OK;

        case UF4_TRANSPORT_PORT_CDC:
        default:
            if(CDC_Transmit_FS(channel->tx_buffer, len) != USBD_OK)
            {
                return HAL_BUSY;
            }
            channel->tx_busy = 1U;
            return HAL_OK;
    }
}

static HAL_StatusTypeDef s_UF4Transport_QueueBytes(UF4Transport_ChannelContext *channel, const uint8_t *data, uint16_t len)
{
    if((channel == NULL) || (data == NULL) || (len == 0U) || (len > UF4_TRANSPORT_MAX_FRAME_SIZE))
    {
        return HAL_ERROR;
    }

    if(s_UF4Transport_SendOnChannel(channel, data, len) == HAL_OK)
    {
        return HAL_OK;
    }

    memcpy(channel->pending_buffer, data, len);
    channel->pending_len = len;
    return HAL_OK;
}

static void s_UF4Transport_FlushPending(UF4Transport_ChannelContext *channel)
{
    if((channel == NULL) || (channel->pending_len == 0U))
    {
        return;
    }

    if(s_UF4Transport_SendOnChannel(channel, channel->pending_buffer, channel->pending_len) == HAL_OK)
    {
        channel->pending_len = 0U;
    }
}

static uint8_t s_UF4Transport_CdcTxIdle(void)
{
    extern USBD_HandleTypeDef hUsbDeviceFS;
    USBD_CDC_HandleTypeDef *hcdc;

    if(hUsbDeviceFS.pClassData == NULL)
    {
        return 1U;
    }

    hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    return (uint8_t)(hcdc->TxState == 0U);
}

static void s_UF4Transport_PollTxCompletion(UF4Transport_ChannelContext *channel)
{
    if((channel == NULL) || (channel->tx_busy == 0U))
    {
        return;
    }

    if((channel->port == UF4_TRANSPORT_PORT_USART1) && (huart1.gState == HAL_UART_STATE_READY))
    {
        channel->tx_busy = 0U;
    }
    else if((channel->port == UF4_TRANSPORT_PORT_USART2) && (huart2.gState == HAL_UART_STATE_READY))
    {
        channel->tx_busy = 0U;
    }
    else if((channel->port == UF4_TRANSPORT_PORT_CDC) && (s_UF4Transport_CdcTxIdle() != 0U))
    {
        /* CDC 的 TX 完成回调不一定总能抢在主循环再次尝试发送之前到达，
           这里直接查询 USB CDC 的真实 TxState 作为兜底，避免 tx_busy 永久卡死。 */
        channel->tx_busy = 0U;
    }
}

static void s_UF4Transport_Uf4Tx(const uint8_t *data, uint16_t len, void *user)
{
    (void)user;
    (void)UF4Transport_SendBytes(data, len);
}

static void s_UF4Transport_Uf4WriteApply(void *user)
{
    (void)user;
    s_UF4Transport_ApplyWriteRegisters();
    s_UF4Transport_UpdateReadRegisters();
}

static void s_UF4Transport_MirrorUartRxToCdc(UF4Transport_Port port, const uint8_t *data, uint16_t len)
{
#if UF4_TRANSPORT_UART1_CDC_MIRROR
    if((port == UF4_TRANSPORT_PORT_USART1) && (data != NULL) && (len > 0U) && (s_UF4Transport_CdcTxIdle() != 0U))
    {
        (void)CDC_Transmit_FS((uint8_t *)data, len);
    }
#else
    (void)port;
    (void)data;
    (void)len;
#endif
}

static void s_UF4Transport_ProcessUartRxWindow(UF4Transport_Port port, uint8_t *buffer, uint16_t *last_pos, uint16_t current_pos)
{
    if(current_pos > UF4_TRANSPORT_UART_RX_DMA_SIZE)
    {
        current_pos = UF4_TRANSPORT_UART_RX_DMA_SIZE;
    }

    if(current_pos > *last_pos)
    {
        s_UF4Transport_MirrorUartRxToCdc(port, &buffer[*last_pos], (uint16_t)(current_pos - *last_pos));
        (void)UF4_InputBuffer(&buffer[*last_pos], (uint16_t)(current_pos - *last_pos));
    }
    else if(current_pos < *last_pos)
    {
        s_UF4Transport_MirrorUartRxToCdc(port, &buffer[*last_pos], (uint16_t)(UF4_TRANSPORT_UART_RX_DMA_SIZE - *last_pos));
        (void)UF4_InputBuffer(&buffer[*last_pos], (uint16_t)(UF4_TRANSPORT_UART_RX_DMA_SIZE - *last_pos));
        if(current_pos > 0U)
        {
            s_UF4Transport_MirrorUartRxToCdc(port, buffer, current_pos);
            (void)UF4_InputBuffer(buffer, current_pos);
        }
    }
    else if(current_pos == UF4_TRANSPORT_UART_RX_DMA_SIZE)
    {
        s_UF4Transport_MirrorUartRxToCdc(port, &buffer[*last_pos], (uint16_t)(UF4_TRANSPORT_UART_RX_DMA_SIZE - *last_pos));
        (void)UF4_InputBuffer(&buffer[*last_pos], (uint16_t)(UF4_TRANSPORT_UART_RX_DMA_SIZE - *last_pos));
    }

    (void)port;
    *last_pos = (current_pos >= UF4_TRANSPORT_UART_RX_DMA_SIZE) ? 0U : current_pos;
}

static void s_UF4Transport_PollUartRx(void)
{
    uint16_t current_pos;

    if((s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_USART1) != 0U) && (huart1.hdmarx != NULL))
    {
        current_pos = (uint16_t)(UF4_TRANSPORT_UART_RX_DMA_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));
        s_UF4Transport_ProcessUartRxWindow(UF4_TRANSPORT_PORT_USART1, s_uf4_transport.uart1_rx_buffer, &s_uf4_transport.uart1_rx_last_pos, current_pos);
    }

    if((s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_USART2) != 0U) && (huart2.hdmarx != NULL))
    {
        current_pos = (uint16_t)(UF4_TRANSPORT_UART_RX_DMA_SIZE - __HAL_DMA_GET_COUNTER(huart2.hdmarx));
        s_UF4Transport_ProcessUartRxWindow(UF4_TRANSPORT_PORT_USART2, s_uf4_transport.uart2_rx_buffer, &s_uf4_transport.uart2_rx_last_pos, current_pos);
    }
}

static void s_UF4Transport_StartUartRxDma(UART_HandleTypeDef *huart, uint8_t *buffer)
{
    if(HAL_UART_Receive_DMA(huart, buffer, UF4_TRANSPORT_UART_RX_DMA_SIZE) == HAL_OK)
    {
        if(huart->hdmarx != NULL)
        {
            __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
        }
    }
}

/**
 * @brief 初始化 UF4 传输层与寄存器绑定。
 * 完成端口上下文、ID 表绑定、UF4 协议栈初始化和串口 DMA 接收启动。
 */
void UF4Transport_Init(void)
{
    uint32_t i;

    memset(&s_uf4_transport, 0, sizeof(s_uf4_transport));
    s_uf4_transport.selected_port = s_UF4Transport_GetFixedPort();
    s_uf4_transport.fan_manual_enable = 1U;
    s_uf4_transport.fan_set_permille = POWER_CTRL_FAN_MAX_RUN_DUTY * 10U;
    s_uf4_transport.last_fan_apply_tick = HAL_GetTick();

    for(i = 0U; i < UF4_TRANSPORT_CHANNEL_COUNT; ++i)
    {
        s_uf4_transport.channels[i].port = (UF4Transport_Port)i;
    }

    FAN_PWM_set(POWER_CTRL_FAN_MAX_RUN_DUTY);
    s_UF4Transport_UpdateReadRegisters();
    UF4_IDTableBind(s_uf4_id_table, sizeof(s_uf4_id_table) / sizeof(s_uf4_id_table[0]));
    UF4_Init(s_UF4Transport_Uf4Tx, NULL);
    UF4_SetWriteApplyCallback(s_UF4Transport_Uf4WriteApply, NULL);

    if(s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_USART1) != 0U)
    {
        s_UF4Transport_StartUartRxDma(&huart1, s_uf4_transport.uart1_rx_buffer);
    }
    if(s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_USART2) != 0U)
    {
        s_UF4Transport_StartUartRxDma(&huart2, s_uf4_transport.uart2_rx_buffer);
    }
}

/**
 * @brief 执行 UF4 传输后台任务。
 * 刷新读寄存器镜像、轮询串口接收窗口并处理挂起发送。
 */
void UF4Transport_RunTask(void)
{
    uint32_t i;
    const uint32_t tick_now = HAL_GetTick();

    s_UF4Transport_UpdateReadRegisters();
    s_UF4Transport_PollUartRx();

    if((s_uf4_transport.fan_manual_enable != 0U) && ((tick_now - s_uf4_transport.last_fan_apply_tick) >= 50U))
    {
        FAN_PWM_set((uint16_t)(s_uf4_transport.fan_set_permille / 10U));
        s_uf4_transport.last_fan_apply_tick = tick_now;
    }

    /* 流发送（poll TX / flush pending / UF4_Process）已移到 UF4Transport_StreamTick，
       由 TIM7 中断每 20ms 调用，避免输出开启后主循环被 HRTIM PID 中断饿死。 */
    for(i = 0U; i < UF4_TRANSPORT_CHANNEL_COUNT; ++i)
    {
        if(s_UF4Transport_IsPortEnabled(s_uf4_transport.channels[i].port) != 0U)
        {
            s_UF4Transport_PollTxCompletion(&s_uf4_transport.channels[i]);
            s_UF4Transport_FlushPending(&s_uf4_transport.channels[i]);
        }
    }
}

/**
 * @brief 执行 UF4 周期流发送节拍。
 * 刷新流数据源并在各通道发送准备就绪时推进协议栈输出。
 */
void UF4Transport_StreamTick(void)
{
    uint32_t i;

    /* 刷新读寄存器（流数据来源） */
    s_UF4Transport_UpdateReadRegisters();

    /* poll TX 完成 + flush pending，确保上一帧发完才发下一帧 */
    for(i = 0U; i < UF4_TRANSPORT_CHANNEL_COUNT; ++i)
    {
        if(s_UF4Transport_IsPortEnabled(s_uf4_transport.channels[i].port) != 0U)
        {
            s_UF4Transport_PollTxCompletion(&s_uf4_transport.channels[i]);
            s_UF4Transport_FlushPending(&s_uf4_transport.channels[i]);
        }
    }

    UF4_Process();
}

/**
 * @brief 选择当前发送使用的端口。
 * 仅当目标端口已启用时才更新当前发送通道。
 */
void UF4Transport_SelectPort(UF4Transport_Port port)
{
    if(s_UF4Transport_IsPortEnabled(port) != 0U)
    {
        s_uf4_transport.selected_port = port;
    }
}

/**
 * @brief 获取当前启用的 UF4 传输端口。
 * 返回编译期固定选择的物理通信端口。
 */
UF4Transport_Port UF4Transport_GetPort(void)
{
    return s_UF4Transport_GetFixedPort();
}

/**
 * @brief 发送一帧 UF4 数据到当前端口。
 * 数据会优先立即发送，忙时进入挂起缓冲等待后续冲刷。
 */
HAL_StatusTypeDef UF4Transport_SendBytes(const uint8_t *data, uint16_t len)
{
    return s_UF4Transport_QueueBytes(s_UF4Transport_GetChannel(s_uf4_transport.selected_port), data, len);
}

/**
 * @brief 处理 USB CDC 收到的 UF4 数据。
 * 将 CDC 端口选为当前通道，并把输入字节交给 UF4 协议栈解析。
 */
void UF4Transport_OnUsbCdcRx(const uint8_t *data, uint16_t len)
{
    if((data == NULL) || (len == 0U) || (s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_CDC) == 0U))
    {
        return;
    }

    s_uf4_transport.selected_port = UF4_TRANSPORT_PORT_CDC;
    s_UF4Transport_UpdateReadRegisters();
    (void)UF4_InputBuffer(data, len);
    s_UF4Transport_UpdateReadRegisters();
}

/**
 * @brief 提供给外部用户层的 USB CDC 接收桥接入口。
 * 兼容保留一层包装，实际处理直接委托给 UF4Transport_OnUsbCdcRx。
 */
void USER_uf4TransportOnUsbCdcRx(uint8_t *data, uint16_t len)
{
    UF4Transport_OnUsbCdcRx(data, len);
}

/**
 * @brief 标记指定端口发送完成。
 * 在 DMA 或 CDC 发送结束后清除通道忙标志，允许继续发下一帧。
 */
void UF4Transport_OnTxComplete(UF4Transport_Port port)
{
    UF4Transport_ChannelContext *channel = s_UF4Transport_GetChannel(port);

    if(channel != NULL)
    {
        channel->tx_busy = 0U;
    }
}

/**
 * @brief 串口接收事件回调占位函数。
 * 当前串口接收由 DMA 环形缓冲轮询处理，这里仅保留 HAL 回调入口。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    (void)huart;
    (void)Size;
}

/**
 * @brief 串口错误回调。
 * 发生错误时释放发送忙状态并重启对应串口 DMA 接收。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
        UF4Transport_OnTxComplete(UF4_TRANSPORT_PORT_USART1);
        s_uf4_transport.uart1_rx_last_pos = 0U;
        if(s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_USART1) != 0U)
        {
            s_UF4Transport_StartUartRxDma(&huart1, s_uf4_transport.uart1_rx_buffer);
        }
    }
    else if(huart == &huart2)
    {
        UF4Transport_OnTxComplete(UF4_TRANSPORT_PORT_USART2);
        s_uf4_transport.uart2_rx_last_pos = 0U;
        if(s_UF4Transport_IsPortEnabled(UF4_TRANSPORT_PORT_USART2) != 0U)
        {
            s_UF4Transport_StartUartRxDma(&huart2, s_uf4_transport.uart2_rx_buffer);
        }
    }
}

/**
 * @brief 串口 DMA 发送完成回调。
 * 根据完成的 UART 实例通知传输层释放对应通道。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
        UF4Transport_OnTxComplete(UF4_TRANSPORT_PORT_USART1);
    }
    else if(huart == &huart2)
    {
        UF4Transport_OnTxComplete(UF4_TRANSPORT_PORT_USART2);
    }
}
