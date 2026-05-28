/**
 * @file test_user_tvlcom_protocol.c
 * @brief Host-side tests for the F4CP TVLCOM wire protocol.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "power_ctrl.h"
#include "user_tvlcom_protocol.h"

static power_ctrl_snapshot_t g_snapshot;
static power_ctrl_settings_t g_applied_settings;
static uint8_t g_enabled;
static uint32_t g_apply_count;
static uint8_t g_tx[USER_TVLCOM_MAX_FRAME_SIZE];
static uint16_t g_tx_len;

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
    data[2] = (uint8_t)((value >> 16) & 0xFFU);
    data[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

void PowerCtrl_Init(void)
{
}

void PowerCtrl_GetSnapshot(power_ctrl_snapshot_t *snapshot)
{
    *snapshot = g_snapshot;
}

void PowerCtrl_ApplySettings(const power_ctrl_settings_t *settings)
{
    g_applied_settings = *settings;
    g_snapshot.settings = *settings;
    ++g_apply_count;
}

void PowerCtrl_SetEnabled(uint8_t enabled)
{
    g_enabled = enabled;
    g_snapshot.power_enabled = enabled;
}

static int mock_send(const uint8_t *data, uint16_t len, void *user)
{
    (void)user;
    memcpy(g_tx, data, len);
    g_tx_len = len;
    return 0;
}

static void reset_fixture(void)
{
    memset(&g_snapshot, 0, sizeof(g_snapshot));
    memset(&g_applied_settings, 0, sizeof(g_applied_settings));
    memset(g_tx, 0, sizeof(g_tx));
    g_tx_len = 0U;
    g_enabled = 0U;
    g_apply_count = 0U;

    g_snapshot.input_voltage_mv = 24000U;
    g_snapshot.input_current_ma = 1200U;
    g_snapshot.output_voltage_mv = 5000U;
    g_snapshot.output_current_ma = 900U;
    g_snapshot.core_temperature_mc = 42000;
    g_snapshot.board_temperature_mc = 35000;
    g_snapshot.input_voltage_raw = 1001U;
    g_snapshot.input_current_raw = 1002U;
    g_snapshot.output_voltage_raw = 1003U;
    g_snapshot.output_current_raw = 1004U;
    g_snapshot.settings.set_voltage_mv = 5000U;
    g_snapshot.settings.set_current_ma = 10000U;
    g_snapshot.settings.otp_set_mc = 80000;
    g_snapshot.settings.ovp_set_mv = 50000U;
    g_snapshot.settings.ocp_set_ma = 10500U;
    g_snapshot.settings.fan_set_permille = 250U;
    g_snapshot.power_enabled = 0U;
    g_snapshot.state_flag_bits = POWER_CTRL_STATE_FLAG_WAIT;
    g_snapshot.stage_mode = POWER_CTRL_STAGE_BUCK;
    g_snapshot.cvcc_mode = POWER_CTRL_CVCC_CV;
    g_snapshot.otp_value_mc = 78000;
    g_snapshot.ovp_value_mv = 50000U;
    g_snapshot.ocp_value_ma = 10500U;
    g_snapshot.duty_cmd = 123U;
    g_snapshot.pwm_a_compare = 456U;
    g_snapshot.pwm_d_compare = 789U;
    g_snapshot.fan_speed_permille = 250U;
    g_snapshot.loop_current_feedback_ma = 900U;
    g_snapshot.loop_current_reference_ma = 10000U;
    g_snapshot.voltage_loop_reference_mv = 5000U;
}

static uint16_t append_tlv(uint8_t *payload, uint16_t offset, uint8_t type, const uint8_t *value, uint16_t len)
{
    payload[offset++] = type;
    write_le16(&payload[offset], len);
    offset = (uint16_t)(offset + 2U);
    if (len > 0U)
    {
        memcpy(&payload[offset], value, len);
        offset = (uint16_t)(offset + len);
    }
    return offset;
}

static int expect(int condition, const char *name)
{
    if (!condition)
    {
        printf("FAIL: %s\n", name);
        return 0;
    }
    return 1;
}

static int frame_cmd_is(uint8_t cmd, uint8_t seq)
{
    return (g_tx_len >= 6U) &&
           (g_tx[0] == USER_TVLCOM_SOF0) &&
           (g_tx[1] == USER_TVLCOM_SOF1) &&
           (g_tx[4] == cmd) &&
           (g_tx[5] == seq);
}

static int test_crc(void)
{
    const uint8_t value[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    return expect(UserTvlCom_Crc16Modbus(value, (uint16_t)sizeof(value)) == 0x4B37U, "crc modbus vector");
}

static int test_report_split_feed(void)
{
    user_tvlcom_context_t ctx;
    uint8_t req[32];
    uint16_t req_len;
    uint16_t body_len;
    uint16_t tlv_offset;

    reset_fixture();
    UserTvlCom_Init(&ctx, mock_send, NULL);
    req_len = UserTvlCom_BuildFrame(USER_TVLCOM_CMD_REPORT, 0x21U, NULL, 0U, req, (uint16_t)sizeof(req));
    UserTvlCom_Feed(&ctx, req, 3U);
    UserTvlCom_Feed(&ctx, &req[3], (uint16_t)(req_len - 3U));

    body_len = read_le16(&g_tx[2]);
    tlv_offset = 6U;
    return expect(frame_cmd_is(USER_TVLCOM_CMD_REPORT, 0x21U), "report response cmd/seq") &&
           expect(body_len > 2U, "report nonempty payload") &&
           expect(g_tx[tlv_offset] == USER_TVLCOM_DATA_INPUT_VOLTAGE, "report first tlv type");
}

static int test_debug_snapshot(void)
{
    user_tvlcom_context_t ctx;
    uint8_t req[64];
    uint8_t payload[8];
    uint16_t payload_len = 0U;
    uint16_t req_len;
    uint16_t offset;

    reset_fixture();
    UserTvlCom_Init(&ctx, mock_send, NULL);
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_DEBUG_SNAPSHOT, NULL, 0U);
    req_len = UserTvlCom_BuildFrame(USER_TVLCOM_CMD_READ, 0x22U, payload, payload_len, req, (uint16_t)sizeof(req));
    UserTvlCom_Feed(&ctx, req, req_len);

    offset = 6U;
    return expect(frame_cmd_is(USER_TVLCOM_CMD_ACK, 0x22U), "debug ack") &&
           expect(g_tx[offset] == USER_TVLCOM_DATA_OUTPUT_VOLTAGE_RAW, "debug first tlv") &&
           expect(g_tx[(uint16_t)(offset + 7U)] == USER_TVLCOM_DATA_OUTPUT_VOLTAGE, "debug second tlv");
}

static int test_write_commit_and_rollback(void)
{
    user_tvlcom_context_t ctx;
    uint8_t req[96];
    uint8_t payload[64];
    uint8_t value[4];
    uint16_t payload_len = 0U;
    uint16_t req_len;

    reset_fixture();
    UserTvlCom_Init(&ctx, mock_send, NULL);

    write_le32(value, 12000U);
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT, value, 4U);
    write_le32(value, 3000U);
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_SET_CURRENT_LIMIT, value, 4U);
    value[0] = 1U;
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_POWER_STATE, value, 1U);
    req_len = UserTvlCom_BuildFrame(USER_TVLCOM_CMD_WRITE, 0x23U, payload, payload_len, req, (uint16_t)sizeof(req));
    UserTvlCom_Feed(&ctx, req, req_len);

    if (!expect(frame_cmd_is(USER_TVLCOM_CMD_ACK, 0x23U), "write ack") ||
        !expect(g_applied_settings.set_voltage_mv == 12000U, "write voltage committed") ||
        !expect(g_applied_settings.set_current_ma == 3000U, "write current committed") ||
        !expect(g_enabled == 1U, "write enabled committed"))
    {
        return 0;
    }

    payload_len = 0U;
    write_le32(value, 13000U);
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT, value, 4U);
    write_le32(value, 1U);
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_OUTPUT_VOLTAGE, value, 4U);
    req_len = UserTvlCom_BuildFrame(USER_TVLCOM_CMD_WRITE, 0x24U, payload, payload_len, req, (uint16_t)sizeof(req));
    UserTvlCom_Feed(&ctx, req, req_len);

    return expect(frame_cmd_is(USER_TVLCOM_CMD_NACK, 0x24U), "write invalid nack") &&
           expect(g_apply_count == 1U, "write invalid rollback") &&
           expect(g_applied_settings.set_voltage_mv == 12000U, "write invalid did not change settings");
}

static int test_read_length_error(void)
{
    user_tvlcom_context_t ctx;
    uint8_t req[64];
    uint8_t payload[8];
    uint8_t value[4];
    uint16_t payload_len = 0U;
    uint16_t req_len;

    reset_fixture();
    UserTvlCom_Init(&ctx, mock_send, NULL);
    write_le32(value, 0U);
    payload_len = append_tlv(payload, payload_len, USER_TVLCOM_DATA_INPUT_VOLTAGE, value, 4U);
    req_len = UserTvlCom_BuildFrame(USER_TVLCOM_CMD_READ, 0x25U, payload, payload_len, req, (uint16_t)sizeof(req));
    UserTvlCom_Feed(&ctx, req, req_len);

    return expect(frame_cmd_is(USER_TVLCOM_CMD_NACK, 0x25U), "read nonzero len nack");
}

static int test_build_frame_crc(void)
{
    uint8_t frame[32];
    uint16_t frame_len = UserTvlCom_BuildFrame(USER_TVLCOM_CMD_REPORT, 0x26U, NULL, 0U, frame, (uint16_t)sizeof(frame));
    uint16_t crc = UserTvlCom_Crc16Modbus(&frame[2], (uint16_t)(frame_len - 4U));

    return expect(frame_len == 8U, "build report frame len") &&
           expect(frame[0] == USER_TVLCOM_SOF0 && frame[1] == USER_TVLCOM_SOF1, "build sof") &&
           expect(read_le16(&frame[2]) == 2U, "build body len") &&
           expect(read_le32(&frame[4]) != 0U, "build body exists") &&
           expect(read_le16(&frame[6]) == crc, "build crc");
}

int main(void)
{
    int ok = 1;

    ok &= test_crc();
    ok &= test_build_frame_crc();
    ok &= test_report_split_feed();
    ok &= test_debug_snapshot();
    ok &= test_write_commit_and_rollback();
    ok &= test_read_length_error();

    if (ok)
    {
        printf("PASS\n");
        return 0;
    }

    return 1;
}
