/**
 * @file test_user_tvlcom_protocol.c
 * @brief Host-side tests for the F4CP TVLCOM wire protocol.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "power_ctrl.h"
#include "user_tvlcom_protocol.h"

static POWER_ctrlSnapshot_t s_USER_snapshot;
static POWER_ctrlSettings_t s_USER_appliedSettings;
static uint8_t s_USER_enabled;
static uint32_t s_USER_applyCount;
static uint8_t s_USER_tx[USER_TVLCOM_MAX_FRAME_SIZE];
static uint16_t s_USER_txLen;

static void s_USER_writeLe16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
}

static void s_USER_writeLe32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
    data[2] = (uint8_t)((value >> 16) & 0xFFU);
    data[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint16_t s_USER_readLe16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t s_USER_readLe32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

void POWER_initAppCtrl(void)
{
}

void POWER_getAppSnapshot(POWER_ctrlSnapshot_t *snapshot)
{
    *snapshot = s_USER_snapshot;
}

void POWER_applyAppSettings(const POWER_ctrlSettings_t *settings)
{
    s_USER_appliedSettings = *settings;
    s_USER_snapshot.settings = *settings;
    ++s_USER_applyCount;
}

void POWER_setAppEnabled(uint8_t enabled)
{
    s_USER_enabled = enabled;
    s_USER_snapshot.power_enabled = enabled;
}

void USER_flashStoreRequestAppSave(void)
{
}

static int s_USER_mockSend(const uint8_t *data, uint16_t len, void *user)
{
    (void)user;
    memcpy(s_USER_tx, data, len);
    s_USER_txLen = len;
    return 0;
}

static void s_USER_resetFixture(void)
{
    memset(&s_USER_snapshot, 0, sizeof(s_USER_snapshot));
    memset(&s_USER_appliedSettings, 0, sizeof(s_USER_appliedSettings));
    memset(s_USER_tx, 0, sizeof(s_USER_tx));
    s_USER_txLen = 0U;
    s_USER_enabled = 0U;
    s_USER_applyCount = 0U;

    s_USER_snapshot.input_voltage_mv = 24000U;
    s_USER_snapshot.input_current_ma = 1200U;
    s_USER_snapshot.output_voltage_mv = 5000U;
    s_USER_snapshot.output_current_ma = 900U;
    s_USER_snapshot.core_temperature_mc = 42000;
    s_USER_snapshot.board_temperature_mc = 35000;
    s_USER_snapshot.temp2_temperature_mc = 36000;
    s_USER_snapshot.input_voltage_raw = 1001U;
    s_USER_snapshot.input_current_raw = 1002U;
    s_USER_snapshot.output_voltage_raw = 1003U;
    s_USER_snapshot.output_current_raw = 1004U;
    s_USER_snapshot.settings.set_voltage_mv = 5000U;
    s_USER_snapshot.settings.set_current_ma = 10000U;
    s_USER_snapshot.settings.otp_set_mc = 80000;
    s_USER_snapshot.settings.ovp_set_mv = 50000U;
    s_USER_snapshot.settings.ocp_set_ma = 10500U;
    s_USER_snapshot.settings.fan_set_permille = 250U;
    s_USER_snapshot.power_enabled = 0U;
    s_USER_snapshot.state_flag_bits = POWER_CTRL_STATE_FLAG_WAIT;
    s_USER_snapshot.stage_mode = POWER_CTRL_STAGE_BUCK;
    s_USER_snapshot.cvcc_mode = POWER_CTRL_CVCC_CV;
    s_USER_snapshot.otp_value_mc = 78000;
    s_USER_snapshot.ovp_value_mv = 50000U;
    s_USER_snapshot.ocp_value_ma = 10500U;
    s_USER_snapshot.duty_cmd = 123U;
    s_USER_snapshot.pwm_a_compare = 456U;
    s_USER_snapshot.pwm_d_compare = 789U;
    s_USER_snapshot.fan_speed_permille = 250U;
    s_USER_snapshot.loop_current_feedback_ma = 900U;
    s_USER_snapshot.loop_current_reference_ma = 10000U;
    s_USER_snapshot.voltage_loop_reference_mv = 5000U;
}

static uint16_t s_USER_appendTlv(uint8_t *payload, uint16_t offset, uint8_t type, const uint8_t *value, uint16_t len)
{
    payload[offset++] = type;
    s_USER_writeLe16(&payload[offset], len);
    offset = (uint16_t)(offset + 2U);
    if (len > 0U)
    {
        memcpy(&payload[offset], value, len);
        offset = (uint16_t)(offset + len);
    }
    return offset;
}

static int s_USER_expect(int condition, const char *name)
{
    if (!condition)
    {
        printf("FAIL: %s\n", name);
        return 0;
    }
    return 1;
}

static int s_USER_frameCmdIs(uint8_t cmd, uint8_t seq)
{
    return (s_USER_txLen >= 6U) &&
           (s_USER_tx[0] == USER_TVLCOM_SOF0) &&
           (s_USER_tx[1] == USER_TVLCOM_SOF1) &&
           (s_USER_tx[4] == cmd) &&
           (s_USER_tx[5] == seq);
}

static int s_USER_testCrc(void)
{
    const uint8_t value[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    return s_USER_expect(USER_tvlcomCrc16Modbus(value, (uint16_t)sizeof(value)) == 0x4B37U, "crc modbus vector");
}

static int s_USER_testReportSplitFeed(void)
{
    USER_tvlcomContext_t ctx;
    uint8_t req[32];
    uint16_t req_len;
    uint16_t body_len;
    uint16_t tlv_offset;

    s_USER_resetFixture();
    USER_tvlcomInit(&ctx, s_USER_mockSend, NULL);
    req_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_REPORT, 0x21U, NULL, 0U, req, (uint16_t)sizeof(req));
    USER_tvlcomFeed(&ctx, req, 3U);
    USER_tvlcomFeed(&ctx, &req[3], (uint16_t)(req_len - 3U));

    body_len = s_USER_readLe16(&s_USER_tx[2]);
    tlv_offset = 6U;
    return s_USER_expect(s_USER_frameCmdIs(USER_TVLCOM_CMD_REPORT, 0x21U), "report response cmd/seq") &&
           s_USER_expect(body_len > 2U, "report nonempty payload") &&
           s_USER_expect(s_USER_tx[tlv_offset] == USER_TVLCOM_DATA_INPUT_VOLTAGE, "report first tlv type");
}

static int s_USER_testDebugSnapshot(void)
{
    USER_tvlcomContext_t ctx;
    uint8_t req[64];
    uint8_t payload[8];
    uint16_t payload_len = 0U;
    uint16_t req_len;
    uint16_t offset;

    s_USER_resetFixture();
    USER_tvlcomInit(&ctx, s_USER_mockSend, NULL);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_DEBUG_SNAPSHOT, NULL, 0U);
    req_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_READ, 0x22U, payload, payload_len, req, (uint16_t)sizeof(req));
    USER_tvlcomFeed(&ctx, req, req_len);

    offset = 6U;
    return s_USER_expect(s_USER_frameCmdIs(USER_TVLCOM_CMD_ACK, 0x22U), "debug ack") &&
           s_USER_expect(s_USER_tx[offset] == USER_TVLCOM_DATA_OUTPUT_VOLTAGE_RAW, "debug first tlv") &&
           s_USER_expect(s_USER_tx[(uint16_t)(offset + 7U)] == USER_TVLCOM_DATA_OUTPUT_VOLTAGE, "debug second tlv");
}

static int s_USER_testReadTemp2(void)
{
    USER_tvlcomContext_t ctx;
    uint8_t req[64];
    uint8_t payload[8];
    uint16_t payload_len = 0U;
    uint16_t req_len;
    uint16_t offset;

    s_USER_resetFixture();
    USER_tvlcomInit(&ctx, s_USER_mockSend, NULL);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_TEMP2_TEMPERATURE, NULL, 0U);
    req_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_READ, 0x27U, payload, payload_len, req, (uint16_t)sizeof(req));
    USER_tvlcomFeed(&ctx, req, req_len);

    offset = 6U;
    return s_USER_expect(s_USER_frameCmdIs(USER_TVLCOM_CMD_ACK, 0x27U), "temp2 read ack") &&
           s_USER_expect(s_USER_tx[offset] == USER_TVLCOM_DATA_TEMP2_TEMPERATURE, "temp2 tlv type") &&
           s_USER_expect(s_USER_readLe16(&s_USER_tx[(uint16_t)(offset + 1U)]) == 4U, "temp2 tlv len") &&
           s_USER_expect((int32_t)s_USER_readLe32(&s_USER_tx[(uint16_t)(offset + 3U)]) == s_USER_snapshot.temp2_temperature_mc, "temp2 tlv value");
}

static int s_USER_testWriteCommitAndRollback(void)
{
    USER_tvlcomContext_t ctx;
    uint8_t req[96];
    uint8_t payload[64];
    uint8_t value[4];
    uint16_t payload_len = 0U;
    uint16_t req_len;

    s_USER_resetFixture();
    USER_tvlcomInit(&ctx, s_USER_mockSend, NULL);

    s_USER_writeLe32(value, 12000U);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT, value, 4U);
    s_USER_writeLe32(value, 3000U);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_SET_CURRENT_LIMIT, value, 4U);
    value[0] = 1U;
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_POWER_STATE, value, 1U);
    req_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_WRITE, 0x23U, payload, payload_len, req, (uint16_t)sizeof(req));
    USER_tvlcomFeed(&ctx, req, req_len);

    if (!s_USER_expect(s_USER_frameCmdIs(USER_TVLCOM_CMD_ACK, 0x23U), "write ack") ||
        !s_USER_expect(s_USER_appliedSettings.set_voltage_mv == 12000U, "write voltage committed") ||
        !s_USER_expect(s_USER_appliedSettings.set_current_ma == 3000U, "write current committed") ||
        !s_USER_expect(s_USER_enabled == 1U, "write enabled committed"))
    {
        return 0;
    }

    payload_len = 0U;
    s_USER_writeLe32(value, 13000U);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT, value, 4U);
    s_USER_writeLe32(value, 1U);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_OUTPUT_VOLTAGE, value, 4U);
    req_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_WRITE, 0x24U, payload, payload_len, req, (uint16_t)sizeof(req));
    USER_tvlcomFeed(&ctx, req, req_len);

    return s_USER_expect(s_USER_frameCmdIs(USER_TVLCOM_CMD_NACK, 0x24U), "write invalid nack") &&
           s_USER_expect(s_USER_applyCount == 1U, "write invalid rollback") &&
           s_USER_expect(s_USER_appliedSettings.set_voltage_mv == 12000U, "write invalid did not change settings");
}

static int s_USER_testReadLengthError(void)
{
    USER_tvlcomContext_t ctx;
    uint8_t req[64];
    uint8_t payload[8];
    uint8_t value[4];
    uint16_t payload_len = 0U;
    uint16_t req_len;

    s_USER_resetFixture();
    USER_tvlcomInit(&ctx, s_USER_mockSend, NULL);
    s_USER_writeLe32(value, 0U);
    payload_len = s_USER_appendTlv(payload, payload_len, USER_TVLCOM_DATA_INPUT_VOLTAGE, value, 4U);
    req_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_READ, 0x25U, payload, payload_len, req, (uint16_t)sizeof(req));
    USER_tvlcomFeed(&ctx, req, req_len);

    return s_USER_expect(s_USER_frameCmdIs(USER_TVLCOM_CMD_NACK, 0x25U), "read nonzero len nack");
}

static int s_USER_testBuildFrameCrc(void)
{
    uint8_t frame[32];
    uint16_t frame_len = USER_tvlcomBuildFrame(USER_TVLCOM_CMD_REPORT, 0x26U, NULL, 0U, frame, (uint16_t)sizeof(frame));
    uint16_t crc = USER_tvlcomCrc16Modbus(frame, (uint16_t)(frame_len - 2U));

    return s_USER_expect(frame_len == 8U, "build report frame len") &&
           s_USER_expect(frame[0] == USER_TVLCOM_SOF0 && frame[1] == USER_TVLCOM_SOF1, "build sof") &&
           s_USER_expect(s_USER_readLe16(&frame[2]) == 2U, "build body len") &&
           s_USER_expect(s_USER_readLe32(&frame[4]) != 0U, "build body exists") &&
           s_USER_expect(s_USER_readLe16(&frame[6]) == crc, "build crc");
}

int main(void)
{
    int ok = 1;

    ok &= s_USER_testCrc();
    ok &= s_USER_testBuildFrameCrc();
    ok &= s_USER_testReportSplitFeed();
    ok &= s_USER_testDebugSnapshot();
    ok &= s_USER_testReadTemp2();
    ok &= s_USER_testWriteCommitAndRollback();
    ok &= s_USER_testReadLengthError();

    if (ok)
    {
        printf("PASS\n");
        return 0;
    }

    return 1;
}
