/**
 * @file user_tvlcom_protocol.c
 * @brief F4CP TVLCOM command protocol implementation.
 */

#include "user_tvlcom_protocol.h"

#include <string.h>

#include "power_ctrl.h"
#include "user_flash_store.h"

#define USER_TVLCOM_HEADER_SIZE 4U
#define USER_TVLCOM_CRC_SIZE 2U
#define USER_TVLCOM_MIN_BODY_SIZE 2U
#define USER_TVLCOM_TLV_HEADER_SIZE 3U

typedef struct
{
    uint8_t type;
    uint8_t len;
    uint8_t access;
} USER_tvlcomDataDescriptor_t;

typedef struct
{
    uint8_t type;
    uint16_t len;
    const uint8_t *value;
} USER_tvlcomTlvView_t;

typedef struct
{
    uint8_t has_settings;
    uint8_t has_enabled;
    POWER_ctrlSettings_t settings;
    uint8_t enabled;
} USER_tvlcomWriteStage_t;

static const USER_tvlcomDataDescriptor_t s_USER_tvlcomDataDescriptors[] = {
    {USER_TVLCOM_DATA_INPUT_VOLTAGE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_INPUT_CURRENT, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OUTPUT_VOLTAGE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OUTPUT_CURRENT, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_CORE_TEMPERATURE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_BOARD_TEMPERATURE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT, 4U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_SET_CURRENT_LIMIT, 4U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_CC_CV_MODE, 1U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_POWER_STATE, 1U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_FAULT_STATE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_STATE_MACHINE_FLAG_BITS, 1U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_STATE_MACHINE_STATE, 1U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_INPUT_VOLTAGE_RAW, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_INPUT_CURRENT_RAW, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OUTPUT_VOLTAGE_RAW, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OUTPUT_CURRENT_RAW, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OTP_VALUE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OTP_SET_VALUE, 4U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_OVP_VALUE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OVP_SET_VALUE, 4U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_OCP_VALUE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_OCP_SET_VALUE, 4U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_DUTY_CMD, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_PWM_A_COMPARE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_PWM_D_COMPARE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_FAN_SPEED, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_FAN_SET_VALUE, 4U, USER_TVLCOM_ACCESS_READ_WRITE},
    {USER_TVLCOM_DATA_DEBUG_SNAPSHOT, 0U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_LOOP_CURRENT_FEEDBACK, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_LOOP_CURRENT_REFERENCE, 4U, USER_TVLCOM_ACCESS_READ},
    {USER_TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE, 4U, USER_TVLCOM_ACCESS_READ},
};

static const uint8_t s_USER_tvlcomReportTypes[] = {
    USER_TVLCOM_DATA_INPUT_VOLTAGE,
    USER_TVLCOM_DATA_INPUT_CURRENT,
    USER_TVLCOM_DATA_OUTPUT_VOLTAGE,
    USER_TVLCOM_DATA_OUTPUT_CURRENT,
    USER_TVLCOM_DATA_CORE_TEMPERATURE,
    USER_TVLCOM_DATA_BOARD_TEMPERATURE,
    USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT,
    USER_TVLCOM_DATA_SET_CURRENT_LIMIT,
    USER_TVLCOM_DATA_CC_CV_MODE,
    USER_TVLCOM_DATA_POWER_STATE,
    USER_TVLCOM_DATA_FAULT_STATE,
    USER_TVLCOM_DATA_STATE_MACHINE_FLAG_BITS,
    USER_TVLCOM_DATA_STATE_MACHINE_STATE,
    USER_TVLCOM_DATA_INPUT_CURRENT_RAW,
    USER_TVLCOM_DATA_OUTPUT_CURRENT_RAW,
    USER_TVLCOM_DATA_OTP_VALUE,
    USER_TVLCOM_DATA_OTP_SET_VALUE,
    USER_TVLCOM_DATA_OVP_VALUE,
    USER_TVLCOM_DATA_OVP_SET_VALUE,
    USER_TVLCOM_DATA_OCP_VALUE,
    USER_TVLCOM_DATA_OCP_SET_VALUE,
    USER_TVLCOM_DATA_DUTY_CMD,
    USER_TVLCOM_DATA_PWM_A_COMPARE,
    USER_TVLCOM_DATA_PWM_D_COMPARE,
    USER_TVLCOM_DATA_FAN_SPEED,
    USER_TVLCOM_DATA_FAN_SET_VALUE,
    USER_TVLCOM_DATA_LOOP_CURRENT_FEEDBACK,
    USER_TVLCOM_DATA_LOOP_CURRENT_REFERENCE,
    USER_TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE,
};

