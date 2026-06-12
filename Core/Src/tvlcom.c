/**
  ******************************************************************************
  * @file    tvlcom.c
  * @author  UF4
  * @date    26-6-12 下午4:29
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
#include "tvlcom.h"
#include <string.h>

#include "function.h"
#include "hrtim.h"
#include "tim.h"
#include "usart.h"
#include "usbd_cdc_if.h"

extern volatile int32_t IErr0, IErr1;
extern volatile int32_t i0, i1;
extern volatile int32_t VErr0, VErr1, VErr2;
extern volatile int32_t u0, u1;

#define TVLCOM_CMD_ACK                  0x00U
#define TVLCOM_CMD_READ                 0x01U
#define TVLCOM_CMD_WRITE                0x02U
#define TVLCOM_CMD_REPORT               0x03U
#define TVLCOM_CMD_STREAM_START         0x04U
#define TVLCOM_CMD_STREAM_STOP          0x05U
#define TVLCOM_CMD_NACK                 0xFFU

#define TVLCOM_ACCESS_READ              0x01U
#define TVLCOM_ACCESS_WRITE             0x02U
#define TVLCOM_ACCESS_READ_WRITE        (TVLCOM_ACCESS_READ | TVLCOM_ACCESS_WRITE)

#define TVLCOM_DATA_INPUT_VOLTAGE               10U
#define TVLCOM_DATA_INPUT_CURRENT               11U
#define TVLCOM_DATA_OUTPUT_VOLTAGE              12U
#define TVLCOM_DATA_OUTPUT_CURRENT              13U
#define TVLCOM_DATA_CORE_TEMPERATURE            14U
#define TVLCOM_DATA_BOARD_TEMPERATURE           15U
#define TVLCOM_DATA_TEMP2_TEMPERATURE           16U
#define TVLCOM_DATA_SET_VOLTAGE_LIMIT           17U
#define TVLCOM_DATA_SET_CURRENT_LIMIT           18U
#define TVLCOM_DATA_CC_CV_MODE                  20U
#define TVLCOM_DATA_POWER_STATE                 21U
#define TVLCOM_DATA_FAULT_STATE                 22U
#define TVLCOM_DATA_STATE_MACHINE_FLAG_BITS     23U
#define TVLCOM_DATA_STATE_MACHINE_STATE         24U
#define TVLCOM_DATA_INPUT_VOLTAGE_RAW           25U
#define TVLCOM_DATA_INPUT_CURRENT_RAW           26U
#define TVLCOM_DATA_OUTPUT_VOLTAGE_RAW          27U
#define TVLCOM_DATA_OUTPUT_CURRENT_RAW          28U
#define TVLCOM_DATA_OTP_VALUE                   29U
#define TVLCOM_DATA_OTP_SET_VALUE               30U
#define TVLCOM_DATA_OVP_VALUE                   31U
#define TVLCOM_DATA_OVP_SET_VALUE               32U
#define TVLCOM_DATA_OCP_VALUE                   33U
#define TVLCOM_DATA_OCP_SET_VALUE               34U
#define TVLCOM_DATA_DUTY_CMD                    35U
#define TVLCOM_DATA_PWM_A_COMPARE               36U
#define TVLCOM_DATA_PWM_D_COMPARE               37U
#define TVLCOM_DATA_FAN_SPEED                   38U
#define TVLCOM_DATA_FAN_SET_VALUE               39U
#define TVLCOM_DATA_DEBUG_SNAPSHOT              40U
#define TVLCOM_DATA_LOOP_CURRENT_FEEDBACK       41U
#define TVLCOM_DATA_LOOP_CURRENT_REFERENCE      42U
#define TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE 43U

#define TVLCOM_STATE_FLAG_INIT          0x01U
#define TVLCOM_STATE_FLAG_WAIT          0x02U
#define TVLCOM_STATE_FLAG_RISE          0x04U
#define TVLCOM_STATE_FLAG_RUN           0x08U
#define TVLCOM_STATE_FLAG_ERR           0x0FU

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t access;
    uint8_t signedValue;
} TVLCOM_DataMeta;

typedef struct
{
    uint8_t fastTypes[8];
    uint8_t slowTypes[8];
    uint8_t fastCount;
    uint8_t slowCount;
    uint16_t fastPeriodMs;
    uint16_t slowPeriodMs;
    uint32_t lastFastTick;
    uint32_t lastSlowTick;
    uint8_t enabled;
} TVLCOM_StreamState;

typedef struct
{
    TVLCOM_Port currentPort;
    uint8_t rxFrameBuffer[TVLCOM_MAX_FRAME_SIZE];
    uint16_t rxFrameLength;
    uint8_t txBuffer[TVLCOM_MAX_FRAME_SIZE];
    uint8_t pendingBuffer[TVLCOM_MAX_FRAME_SIZE];
    uint16_t pendingLength;
    uint8_t pendingIsStream;

    uint8_t uart1RxBuffer[TVLCOM_UART_RX_DMA_SIZE];
    uint8_t uart2RxBuffer[TVLCOM_UART_RX_DMA_SIZE];
    uint16_t uart1RxLastPos;
    uint16_t uart2RxLastPos;

    uint16_t fanSetPermille;
    uint8_t fanManualEnable;
    uint32_t lastFanApplyTick;

    TVLCOM_StreamState stream;
} TVLCOM_Context;

static TVLCOM_Context s_TVLCOM_Context;

static const TVLCOM_DataMeta s_TVLCOM_DataMetaTable[] =
{
    {TVLCOM_DATA_INPUT_VOLTAGE,                   4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_INPUT_CURRENT,                   4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OUTPUT_VOLTAGE,                  4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OUTPUT_CURRENT,                  4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_CORE_TEMPERATURE,                4U, TVLCOM_ACCESS_READ,       1U},
    {TVLCOM_DATA_BOARD_TEMPERATURE,               4U, TVLCOM_ACCESS_READ,       1U},
    {TVLCOM_DATA_TEMP2_TEMPERATURE,               4U, TVLCOM_ACCESS_READ,       1U},
    {TVLCOM_DATA_SET_VOLTAGE_LIMIT,               4U, TVLCOM_ACCESS_READ_WRITE, 0U},
    {TVLCOM_DATA_SET_CURRENT_LIMIT,               4U, TVLCOM_ACCESS_READ_WRITE, 0U},
    {TVLCOM_DATA_CC_CV_MODE,                      1U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_POWER_STATE,                     1U, TVLCOM_ACCESS_READ_WRITE, 0U},
    {TVLCOM_DATA_FAULT_STATE,                     4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_STATE_MACHINE_FLAG_BITS,         1U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_STATE_MACHINE_STATE,             1U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_INPUT_VOLTAGE_RAW,               4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_INPUT_CURRENT_RAW,               4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OUTPUT_VOLTAGE_RAW,              4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OUTPUT_CURRENT_RAW,              4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OTP_VALUE,                       4U, TVLCOM_ACCESS_READ,       1U},
    {TVLCOM_DATA_OTP_SET_VALUE,                   4U, TVLCOM_ACCESS_READ_WRITE, 1U},
    {TVLCOM_DATA_OVP_VALUE,                       4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OVP_SET_VALUE,                   4U, TVLCOM_ACCESS_READ_WRITE, 0U},
    {TVLCOM_DATA_OCP_VALUE,                       4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_OCP_SET_VALUE,                   4U, TVLCOM_ACCESS_READ_WRITE, 0U},
    {TVLCOM_DATA_DUTY_CMD,                        4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_PWM_A_COMPARE,                   4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_PWM_D_COMPARE,                   4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_FAN_SPEED,                       4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_FAN_SET_VALUE,                   4U, TVLCOM_ACCESS_READ_WRITE, 0U},
    {TVLCOM_DATA_DEBUG_SNAPSHOT,                  0U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_LOOP_CURRENT_FEEDBACK,           4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_LOOP_CURRENT_REFERENCE,          4U, TVLCOM_ACCESS_READ,       0U},
    {TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE,  4U, TVLCOM_ACCESS_READ,       0U}
};

static const uint8_t s_TVLCOM_ReportTypes[] =
{
    TVLCOM_DATA_INPUT_VOLTAGE,
    TVLCOM_DATA_INPUT_CURRENT,
    TVLCOM_DATA_OUTPUT_VOLTAGE,
    TVLCOM_DATA_OUTPUT_CURRENT,
    TVLCOM_DATA_CORE_TEMPERATURE,
    TVLCOM_DATA_BOARD_TEMPERATURE,
    TVLCOM_DATA_TEMP2_TEMPERATURE,
    TVLCOM_DATA_SET_VOLTAGE_LIMIT,
    TVLCOM_DATA_SET_CURRENT_LIMIT,
    TVLCOM_DATA_CC_CV_MODE,
    TVLCOM_DATA_POWER_STATE,
    TVLCOM_DATA_FAULT_STATE,
    TVLCOM_DATA_STATE_MACHINE_FLAG_BITS,
    TVLCOM_DATA_STATE_MACHINE_STATE,
    TVLCOM_DATA_INPUT_CURRENT_RAW,
    TVLCOM_DATA_OUTPUT_CURRENT_RAW,
    TVLCOM_DATA_OTP_VALUE,
    TVLCOM_DATA_OTP_SET_VALUE,
    TVLCOM_DATA_OVP_VALUE,
    TVLCOM_DATA_OVP_SET_VALUE,
    TVLCOM_DATA_OCP_VALUE,
    TVLCOM_DATA_OCP_SET_VALUE,
    TVLCOM_DATA_DUTY_CMD,
    TVLCOM_DATA_PWM_A_COMPARE,
    TVLCOM_DATA_PWM_D_COMPARE,
    TVLCOM_DATA_FAN_SPEED,
    TVLCOM_DATA_FAN_SET_VALUE,
    TVLCOM_DATA_LOOP_CURRENT_FEEDBACK,
    TVLCOM_DATA_LOOP_CURRENT_REFERENCE,
    TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE
};

static const uint8_t s_TVLCOM_DebugSnapshotTypes[] =
{
    TVLCOM_DATA_OUTPUT_VOLTAGE_RAW,
    TVLCOM_DATA_OUTPUT_VOLTAGE,
    TVLCOM_DATA_INPUT_CURRENT_RAW,
    TVLCOM_DATA_OUTPUT_CURRENT_RAW,
    TVLCOM_DATA_INPUT_CURRENT,
    TVLCOM_DATA_OUTPUT_CURRENT,
    TVLCOM_DATA_LOOP_CURRENT_FEEDBACK,
    TVLCOM_DATA_LOOP_CURRENT_REFERENCE,
    TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE
};

static const TVLCOM_DataMeta *s_TVLCOM_GetMeta(uint8_t type)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)(sizeof(s_TVLCOM_DataMetaTable) / sizeof(s_TVLCOM_DataMetaTable[0])); ++i)
    {
        if (s_TVLCOM_DataMetaTable[i].type == type)
        {
            return &s_TVLCOM_DataMetaTable[i];
        }
    }
    return NULL;
}

static void s_TVLCOM_WriteLe16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void s_TVLCOM_WriteLe32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFU);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint16_t s_TVLCOM_ReadLe16(const uint8_t *buffer)
{
    return (uint16_t)((uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8));
}

static uint32_t s_TVLCOM_ReadLe32(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] |
           ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
}

static uint16_t s_TVLCOM_Crc16Modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;

    for (i = 0U; i < len; ++i)
    {
        uint8_t bitIndex;

        crc ^= data[i];
        for (bitIndex = 0U; bitIndex < 8U; ++bitIndex)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1) ^ 0xA001U);
            }
            else
            {
                crc = (uint16_t)(crc >> 1);
            }
        }
    }

    return crc;
}

static uint16_t s_TVLCOM_BuildFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t payloadLen, uint8_t *frame)
{
    uint16_t bodyLen = (uint16_t)(payloadLen + 2U);
    uint16_t totalLen = (uint16_t)(bodyLen + 6U);
    uint16_t crc;

    frame[0] = TVLCOM_SOF0;
    frame[1] = TVLCOM_SOF1;
    s_TVLCOM_WriteLe16(&frame[2], bodyLen);
    frame[4] = cmd;
    frame[5] = seq;
    if ((payload != NULL) && (payloadLen > 0U))
    {
        memcpy(&frame[6], payload, payloadLen);
    }

    crc = s_TVLCOM_Crc16Modbus(frame, (uint16_t)(totalLen - 2U));
    s_TVLCOM_WriteLe16(&frame[totalLen - 2U], crc);
    return totalLen;
}

static uint32_t s_TVLCOM_FloatToMilliUnsigned(float value)
{
    if (value <= 0.0F)
    {
        return 0U;
    }
    return (uint32_t)(value * 1000.0F + 0.5F);
}

static int32_t s_TVLCOM_FloatToMilliSigned(float value)
{
    if (value >= 0.0F)
    {
        return (int32_t)(value * 1000.0F + 0.5F);
    }
    return (int32_t)(value * 1000.0F - 0.5F);
}

static uint16_t s_TVLCOM_ClampU16(uint32_t value)
{
    if (value > 0xFFFFU)
    {
        return 0xFFFFU;
    }
    return (uint16_t)value;
}

static uint8_t s_TVLCOM_GetStateFlagBits(void)
{
    switch (DF.SMFlag)
    {
        case Init:
            return TVLCOM_STATE_FLAG_INIT;
        case Wait:
            return TVLCOM_STATE_FLAG_WAIT;
        case Rise:
            return TVLCOM_STATE_FLAG_RISE;
        case Run:
            return TVLCOM_STATE_FLAG_RUN;
        case Err:
        default:
            return TVLCOM_STATE_FLAG_ERR;
    }
}

static uint8_t s_TVLCOM_GetStageMode(void)
{
    switch (DF.BBFlag)
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

static uint8_t s_TVLCOM_GetCvccMode(void)
{
    if (CVCC_Mode == CC)
    {
        return 0U;
    }
    return 1U;
}

static uint32_t s_TVLCOM_GetVoltageLoopReferenceMv(void)
{
    if (CtrValue.Vout_ref <= 0)
    {
        return 0U;
    }
    return s_TVLCOM_FloatToMilliUnsigned((float)CtrValue.Vout_ref * REF_3V3 * BSP_POWER_VOUT_SENSE_SCALE / ADC_MAX_VALUE);
}

static uint32_t s_TVLCOM_GetCurrentLoopReferenceMa(void)
{
    if (SET_Value.Iout <= 0.0F)
    {
        return 0U;
    }
    return s_TVLCOM_FloatToMilliUnsigned(SET_Value.Iout);
}

static uint32_t s_TVLCOM_GetLoopFeedbackMa(void)
{
    if (IOUT > 0.0F)
    {
        return s_TVLCOM_FloatToMilliUnsigned(IOUT);
    }
    if (IIN > 0.0F)
    {
        return s_TVLCOM_FloatToMilliUnsigned(IIN);
    }
    return 0U;
}

static uint32_t s_TVLCOM_GetDutyCmd(void)
{
    if (DF.BBFlag == Buck)
    {
        return (uint32_t)CtrValue.BuckDuty;
    }
    return (uint32_t)CtrValue.BoostDuty;
}

static uint32_t s_TVLCOM_GetPwmACompare(void)
{
    return __HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1);
}

static uint32_t s_TVLCOM_GetPwmDCompare(void)
{
    return __HAL_HRTIM_GETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_D, HRTIM_COMPAREUNIT_1);
}

static uint32_t s_TVLCOM_GetFanSpeedPermille(void)
{
    uint32_t compareValue = __HAL_TIM_GET_COMPARE(&htim8, TIM_CHANNEL_3);
    if (compareValue > 1000U)
    {
        compareValue = 1000U;
    }
    return compareValue;
}

static uint32_t s_TVLCOM_GetFanSetPermille(void)
{
    if (s_TVLCOM_Context.fanManualEnable != 0U)
    {
        return s_TVLCOM_Context.fanSetPermille;
    }
    return s_TVLCOM_GetFanSpeedPermille();
}

static int32_t s_TVLCOM_GetOtpValueMc(void)
{
    float hottestTemp = Board1_TEMP;

    if (Board2_TEMP > hottestTemp)
    {
        hottestTemp = Board2_TEMP;
    }
    return s_TVLCOM_FloatToMilliSigned(hottestTemp);
}

static uint32_t s_TVLCOM_GetU32ValueByType(uint8_t type)
{
    switch (type)
    {
        case TVLCOM_DATA_INPUT_VOLTAGE:
            return s_TVLCOM_FloatToMilliUnsigned(VIN);
        case TVLCOM_DATA_INPUT_CURRENT:
            return s_TVLCOM_FloatToMilliUnsigned(IIN);
        case TVLCOM_DATA_OUTPUT_VOLTAGE:
            return s_TVLCOM_FloatToMilliUnsigned(VOUT);
        case TVLCOM_DATA_OUTPUT_CURRENT:
            return s_TVLCOM_FloatToMilliUnsigned(IOUT);
        case TVLCOM_DATA_SET_VOLTAGE_LIMIT:
            return s_TVLCOM_FloatToMilliUnsigned(SET_Value.Vout);
        case TVLCOM_DATA_SET_CURRENT_LIMIT:
            return s_TVLCOM_FloatToMilliUnsigned(SET_Value.Iout);
        case TVLCOM_DATA_FAULT_STATE:
            return (uint32_t)DF.ErrFlag;
        case TVLCOM_DATA_INPUT_VOLTAGE_RAW:
            return (uint32_t)SADC.Vin;
        case TVLCOM_DATA_INPUT_CURRENT_RAW:
            return (uint32_t)SADC.Iin;
        case TVLCOM_DATA_OUTPUT_VOLTAGE_RAW:
            return (uint32_t)SADC.Vout;
        case TVLCOM_DATA_OUTPUT_CURRENT_RAW:
            return (uint32_t)SADC.Iout;
        case TVLCOM_DATA_OVP_VALUE:
            return s_TVLCOM_FloatToMilliUnsigned(VOUT);
        case TVLCOM_DATA_OVP_SET_VALUE:
            return s_TVLCOM_FloatToMilliUnsigned(MAX_VOUT_OVP_VAL);
        case TVLCOM_DATA_OCP_VALUE:
            return s_TVLCOM_FloatToMilliUnsigned(IOUT);
        case TVLCOM_DATA_OCP_SET_VALUE:
            return s_TVLCOM_FloatToMilliUnsigned(MAX_VOUT_OCP_VAL);
        case TVLCOM_DATA_DUTY_CMD:
            return s_TVLCOM_GetDutyCmd();
        case TVLCOM_DATA_PWM_A_COMPARE:
            return s_TVLCOM_GetPwmACompare();
        case TVLCOM_DATA_PWM_D_COMPARE:
            return s_TVLCOM_GetPwmDCompare();
        case TVLCOM_DATA_FAN_SPEED:
            return s_TVLCOM_GetFanSpeedPermille();
        case TVLCOM_DATA_FAN_SET_VALUE:
            return s_TVLCOM_GetFanSetPermille();
        case TVLCOM_DATA_LOOP_CURRENT_FEEDBACK:
            return s_TVLCOM_GetLoopFeedbackMa();
        case TVLCOM_DATA_LOOP_CURRENT_REFERENCE:
            return s_TVLCOM_GetCurrentLoopReferenceMa();
        case TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE:
            return s_TVLCOM_GetVoltageLoopReferenceMv();
        default:
            return 0U;
    }
}

static int32_t s_TVLCOM_GetS32ValueByType(uint8_t type)
{
    switch (type)
    {
        case TVLCOM_DATA_CORE_TEMPERATURE:
            return s_TVLCOM_FloatToMilliSigned(CPU_TEMP);
        case TVLCOM_DATA_BOARD_TEMPERATURE:
            return s_TVLCOM_FloatToMilliSigned(Board1_TEMP);
        case TVLCOM_DATA_TEMP2_TEMPERATURE:
            return s_TVLCOM_FloatToMilliSigned(Board2_TEMP);
        case TVLCOM_DATA_OTP_VALUE:
            return s_TVLCOM_GetOtpValueMc();
        case TVLCOM_DATA_OTP_SET_VALUE:
            return s_TVLCOM_FloatToMilliSigned(MAX_OTP_VAL);
        default:
            return (int32_t)s_TVLCOM_GetU32ValueByType(type);
    }
}

static uint16_t s_TVLCOM_AppendTlvU8(uint8_t *payload, uint16_t offset, uint8_t type, uint8_t value)
{
    if ((uint16_t)(offset + 4U) > TVLCOM_MAX_PAYLOAD_SIZE)
    {
        return 0U;
    }

    payload[offset++] = type;
    s_TVLCOM_WriteLe16(&payload[offset], 1U);
    offset = (uint16_t)(offset + 2U);
    payload[offset++] = value;
    return offset;
}

static uint16_t s_TVLCOM_AppendTlvU32(uint8_t *payload, uint16_t offset, uint8_t type, uint32_t value)
{
    if ((uint16_t)(offset + 7U) > TVLCOM_MAX_PAYLOAD_SIZE)
    {
        return 0U;
    }

    payload[offset++] = type;
    s_TVLCOM_WriteLe16(&payload[offset], 4U);
    offset = (uint16_t)(offset + 2U);
    s_TVLCOM_WriteLe32(&payload[offset], value);
    offset = (uint16_t)(offset + 4U);
    return offset;
}

static uint16_t s_TVLCOM_AppendTlvS32(uint8_t *payload, uint16_t offset, uint8_t type, int32_t value)
{
    return s_TVLCOM_AppendTlvU32(payload, offset, type, (uint32_t)value);
}

static uint16_t s_TVLCOM_AppendTypedValue(uint8_t *payload, uint16_t offset, uint8_t type)
{
    const TVLCOM_DataMeta *meta = s_TVLCOM_GetMeta(type);

    if (meta == NULL)
    {
        return 0U;
    }

    if (type == TVLCOM_DATA_CC_CV_MODE)
    {
        return s_TVLCOM_AppendTlvU8(payload, offset, type, s_TVLCOM_GetCvccMode());
    }
    if (type == TVLCOM_DATA_POWER_STATE)
    {
        return s_TVLCOM_AppendTlvU8(payload, offset, type, (uint8_t)(DF.OUTPUT_Flag != 0U ? 1U : 0U));
    }
    if (type == TVLCOM_DATA_STATE_MACHINE_FLAG_BITS)
    {
        return s_TVLCOM_AppendTlvU8(payload, offset, type, s_TVLCOM_GetStateFlagBits());
    }
    if (type == TVLCOM_DATA_STATE_MACHINE_STATE)
    {
        return s_TVLCOM_AppendTlvU8(payload, offset, type, s_TVLCOM_GetStageMode());
    }
    if (meta->signedValue != 0U)
    {
        return s_TVLCOM_AppendTlvS32(payload, offset, type, s_TVLCOM_GetS32ValueByType(type));
    }
    return s_TVLCOM_AppendTlvU32(payload, offset, type, s_TVLCOM_GetU32ValueByType(type));
}

static uint8_t s_TVLCOM_IsAllowedFastStreamType(uint8_t type)
{
    return (uint8_t)((type == TVLCOM_DATA_INPUT_VOLTAGE) ||
                     (type == TVLCOM_DATA_INPUT_CURRENT) ||
                     (type == TVLCOM_DATA_OUTPUT_VOLTAGE) ||
                     (type == TVLCOM_DATA_OUTPUT_CURRENT));
}

static uint8_t s_TVLCOM_IsAllowedSlowStreamType(uint8_t type)
{
    return (uint8_t)((type == TVLCOM_DATA_CORE_TEMPERATURE) ||
                     (type == TVLCOM_DATA_BOARD_TEMPERATURE) ||
                     (type == TVLCOM_DATA_TEMP2_TEMPERATURE) ||
                     (type == TVLCOM_DATA_FAN_SPEED) ||
                     (type == TVLCOM_DATA_FAN_SET_VALUE));
}

static uint16_t s_TVLCOM_AppendStreamGroup(uint8_t *buffer, uint16_t offset, const uint8_t *types, uint8_t count)
{
    uint8_t i;

    for (i = 0U; i < count; ++i)
    {
        uint32_t value = s_TVLCOM_GetU32ValueByType(types[i]);

        if ((types[i] == TVLCOM_DATA_CORE_TEMPERATURE) ||
            (types[i] == TVLCOM_DATA_BOARD_TEMPERATURE) ||
            (types[i] == TVLCOM_DATA_TEMP2_TEMPERATURE))
        {
            int32_t temperatureValue = s_TVLCOM_GetS32ValueByType(types[i]);

            if (temperatureValue < 0)
            {
                value = 0U;
            }
            else
            {
                value = (uint32_t)temperatureValue;
            }
        }

        if ((uint16_t)(offset + 2U) > TVLCOM_STREAM_BUFFER_SIZE)
        {
            return 0U;
        }
        s_TVLCOM_WriteLe16(&buffer[offset], s_TVLCOM_ClampU16(value));
        offset = (uint16_t)(offset + 2U);

        if (i + 1U < count)
        {
            if ((uint16_t)(offset + 2U) > TVLCOM_STREAM_BUFFER_SIZE)
            {
                return 0U;
            }
            buffer[offset++] = TVLCOM_CHANNEL_SEPARATOR0;
            buffer[offset++] = TVLCOM_CHANNEL_SEPARATOR1;
        }
    }
    return offset;
}

static HAL_StatusTypeDef s_TVLCOM_SendCurrentPort(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return HAL_ERROR;
    }

    switch (s_TVLCOM_Context.currentPort)
    {
        case TVLCOM_PORT_USART1:
            return HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, len);
        case TVLCOM_PORT_USART2:
            return HAL_UART_Transmit_DMA(&huart2, (uint8_t *)data, len);
        case TVLCOM_PORT_CDC:
        default:
            return (CDC_Transmit_FS((uint8_t *)data, len) == USBD_OK) ? HAL_OK : HAL_BUSY;
    }
}

static HAL_StatusTypeDef s_TVLCOM_QueueBytes(const uint8_t *data, uint16_t len, uint8_t isStream)
{
    HAL_StatusTypeDef status;

    status = s_TVLCOM_SendCurrentPort(data, len);
    if (status == HAL_OK)
    {
        return HAL_OK;
    }

    if (s_TVLCOM_Context.pendingLength != 0U)
    {
        if ((isStream != 0U) && (s_TVLCOM_Context.pendingIsStream != 0U))
        {
            if (len <= TVLCOM_MAX_FRAME_SIZE)
            {
                memcpy(s_TVLCOM_Context.pendingBuffer, data, len);
                s_TVLCOM_Context.pendingLength = len;
                return HAL_OK;
            }
        }
        return HAL_BUSY;
    }

    if (len > TVLCOM_MAX_FRAME_SIZE)
    {
        return HAL_ERROR;
    }

    memcpy(s_TVLCOM_Context.pendingBuffer, data, len);
    s_TVLCOM_Context.pendingLength = len;
    s_TVLCOM_Context.pendingIsStream = isStream;
    return HAL_OK;
}

static void s_TVLCOM_FlushPending(void)
{
    if (s_TVLCOM_Context.pendingLength == 0U)
    {
        return;
    }

    if (s_TVLCOM_SendCurrentPort(s_TVLCOM_Context.pendingBuffer, s_TVLCOM_Context.pendingLength) == HAL_OK)
    {
        s_TVLCOM_Context.pendingLength = 0U;
        s_TVLCOM_Context.pendingIsStream = 0U;
    }
}

static void s_TVLCOM_SendSimpleFrame(uint8_t cmd, uint8_t seq)
{
    uint16_t frameLen = s_TVLCOM_BuildFrame(cmd, seq, NULL, 0U, s_TVLCOM_Context.txBuffer);

    (void)s_TVLCOM_QueueBytes(s_TVLCOM_Context.txBuffer, frameLen, 0U);
}

static void s_TVLCOM_SendPayloadFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    uint16_t frameLen = s_TVLCOM_BuildFrame(cmd, seq, payload, payloadLen, s_TVLCOM_Context.txBuffer);

    (void)s_TVLCOM_QueueBytes(s_TVLCOM_Context.txBuffer, frameLen, 0U);
}

static void s_TVLCOM_UpdateControlReferenceFromSetting(void)
{
    CtrValue.Vout_SETref = (int32_t)((SET_Value.Vout / BSP_POWER_VOUT_SENSE_SCALE) / REF_3V3 * ADC_MAX_VALUE);
    CtrValue.Iout_ref = (int32_t)(((BSP_POWER_CURRENT_BIAS_V + SET_Value.Iout * BSP_POWER_CURRENT_SENSE_V_PER_A) / REF_3V3) * ADC_MAX_VALUE);
}

static uint8_t s_TVLCOM_HandleReadLikePayload(const uint8_t *payload, uint16_t payloadLen, uint8_t *responsePayload, uint16_t *responseLen)
{
    uint16_t offset = 0U;
    uint16_t outOffset = 0U;

    while (offset < payloadLen)
    {
        uint8_t type;
        uint16_t length;
        const TVLCOM_DataMeta *meta;

        if ((uint16_t)(offset + 3U) > payloadLen)
        {
            return 0U;
        }

        type = payload[offset++];
        length = s_TVLCOM_ReadLe16(&payload[offset]);
        offset = (uint16_t)(offset + 2U);

        if ((uint16_t)(offset + length) > payloadLen)
        {
            return 0U;
        }
        if (length != 0U)
        {
            return 0U;
        }

        if (type == TVLCOM_DATA_DEBUG_SNAPSHOT)
        {
            uint32_t i;

            for (i = 0U; i < (uint32_t)(sizeof(s_TVLCOM_DebugSnapshotTypes) / sizeof(s_TVLCOM_DebugSnapshotTypes[0])); ++i)
            {
                outOffset = s_TVLCOM_AppendTypedValue(responsePayload, outOffset, s_TVLCOM_DebugSnapshotTypes[i]);
                if (outOffset == 0U)
                {
                    return 0U;
                }
            }
        }
        else
        {
            meta = s_TVLCOM_GetMeta(type);
            if ((meta == NULL) || ((meta->access & TVLCOM_ACCESS_READ) == 0U))
            {
                return 0U;
            }

            outOffset = s_TVLCOM_AppendTypedValue(responsePayload, outOffset, type);
            if (outOffset == 0U)
            {
                return 0U;
            }
        }
    }

    *responseLen = outOffset;
    return 1U;
}

static void s_TVLCOM_HandleRead(uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    uint8_t responsePayload[TVLCOM_MAX_PAYLOAD_SIZE];
    uint16_t responseLen = 0U;

    if (s_TVLCOM_HandleReadLikePayload(payload, payloadLen, responsePayload, &responseLen) == 0U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    s_TVLCOM_SendPayloadFrame(TVLCOM_CMD_ACK, seq, responsePayload, responseLen);
}

static void s_TVLCOM_HandleReport(uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    uint8_t responsePayload[TVLCOM_MAX_PAYLOAD_SIZE];
    uint16_t responseLen = 0U;
    uint32_t i;

    if (payloadLen != 0U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    for (i = 0U; i < (uint32_t)(sizeof(s_TVLCOM_ReportTypes) / sizeof(s_TVLCOM_ReportTypes[0])); ++i)
    {
        responseLen = s_TVLCOM_AppendTypedValue(responsePayload, responseLen, s_TVLCOM_ReportTypes[i]);
        if (responseLen == 0U)
        {
            s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
            return;
        }
    }

    s_TVLCOM_SendPayloadFrame(TVLCOM_CMD_REPORT, seq, responsePayload, responseLen);
}

static void s_TVLCOM_HandleWrite(uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    float newSetVoltage = SET_Value.Vout;
    float newSetCurrent = SET_Value.Iout;
    float newOtp = MAX_OTP_VAL;
    float newOvp = MAX_VOUT_OVP_VAL;
    float newOcp = MAX_VOUT_OCP_VAL;
    uint16_t newFanPermille = s_TVLCOM_Context.fanSetPermille;
    uint8_t fanWritten = 0U;
    uint8_t powerWritten = 0U;
    uint8_t powerEnabled = (uint8_t)(DF.OUTPUT_Flag != 0U ? 1U : 0U);
    uint16_t offset = 0U;

    while (offset < payloadLen)
    {
        uint8_t type;
        uint16_t length;
        const uint8_t *value;
        const TVLCOM_DataMeta *meta;

        if ((uint16_t)(offset + 3U) > payloadLen)
        {
            s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
            return;
        }

        type = payload[offset++];
        length = s_TVLCOM_ReadLe16(&payload[offset]);
        offset = (uint16_t)(offset + 2U);
        if ((uint16_t)(offset + length) > payloadLen)
        {
            s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
            return;
        }

        value = &payload[offset];
        offset = (uint16_t)(offset + length);

        meta = s_TVLCOM_GetMeta(type);
        if ((meta == NULL) || ((meta->access & TVLCOM_ACCESS_WRITE) == 0U) || (length != meta->length))
        {
            s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
            return;
        }

        switch (type)
        {
            case TVLCOM_DATA_SET_VOLTAGE_LIMIT:
                newSetVoltage = (float)s_TVLCOM_ReadLe32(value) / 1000.0F;
                break;
            case TVLCOM_DATA_SET_CURRENT_LIMIT:
                newSetCurrent = (float)s_TVLCOM_ReadLe32(value) / 1000.0F;
                break;
            case TVLCOM_DATA_POWER_STATE:
                powerWritten = 1U;
                powerEnabled = (uint8_t)((value[0] != 0U) ? 1U : 0U);
                break;
            case TVLCOM_DATA_OTP_SET_VALUE:
                newOtp = (float)((int32_t)s_TVLCOM_ReadLe32(value)) / 1000.0F;
                break;
            case TVLCOM_DATA_OVP_SET_VALUE:
                newOvp = (float)s_TVLCOM_ReadLe32(value) / 1000.0F;
                break;
            case TVLCOM_DATA_OCP_SET_VALUE:
                newOcp = (float)s_TVLCOM_ReadLe32(value) / 1000.0F;
                break;
            case TVLCOM_DATA_FAN_SET_VALUE:
            {
                uint32_t fanValue = s_TVLCOM_ReadLe32(value);

                if (fanValue > 1000U)
                {
                    s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
                    return;
                }
                fanWritten = 1U;
                newFanPermille = (uint16_t)fanValue;
                break;
            }
            default:
                s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
                return;
        }
    }

    if (newSetVoltage < 0.0F)
    {
        newSetVoltage = 0.0F;
    }
    if (newSetCurrent < 0.0F)
    {
        newSetCurrent = 0.0F;
    }
    if (newOtp < 0.0F)
    {
        newOtp = 0.0F;
    }
    if (newOvp < 0.0F)
    {
        newOvp = 0.0F;
    }
    if (newOcp < 0.0F)
    {
        newOcp = 0.0F;
    }

    SET_Value.Vout = newSetVoltage;
    SET_Value.Iout = newSetCurrent;
    MAX_OTP_VAL = newOtp;
    MAX_VOUT_OVP_VAL = newOvp;
    MAX_VOUT_OCP_VAL = newOcp;
    SET_Value.SET_modified_flag = 1.0F;
    s_TVLCOM_UpdateControlReferenceFromSetting();

    if (powerWritten != 0U)
    {
        DF.OUTPUT_Flag = powerEnabled;
        if (powerEnabled == 0U)
        {
            DF.PWMENFlag = 0U;
            if (DF.SMFlag != Err)
            {
                DF.SMFlag = Wait;
            }
        }
    }

    if (fanWritten != 0U)
    {
        s_TVLCOM_Context.fanManualEnable = 1U;
        s_TVLCOM_Context.fanSetPermille = newFanPermille;
        FAN_PWM_set((uint16_t)(newFanPermille / 10U));
        s_TVLCOM_Context.lastFanApplyTick = HAL_GetTick();
    }

    s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_ACK, seq);
}

static uint8_t s_TVLCOM_ParseStreamTypes(const uint8_t *payload, uint16_t payloadLen, uint16_t *offset, uint8_t *types, uint8_t count, uint8_t fastGroup)
{
    uint8_t i;

    for (i = 0U; i < count; ++i)
    {
        uint8_t type;
        uint16_t length;

        if ((uint16_t)(*offset + 3U) > payloadLen)
        {
            return 0U;
        }
        type = payload[(*offset)++];
        length = s_TVLCOM_ReadLe16(&payload[*offset]);
        *offset = (uint16_t)(*offset + 2U);

        if (length != 0U)
        {
            return 0U;
        }

        if (fastGroup != 0U)
        {
            if (s_TVLCOM_IsAllowedFastStreamType(type) == 0U)
            {
                return 0U;
            }
        }
        else
        {
            if (s_TVLCOM_IsAllowedSlowStreamType(type) == 0U)
            {
                return 0U;
            }
        }

        types[i] = type;
    }

    return 1U;
}

static void s_TVLCOM_HandleStreamStart(uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    uint16_t offset = 0U;
    uint16_t fastPeriod;
    uint16_t slowPeriod;
    uint8_t fastCount;
    uint8_t slowCount;

    if (payloadLen < 5U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    fastPeriod = s_TVLCOM_ReadLe16(&payload[offset]);
    offset = (uint16_t)(offset + 2U);
    fastCount = payload[offset++];

    if ((fastCount == 0U) || (fastCount > 8U))
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }
    if (s_TVLCOM_ParseStreamTypes(payload, payloadLen, &offset, s_TVLCOM_Context.stream.fastTypes, fastCount, 1U) == 0U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    if ((uint16_t)(offset + 3U) > payloadLen)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    slowPeriod = s_TVLCOM_ReadLe16(&payload[offset]);
    offset = (uint16_t)(offset + 2U);
    slowCount = payload[offset++];

    if (slowCount > 8U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }
    if (s_TVLCOM_ParseStreamTypes(payload, payloadLen, &offset, s_TVLCOM_Context.stream.slowTypes, slowCount, 0U) == 0U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }
    if (offset != payloadLen)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }
    if (fastPeriod == 0U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }
    if ((slowCount > 0U) && ((slowPeriod == 0U) || ((slowPeriod % fastPeriod) != 0U)))
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    s_TVLCOM_Context.stream.fastCount = fastCount;
    s_TVLCOM_Context.stream.slowCount = slowCount;
    s_TVLCOM_Context.stream.fastPeriodMs = fastPeriod;
    s_TVLCOM_Context.stream.slowPeriodMs = slowPeriod;
    s_TVLCOM_Context.stream.lastFastTick = HAL_GetTick();
    s_TVLCOM_Context.stream.lastSlowTick = 0U;
    s_TVLCOM_Context.stream.enabled = 1U;
    s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_ACK, seq);
}

static void s_TVLCOM_HandleStreamStop(uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    (void)payload;
    if (payloadLen != 0U)
    {
        s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
        return;
    }

    s_TVLCOM_Context.stream.enabled = 0U;
    s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_ACK, seq);
}

static void s_TVLCOM_HandleFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t payloadLen)
{
    switch (cmd)
    {
        case TVLCOM_CMD_READ:
            s_TVLCOM_HandleRead(seq, payload, payloadLen);
            break;
        case TVLCOM_CMD_WRITE:
            s_TVLCOM_HandleWrite(seq, payload, payloadLen);
            break;
        case TVLCOM_CMD_REPORT:
            s_TVLCOM_HandleReport(seq, payload, payloadLen);
            break;
        case TVLCOM_CMD_STREAM_START:
            s_TVLCOM_HandleStreamStart(seq, payload, payloadLen);
            break;
        case TVLCOM_CMD_STREAM_STOP:
            s_TVLCOM_HandleStreamStop(seq, payload, payloadLen);
            break;
        default:
            s_TVLCOM_SendSimpleFrame(TVLCOM_CMD_NACK, seq);
            break;
    }
}

static void s_TVLCOM_ParseRxBuffer(void)
{
    while (s_TVLCOM_Context.rxFrameLength >= 6U)
    {
        uint16_t bodyLen;
        uint16_t totalLen;
        uint16_t crcExpected;
        uint16_t crcActual;

        if ((s_TVLCOM_Context.rxFrameBuffer[0] != TVLCOM_SOF0) ||
            (s_TVLCOM_Context.rxFrameBuffer[1] != TVLCOM_SOF1))
        {
            memmove(&s_TVLCOM_Context.rxFrameBuffer[0], &s_TVLCOM_Context.rxFrameBuffer[1], (size_t)(s_TVLCOM_Context.rxFrameLength - 1U));
            --s_TVLCOM_Context.rxFrameLength;
            continue;
        }

        bodyLen = s_TVLCOM_ReadLe16(&s_TVLCOM_Context.rxFrameBuffer[2]);
        if ((bodyLen < 2U) || (bodyLen > (TVLCOM_MAX_PAYLOAD_SIZE + 2U)))
        {
            memmove(&s_TVLCOM_Context.rxFrameBuffer[0], &s_TVLCOM_Context.rxFrameBuffer[1], (size_t)(s_TVLCOM_Context.rxFrameLength - 1U));
            --s_TVLCOM_Context.rxFrameLength;
            continue;
        }

        totalLen = (uint16_t)(bodyLen + 6U);
        if (s_TVLCOM_Context.rxFrameLength < totalLen)
        {
            break;
        }

        crcExpected = s_TVLCOM_ReadLe16(&s_TVLCOM_Context.rxFrameBuffer[totalLen - 2U]);
        crcActual = s_TVLCOM_Crc16Modbus(s_TVLCOM_Context.rxFrameBuffer, (uint16_t)(totalLen - 2U));
        if (crcExpected != crcActual)
        {
            memmove(&s_TVLCOM_Context.rxFrameBuffer[0], &s_TVLCOM_Context.rxFrameBuffer[1], (size_t)(s_TVLCOM_Context.rxFrameLength - 1U));
            --s_TVLCOM_Context.rxFrameLength;
            continue;
        }

        s_TVLCOM_HandleFrame(s_TVLCOM_Context.rxFrameBuffer[4],
                             s_TVLCOM_Context.rxFrameBuffer[5],
                             &s_TVLCOM_Context.rxFrameBuffer[6],
                             (uint16_t)(bodyLen - 2U));

        if (s_TVLCOM_Context.rxFrameLength > totalLen)
        {
            memmove(&s_TVLCOM_Context.rxFrameBuffer[0],
                    &s_TVLCOM_Context.rxFrameBuffer[totalLen],
                    (size_t)(s_TVLCOM_Context.rxFrameLength - totalLen));
        }
        s_TVLCOM_Context.rxFrameLength = (uint16_t)(s_TVLCOM_Context.rxFrameLength - totalLen);
    }
}

static void s_TVLCOM_OnRxBytes(TVLCOM_Port port, const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return;
    }
    if (port != s_TVLCOM_Context.currentPort)
    {
        return;
    }

    while (len > 0U)
    {
        uint16_t copyLen = len;

        if ((uint16_t)(s_TVLCOM_Context.rxFrameLength + copyLen) > TVLCOM_MAX_FRAME_SIZE)
        {
            s_TVLCOM_Context.rxFrameLength = 0U;
            if (copyLen > TVLCOM_MAX_FRAME_SIZE)
            {
                data = &data[copyLen - TVLCOM_MAX_FRAME_SIZE];
                copyLen = TVLCOM_MAX_FRAME_SIZE;
            }
        }

        memcpy(&s_TVLCOM_Context.rxFrameBuffer[s_TVLCOM_Context.rxFrameLength], data, copyLen);
        s_TVLCOM_Context.rxFrameLength = (uint16_t)(s_TVLCOM_Context.rxFrameLength + copyLen);
        s_TVLCOM_ParseRxBuffer();

        data = &data[copyLen];
        len = (uint16_t)(len - copyLen);
    }
}

static void s_TVLCOM_ProcessUartRxEvent(UART_HandleTypeDef *huart, uint16_t size)
{
    uint8_t *buffer;
    uint16_t *lastPos;
    TVLCOM_Port port;
    uint16_t currentPos = size;

    if (huart == &huart1)
    {
        buffer = s_TVLCOM_Context.uart1RxBuffer;
        lastPos = &s_TVLCOM_Context.uart1RxLastPos;
        port = TVLCOM_PORT_USART1;
    }
    else if (huart == &huart2)
    {
        buffer = s_TVLCOM_Context.uart2RxBuffer;
        lastPos = &s_TVLCOM_Context.uart2RxLastPos;
        port = TVLCOM_PORT_USART2;
    }
    else
    {
        return;
    }

    if (currentPos > TVLCOM_UART_RX_DMA_SIZE)
    {
        currentPos = TVLCOM_UART_RX_DMA_SIZE;
    }

    if (currentPos > *lastPos)
    {
        s_TVLCOM_OnRxBytes(port, &buffer[*lastPos], (uint16_t)(currentPos - *lastPos));
    }
    else if (currentPos < *lastPos)
    {
        s_TVLCOM_OnRxBytes(port, &buffer[*lastPos], (uint16_t)(TVLCOM_UART_RX_DMA_SIZE - *lastPos));
        if (currentPos > 0U)
        {
            s_TVLCOM_OnRxBytes(port, &buffer[0], currentPos);
        }
    }
    else if (currentPos == TVLCOM_UART_RX_DMA_SIZE)
    {
        s_TVLCOM_OnRxBytes(port, &buffer[*lastPos], (uint16_t)(TVLCOM_UART_RX_DMA_SIZE - *lastPos));
    }

    *lastPos = (currentPos >= TVLCOM_UART_RX_DMA_SIZE) ? 0U : currentPos;
}

static void s_TVLCOM_StartUartRxDma(UART_HandleTypeDef *huart, uint8_t *buffer)
{
    if (HAL_UARTEx_ReceiveToIdle_DMA(huart, buffer, TVLCOM_UART_RX_DMA_SIZE) == HAL_OK)
    {
        if (huart->hdmarx != NULL)
        {
            __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
        }
    }
}

void TVLCOM_Init(void)
{
    memset(&s_TVLCOM_Context, 0, sizeof(s_TVLCOM_Context));
    s_TVLCOM_Context.currentPort = TVLCOM_PORT_CDC;
    s_TVLCOM_Context.fanSetPermille = 0U;
    s_TVLCOM_StartUartRxDma(&huart1, s_TVLCOM_Context.uart1RxBuffer);
    s_TVLCOM_StartUartRxDma(&huart2, s_TVLCOM_Context.uart2RxBuffer);
}

void TVLCOM_RunTask(void)
{
    uint32_t tickNow = HAL_GetTick();

    s_TVLCOM_FlushPending();

    if ((s_TVLCOM_Context.fanManualEnable != 0U) &&
        ((tickNow - s_TVLCOM_Context.lastFanApplyTick) >= 50U))
    {
        FAN_PWM_set((uint16_t)(s_TVLCOM_Context.fanSetPermille / 10U));
        s_TVLCOM_Context.lastFanApplyTick = tickNow;
    }

    if ((s_TVLCOM_Context.stream.enabled == 0U) || (s_TVLCOM_Context.pendingLength != 0U))
    {
        return;
    }

    if ((tickNow - s_TVLCOM_Context.stream.lastFastTick) >= s_TVLCOM_Context.stream.fastPeriodMs)
    {
        uint8_t streamBuffer[TVLCOM_STREAM_BUFFER_SIZE];
        uint16_t streamLen;
        uint8_t appendSlow = 0U;

        streamLen = s_TVLCOM_AppendStreamGroup(streamBuffer, 0U, s_TVLCOM_Context.stream.fastTypes, s_TVLCOM_Context.stream.fastCount);
        if (streamLen == 0U)
        {
            return;
        }

        if ((s_TVLCOM_Context.stream.slowCount > 0U) &&
            ((s_TVLCOM_Context.stream.lastSlowTick == 0U) ||
             ((tickNow - s_TVLCOM_Context.stream.lastSlowTick) >= s_TVLCOM_Context.stream.slowPeriodMs)))
        {
            appendSlow = 1U;
            streamLen = s_TVLCOM_AppendStreamGroup(streamBuffer, streamLen, s_TVLCOM_Context.stream.slowTypes, s_TVLCOM_Context.stream.slowCount);
            if (streamLen == 0U)
            {
                return;
            }
        }

        if (s_TVLCOM_QueueBytes(streamBuffer, streamLen, 1U) == HAL_OK)
        {
            s_TVLCOM_Context.stream.lastFastTick = tickNow;
            if (appendSlow != 0U)
            {
                s_TVLCOM_Context.stream.lastSlowTick = tickNow;
            }
        }
    }
}

void TVLCOM_SelectPort(TVLCOM_Port port)
{
    s_TVLCOM_Context.currentPort = port;
    s_TVLCOM_Context.rxFrameLength = 0U;
    s_TVLCOM_Context.pendingLength = 0U;
    s_TVLCOM_Context.pendingIsStream = 0U;
    s_TVLCOM_Context.stream.enabled = 0U;
}

TVLCOM_Port TVLCOM_GetPort(void)
{
    return s_TVLCOM_Context.currentPort;
}

HAL_StatusTypeDef TVLCOM_SendBytes(const uint8_t *data, uint16_t len)
{
    return s_TVLCOM_QueueBytes(data, len, 0U);
}

void TVLCOM_OnUsbCdcRx(const uint8_t *data, uint16_t len)
{
    s_TVLCOM_OnRxBytes(TVLCOM_PORT_CDC, data, len);
}

void USER_tvlcomTransportOnUsbCdcRx(uint8_t *data, uint16_t len)
{
    TVLCOM_OnUsbCdcRx(data, len);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    s_TVLCOM_ProcessUartRxEvent(huart, Size);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        s_TVLCOM_Context.uart1RxLastPos = 0U;
        s_TVLCOM_StartUartRxDma(&huart1, s_TVLCOM_Context.uart1RxBuffer);
    }
    else if (huart == &huart2)
    {
        s_TVLCOM_Context.uart2RxLastPos = 0U;
        s_TVLCOM_StartUartRxDma(&huart2, s_TVLCOM_Context.uart2RxBuffer);
    }
}