static const uint8_t s_USER_tvlcomDebugSnapshotTypes[] = {
    USER_TVLCOM_DATA_OUTPUT_VOLTAGE_RAW,
    USER_TVLCOM_DATA_OUTPUT_VOLTAGE,
    USER_TVLCOM_DATA_INPUT_CURRENT_RAW,
    USER_TVLCOM_DATA_OUTPUT_CURRENT_RAW,
    USER_TVLCOM_DATA_INPUT_CURRENT,
    USER_TVLCOM_DATA_OUTPUT_CURRENT,
    USER_TVLCOM_DATA_LOOP_CURRENT_FEEDBACK,
    USER_TVLCOM_DATA_LOOP_CURRENT_REFERENCE,
    USER_TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE,
};

static uint16_t s_USER_tvlcomReadLe16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t s_USER_tvlcomReadLe32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static void s_USER_tvlcomWriteLe16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void s_USER_tvlcomWriteLe32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
    data[2] = (uint8_t)((value >> 16) & 0xFFU);
    data[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static const USER_tvlcomDataDescriptor_t *s_USER_tvlcomFindDescriptor(uint8_t type)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)(sizeof(s_USER_tvlcomDataDescriptors) / sizeof(s_USER_tvlcomDataDescriptors[0])); ++i)
    {
        if (s_USER_tvlcomDataDescriptors[i].type == type)
        {
            return &s_USER_tvlcomDataDescriptors[i];
        }
    }

    return NULL;
}

static uint8_t s_USER_tvlcomCanRead(const USER_tvlcomDataDescriptor_t *descriptor)
{
    return (descriptor != NULL) && ((descriptor->access & USER_TVLCOM_ACCESS_READ) != 0U);
}

static uint8_t s_USER_tvlcomCanWrite(const USER_tvlcomDataDescriptor_t *descriptor)
{
    return (descriptor != NULL) && ((descriptor->access & USER_TVLCOM_ACCESS_WRITE) != 0U);
}

static int s_USER_tvlcomAppendTlv(uint8_t *payload, uint16_t payload_cap, uint16_t *offset, uint8_t type, const uint8_t *value, uint16_t len)
{
    if ((payload == NULL) || (offset == NULL) || ((*offset + USER_TVLCOM_TLV_HEADER_SIZE + len) > payload_cap))
    {
        return USER_TVLCOM_STATUS_ERROR;
    }

    payload[*offset] = type;
    s_USER_tvlcomWriteLe16(&payload[*offset + 1U], len);
    *offset = (uint16_t)(*offset + USER_TVLCOM_TLV_HEADER_SIZE);

    if (len > 0U)
    {
        if (value == NULL)
        {
            return USER_TVLCOM_STATUS_ERROR;
        }
        memcpy(&payload[*offset], value, len);
        *offset = (uint16_t)(*offset + len);
    }

    return USER_TVLCOM_STATUS_OK;
}

static int s_USER_tvlcomAppendU8(uint8_t *payload, uint16_t payload_cap, uint16_t *offset, uint8_t type, uint8_t value)
{
    return s_USER_tvlcomAppendTlv(payload, payload_cap, offset, type, &value, 1U);
}

static int s_USER_tvlcomAppendU32(uint8_t *payload, uint16_t payload_cap, uint16_t *offset, uint8_t type, uint32_t value)
{
    uint8_t encoded[4];

    s_USER_tvlcomWriteLe32(encoded, value);
    return s_USER_tvlcomAppendTlv(payload, payload_cap, offset, type, encoded, 4U);
}

static int s_USER_tvlcomAppendI32(uint8_t *payload, uint16_t payload_cap, uint16_t *offset, uint8_t type, int32_t value)
{
    return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, (uint32_t)value);
}

static int s_USER_tvlcomAppendSnapshotType(uint8_t type, const POWER_ctrlSnapshot_t *snapshot, uint8_t *payload, uint16_t payload_cap, uint16_t *offset)
{
    if ((snapshot == NULL) || (payload == NULL) || (offset == NULL))
    {
        return USER_TVLCOM_STATUS_ERROR;
    }

    switch (type)
    {
        case USER_TVLCOM_DATA_INPUT_VOLTAGE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->input_voltage_mv);
        case USER_TVLCOM_DATA_INPUT_CURRENT:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->input_current_ma);
        case USER_TVLCOM_DATA_OUTPUT_VOLTAGE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->output_voltage_mv);
        case USER_TVLCOM_DATA_OUTPUT_CURRENT:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->output_current_ma);
        case USER_TVLCOM_DATA_CORE_TEMPERATURE:
            return s_USER_tvlcomAppendI32(payload, payload_cap, offset, type, snapshot->core_temperature_mc);
        case USER_TVLCOM_DATA_BOARD_TEMPERATURE:
            return s_USER_tvlcomAppendI32(payload, payload_cap, offset, type, snapshot->board_temperature_mc);
        case USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->settings.set_voltage_mv);
        case USER_TVLCOM_DATA_SET_CURRENT_LIMIT:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->settings.set_current_ma);
        case USER_TVLCOM_DATA_CC_CV_MODE:
            return s_USER_tvlcomAppendU8(payload, payload_cap, offset, type, snapshot->cvcc_mode);
        case USER_TVLCOM_DATA_POWER_STATE:
            return s_USER_tvlcomAppendU8(payload, payload_cap, offset, type, snapshot->power_enabled);
        case USER_TVLCOM_DATA_FAULT_STATE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->fault_flags);
        case USER_TVLCOM_DATA_STATE_MACHINE_FLAG_BITS:
            return s_USER_tvlcomAppendU8(payload, payload_cap, offset, type, snapshot->state_flag_bits);
        case USER_TVLCOM_DATA_STATE_MACHINE_STATE:
            return s_USER_tvlcomAppendU8(payload, payload_cap, offset, type, snapshot->stage_mode);
        case USER_TVLCOM_DATA_INPUT_VOLTAGE_RAW:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->input_voltage_raw);
        case USER_TVLCOM_DATA_INPUT_CURRENT_RAW:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->input_current_raw);
        case USER_TVLCOM_DATA_OUTPUT_VOLTAGE_RAW:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->output_voltage_raw);
        case USER_TVLCOM_DATA_OUTPUT_CURRENT_RAW:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->output_current_raw);
        case USER_TVLCOM_DATA_OTP_VALUE:
            return s_USER_tvlcomAppendI32(payload, payload_cap, offset, type, snapshot->otp_value_mc);
        case USER_TVLCOM_DATA_OTP_SET_VALUE:
            return s_USER_tvlcomAppendI32(payload, payload_cap, offset, type, snapshot->settings.otp_set_mc);
        case USER_TVLCOM_DATA_OVP_VALUE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->ovp_value_mv);
        case USER_TVLCOM_DATA_OVP_SET_VALUE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->settings.ovp_set_mv);
        case USER_TVLCOM_DATA_OCP_VALUE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->ocp_value_ma);
        case USER_TVLCOM_DATA_OCP_SET_VALUE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->settings.ocp_set_ma);
        case USER_TVLCOM_DATA_DUTY_CMD:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->duty_cmd);
        case USER_TVLCOM_DATA_PWM_A_COMPARE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->pwm_a_compare);
        case USER_TVLCOM_DATA_PWM_D_COMPARE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->pwm_d_compare);
        case USER_TVLCOM_DATA_FAN_SPEED:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->fan_speed_permille);
        case USER_TVLCOM_DATA_FAN_SET_VALUE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->settings.fan_set_permille);
        case USER_TVLCOM_DATA_LOOP_CURRENT_FEEDBACK:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->loop_current_feedback_ma);
        case USER_TVLCOM_DATA_LOOP_CURRENT_REFERENCE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->loop_current_reference_ma);
        case USER_TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE:
            return s_USER_tvlcomAppendU32(payload, payload_cap, offset, type, snapshot->voltage_loop_reference_mv);
        default:
            return USER_TVLCOM_STATUS_ERROR;
    }
}

static int s_USER_tvlcomParseNextTlv(const uint8_t *payload, uint16_t payload_len, uint16_t *offset, USER_tvlcomTlvView_t *tlv)
{
    uint16_t len;

    if ((payload == NULL) || (offset == NULL) || (tlv == NULL) || ((*offset + USER_TVLCOM_TLV_HEADER_SIZE) > payload_len))
    {
        return USER_TVLCOM_STATUS_ERROR;
    }

    len = s_USER_tvlcomReadLe16(&payload[*offset + 1U]);
    if ((*offset + USER_TVLCOM_TLV_HEADER_SIZE + len) > payload_len)
    {
        return USER_TVLCOM_STATUS_ERROR;
    }

    tlv->type = payload[*offset];
    tlv->len = len;
    tlv->value = &payload[*offset + USER_TVLCOM_TLV_HEADER_SIZE];
    *offset = (uint16_t)(*offset + USER_TVLCOM_TLV_HEADER_SIZE + len);
    return USER_TVLCOM_STATUS_OK;
}

static void s_USER_tvlcomSendResponse(USER_tvlcomContext_t *ctx, uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t payload_len)
{
    uint8_t frame[USER_TVLCOM_MAX_FRAME_SIZE];
    uint16_t frame_len;

    if ((ctx == NULL) || (ctx->send == NULL))
    {
        return;
    }

    frame_len = USER_tvlcomBuildFrame(cmd, seq, payload, payload_len, frame, (uint16_t)sizeof(frame));
    if (frame_len > 0U)
    {
        (void)ctx->send(frame, frame_len, ctx->send_user);
    }
}

static void s_USER_tvlcomSendNack(USER_tvlcomContext_t *ctx, uint8_t seq)
{
    s_USER_tvlcomSendResponse(ctx, USER_TVLCOM_CMD_NACK, seq, NULL, 0U);
}

static int s_USER_tvlcomBuildReadPayload(const uint8_t *request_payload, uint16_t request_len, uint8_t *response_payload, uint16_t response_cap, uint16_t *response_len)
{
    uint16_t offset = 0U;
    POWER_ctrlSnapshot_t snapshot;

    if ((response_payload == NULL) || (response_len == NULL))
    {
        return USER_TVLCOM_STATUS_ERROR;
    }

    *response_len = 0U;
    POWER_getAppSnapshot(&snapshot);

    while (offset < request_len)
    {
        USER_tvlcomTlvView_t tlv;
        const USER_tvlcomDataDescriptor_t *descriptor;

        if (s_USER_tvlcomParseNextTlv(request_payload, request_len, &offset, &tlv) != USER_TVLCOM_STATUS_OK)
        {
            return USER_TVLCOM_STATUS_ERROR;
        }

        descriptor = s_USER_tvlcomFindDescriptor(tlv.type);
        if (!s_USER_tvlcomCanRead(descriptor) || (tlv.len != 0U))
        {
            return USER_TVLCOM_STATUS_ERROR;
        }

        if (tlv.type == USER_TVLCOM_DATA_DEBUG_SNAPSHOT)
        {
            uint32_t i;

            for (i = 0U; i < (uint32_t)(sizeof(s_USER_tvlcomDebugSnapshotTypes) / sizeof(s_USER_tvlcomDebugSnapshotTypes[0])); ++i)
            {
                if (s_USER_tvlcomAppendSnapshotType(s_USER_tvlcomDebugSnapshotTypes[i], &snapshot, response_payload, response_cap, response_len) != USER_TVLCOM_STATUS_OK)
                {
                    return USER_TVLCOM_STATUS_ERROR;
                }
            }
        }
        else if (s_USER_tvlcomAppendSnapshotType(tlv.type, &snapshot, response_payload, response_cap, response_len) != USER_TVLCOM_STATUS_OK)
        {
            return USER_TVLCOM_STATUS_ERROR;
        }
    }

    return USER_TVLCOM_STATUS_OK;
}

static int s_USER_tvlcomApplyWritePayload(const uint8_t *payload, uint16_t payload_len)
{
    uint16_t offset = 0U;
    POWER_ctrlSnapshot_t snapshot;
    USER_tvlcomWriteStage_t stage;

    POWER_getAppSnapshot(&snapshot);
    memset(&stage, 0, sizeof(stage));
    stage.settings = snapshot.settings;
    stage.enabled = snapshot.power_enabled;

    while (offset < payload_len)
    {
        USER_tvlcomTlvView_t tlv;
        const USER_tvlcomDataDescriptor_t *descriptor;

        if (s_USER_tvlcomParseNextTlv(payload, payload_len, &offset, &tlv) != USER_TVLCOM_STATUS_OK)
        {
            return USER_TVLCOM_STATUS_ERROR;
        }

        descriptor = s_USER_tvlcomFindDescriptor(tlv.type);
        if (!s_USER_tvlcomCanWrite(descriptor) || (tlv.len != descriptor->len))
        {
            return USER_TVLCOM_STATUS_ERROR;
        }

        switch (tlv.type)
        {
            case USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT:
                stage.settings.set_voltage_mv = s_USER_tvlcomReadLe32(tlv.value);
                stage.has_settings = 1U;
                break;
            case USER_TVLCOM_DATA_SET_CURRENT_LIMIT:
                stage.settings.set_current_ma = s_USER_tvlcomReadLe32(tlv.value);
                stage.has_settings = 1U;
                break;
            case USER_TVLCOM_DATA_POWER_STATE:
                if (tlv.value[0] > 1U)
                {
                    return USER_TVLCOM_STATUS_ERROR;
                }
                stage.enabled = tlv.value[0];
                stage.has_enabled = 1U;
                break;
            case USER_TVLCOM_DATA_OTP_SET_VALUE:
                stage.settings.otp_set_mc = (int32_t)s_USER_tvlcomReadLe32(tlv.value);
                stage.has_settings = 1U;
                break;
            case USER_TVLCOM_DATA_OVP_SET_VALUE:
                stage.settings.ovp_set_mv = s_USER_tvlcomReadLe32(tlv.value);
                stage.has_settings = 1U;
                break;
            case USER_TVLCOM_DATA_OCP_SET_VALUE:
                stage.settings.ocp_set_ma = s_USER_tvlcomReadLe32(tlv.value);
                stage.has_settings = 1U;
                break;
            case USER_TVLCOM_DATA_FAN_SET_VALUE:
                stage.settings.fan_set_permille = s_USER_tvlcomReadLe32(tlv.value);
                if (stage.settings.fan_set_permille > 1000U)
                {
                    return USER_TVLCOM_STATUS_ERROR;
                }
                stage.has_settings = 1U;
                break;
            default:
                return USER_TVLCOM_STATUS_ERROR;
        }
    }

    if (stage.has_settings != 0U)
    {
        POWER_applyAppSettings(&stage.settings);
        USER_flashStoreRequestAppSave();
    }

    if (stage.has_enabled != 0U)
    {
        POWER_setAppEnabled(stage.enabled);
    }

    return USER_TVLCOM_STATUS_OK;
}

static int s_USER_tvlcomBuildReportPayload(uint8_t *response_payload, uint16_t response_cap, uint16_t *response_len)
{
    uint32_t i;
    POWER_ctrlSnapshot_t snapshot;

    if ((response_payload == NULL) || (response_len == NULL))
    {
        return USER_TVLCOM_STATUS_ERROR;
    }

    *response_len = 0U;
    POWER_getAppSnapshot(&snapshot);

    for (i = 0U; i < (uint32_t)(sizeof(s_USER_tvlcomReportTypes) / sizeof(s_USER_tvlcomReportTypes[0])); ++i)
    {
        if (s_USER_tvlcomAppendSnapshotType(s_USER_tvlcomReportTypes[i], &snapshot, response_payload, response_cap, response_len) != USER_TVLCOM_STATUS_OK)
        {
            return USER_TVLCOM_STATUS_ERROR;
        }
    }

    return USER_TVLCOM_STATUS_OK;
}

static void s_USER_tvlcomHandleFrame(USER_tvlcomContext_t *ctx, const uint8_t *frame, uint16_t frame_len)
{
    const uint16_t body_len = s_USER_tvlcomReadLe16(&frame[2]);
    const uint8_t *body = &frame[USER_TVLCOM_HEADER_SIZE];
    const uint8_t cmd = body[0];
    const uint8_t seq = body[1];
    const uint8_t *payload = &body[2];
    const uint16_t payload_len = (uint16_t)(body_len - USER_TVLCOM_MIN_BODY_SIZE);
    uint8_t response_payload[USER_TVLCOM_MAX_PAYLOAD_SIZE];
    uint16_t response_len = 0U;

    if ((frame_len < (USER_TVLCOM_HEADER_SIZE + USER_TVLCOM_MIN_BODY_SIZE + USER_TVLCOM_CRC_SIZE)) ||
        (body_len < USER_TVLCOM_MIN_BODY_SIZE))
    {
        return;
    }

    switch (cmd)
    {
        case USER_TVLCOM_CMD_READ:
            if (s_USER_tvlcomBuildReadPayload(payload, payload_len, response_payload, (uint16_t)sizeof(response_payload), &response_len) == USER_TVLCOM_STATUS_OK)
            {
                s_USER_tvlcomSendResponse(ctx, USER_TVLCOM_CMD_ACK, seq, response_payload, response_len);
            }
            else
            {
                s_USER_tvlcomSendNack(ctx, seq);
            }
            break;

        case USER_TVLCOM_CMD_WRITE:
            if (s_USER_tvlcomApplyWritePayload(payload, payload_len) == USER_TVLCOM_STATUS_OK)
            {
                s_USER_tvlcomSendResponse(ctx, USER_TVLCOM_CMD_ACK, seq, NULL, 0U);
            }
            else
            {
                s_USER_tvlcomSendNack(ctx, seq);
            }
            break;

        case USER_TVLCOM_CMD_REPORT:
            if ((payload_len == 0U) &&
                (s_USER_tvlcomBuildReportPayload(response_payload, (uint16_t)sizeof(response_payload), &response_len) == USER_TVLCOM_STATUS_OK))
            {
                s_USER_tvlcomSendResponse(ctx, USER_TVLCOM_CMD_REPORT, seq, response_payload, response_len);
            }
            else
            {
                s_USER_tvlcomSendNack(ctx, seq);
            }
            break;

        default:
            s_USER_tvlcomSendNack(ctx, seq);
            break;
    }
}

static void s_USER_tvlcomResetParser(USER_tvlcomContext_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->rx_len = 0U;
    ctx->expected_len = 0U;
    ctx->sof_state = 0U;
}

uint16_t USER_tvlcomCrc16Modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;

    if (data == NULL)
    {
        return crc;
    }

    for (i = 0U; i < len; ++i)
    {
        uint8_t bit;
        crc ^= data[i];
        for (bit = 0U; bit < 8U; ++bit)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1) ^ 0xA001U);
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

uint16_t USER_tvlcomBuildFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t payload_len, uint8_t *out, uint16_t out_cap)
{
    uint16_t body_len = (uint16_t)(USER_TVLCOM_MIN_BODY_SIZE + payload_len);
    uint16_t frame_len = (uint16_t)(USER_TVLCOM_HEADER_SIZE + body_len + USER_TVLCOM_CRC_SIZE);
    uint16_t crc;

    if ((out == NULL) || (body_len > USER_TVLCOM_MAX_PAYLOAD_SIZE) || (frame_len > out_cap) || ((payload_len > 0U) && (payload == NULL)))
    {
        return 0U;
    }

    out[0] = USER_TVLCOM_SOF0;
    out[1] = USER_TVLCOM_SOF1;
    s_USER_tvlcomWriteLe16(&out[2], body_len);
    out[4] = cmd;
    out[5] = seq;

    if (payload_len > 0U)
    {
        memcpy(&out[6], payload, payload_len);
    }

    crc = USER_tvlcomCrc16Modbus(&out[2], (uint16_t)(2U + body_len));
    s_USER_tvlcomWriteLe16(&out[USER_TVLCOM_HEADER_SIZE + body_len], crc);
    return frame_len;
}

void USER_tvlcomInit(USER_tvlcomContext_t *ctx, USER_tvlcomSendFn_t send, void *send_user)
{
    if (ctx == NULL)
    {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->send = send;
    ctx->send_user = send_user;
}

void USER_tvlcomFeed(USER_tvlcomContext_t *ctx, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if ((ctx == NULL) || (data == NULL))
    {
        return;
    }

    for (i = 0U; i < len; ++i)
    {
        uint8_t byte = data[i];

        if (ctx->sof_state == 0U)
        {
            if (byte == USER_TVLCOM_SOF0)
            {
                ctx->rx_buf[0] = byte;
                ctx->rx_len = 1U;
                ctx->sof_state = 1U;
            }
            continue;
        }

        if (ctx->sof_state == 1U)
        {
            if (byte == USER_TVLCOM_SOF1)
            {
                ctx->rx_buf[1] = byte;
                ctx->rx_len = 2U;
                ctx->sof_state = 2U;
            }
            else if (byte == USER_TVLCOM_SOF0)
            {
                ctx->rx_buf[0] = byte;
                ctx->rx_len = 1U;
            }
            else
            {
                s_USER_tvlcomResetParser(ctx);
            }
            continue;
        }

        if (ctx->rx_len >= sizeof(ctx->rx_buf))
        {
            s_USER_tvlcomResetParser(ctx);
            continue;
        }

        ctx->rx_buf[ctx->rx_len++] = byte;

        if (ctx->rx_len == USER_TVLCOM_HEADER_SIZE)
        {
            const uint16_t body_len = s_USER_tvlcomReadLe16(&ctx->rx_buf[2]);
            if ((body_len < USER_TVLCOM_MIN_BODY_SIZE) || (body_len > USER_TVLCOM_MAX_PAYLOAD_SIZE))
            {
                s_USER_tvlcomResetParser(ctx);
                continue;
            }
            ctx->expected_len = (uint16_t)(USER_TVLCOM_HEADER_SIZE + body_len + USER_TVLCOM_CRC_SIZE);
        }

        if ((ctx->expected_len > 0U) && (ctx->rx_len == ctx->expected_len))
        {
            const uint16_t received_crc = s_USER_tvlcomReadLe16(&ctx->rx_buf[ctx->expected_len - USER_TVLCOM_CRC_SIZE]);
            const uint16_t calculated_crc = USER_tvlcomCrc16Modbus(&ctx->rx_buf[2], (uint16_t)(ctx->expected_len - 2U - USER_TVLCOM_CRC_SIZE));

            if (received_crc == calculated_crc)
            {
                s_USER_tvlcomHandleFrame(ctx, ctx->rx_buf, ctx->expected_len);
            }
            s_USER_tvlcomResetParser(ctx);
        }
    }
}
