/**
 * @file user_tvlcom_protocol.h
 * @brief F4CP TVLCOM command protocol.
 */

#ifndef UF4DIGITALPOWER_USER_TVLCOM_PROTOCOL_H
#define UF4DIGITALPOWER_USER_TVLCOM_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define USER_TVLCOM_SOF0 0xAAU
#define USER_TVLCOM_SOF1 0x55U
#define USER_TVLCOM_MAX_PAYLOAD_SIZE 384U
#define USER_TVLCOM_MAX_FRAME_SIZE (2U + 2U + USER_TVLCOM_MAX_PAYLOAD_SIZE + 2U)

typedef enum
{
    USER_TVLCOM_CMD_ACK = 0x00U,
    USER_TVLCOM_CMD_READ = 0x01U,
    USER_TVLCOM_CMD_WRITE = 0x02U,
    USER_TVLCOM_CMD_REPORT = 0x03U,
    USER_TVLCOM_CMD_NACK = 0xFFU,
} user_tvlcom_cmd_t;

typedef enum
{
    USER_TVLCOM_ACCESS_READ = 0x01U,
    USER_TVLCOM_ACCESS_WRITE = 0x02U,
    USER_TVLCOM_ACCESS_READ_WRITE = 0x03U,
} user_tvlcom_access_t;

typedef enum
{
    USER_TVLCOM_DATA_INPUT_VOLTAGE = 10U,
    USER_TVLCOM_DATA_INPUT_CURRENT = 11U,
    USER_TVLCOM_DATA_OUTPUT_VOLTAGE = 12U,
    USER_TVLCOM_DATA_OUTPUT_CURRENT = 13U,
    USER_TVLCOM_DATA_CORE_TEMPERATURE = 14U,
    USER_TVLCOM_DATA_BOARD_TEMPERATURE = 15U,
    USER_TVLCOM_DATA_SET_VOLTAGE_LIMIT = 17U,
    USER_TVLCOM_DATA_SET_CURRENT_LIMIT = 18U,
    USER_TVLCOM_DATA_CC_CV_MODE = 20U,
    USER_TVLCOM_DATA_POWER_STATE = 21U,
    USER_TVLCOM_DATA_FAULT_STATE = 22U,
    USER_TVLCOM_DATA_STATE_MACHINE_FLAG_BITS = 23U,
    USER_TVLCOM_DATA_STATE_MACHINE_STATE = 24U,
    USER_TVLCOM_DATA_INPUT_VOLTAGE_RAW = 25U,
    USER_TVLCOM_DATA_INPUT_CURRENT_RAW = 26U,
    USER_TVLCOM_DATA_OUTPUT_VOLTAGE_RAW = 27U,
    USER_TVLCOM_DATA_OUTPUT_CURRENT_RAW = 28U,
    USER_TVLCOM_DATA_OTP_VALUE = 29U,
    USER_TVLCOM_DATA_OTP_SET_VALUE = 30U,
    USER_TVLCOM_DATA_OVP_VALUE = 31U,
    USER_TVLCOM_DATA_OVP_SET_VALUE = 32U,
    USER_TVLCOM_DATA_OCP_VALUE = 33U,
    USER_TVLCOM_DATA_OCP_SET_VALUE = 34U,
    USER_TVLCOM_DATA_DUTY_CMD = 35U,
    USER_TVLCOM_DATA_PWM_A_COMPARE = 36U,
    USER_TVLCOM_DATA_PWM_D_COMPARE = 37U,
    USER_TVLCOM_DATA_FAN_SPEED = 38U,
    USER_TVLCOM_DATA_FAN_SET_VALUE = 39U,
    USER_TVLCOM_DATA_DEBUG_SNAPSHOT = 40U,
    USER_TVLCOM_DATA_LOOP_CURRENT_FEEDBACK = 41U,
    USER_TVLCOM_DATA_LOOP_CURRENT_REFERENCE = 42U,
    USER_TVLCOM_DATA_VOLTAGE_LOOP_CURRENT_REFERENCE = 43U,
} user_tvlcom_data_type_t;

typedef enum
{
    USER_TVLCOM_STATUS_OK = 0,
    USER_TVLCOM_STATUS_ERROR = -1,
} user_tvlcom_status_t;

typedef int (*user_tvlcom_send_fn_t)(const uint8_t *data, uint16_t len, void *user);

typedef struct
{
    uint8_t rx_buf[USER_TVLCOM_MAX_FRAME_SIZE];
    uint16_t rx_len;
    uint16_t expected_len;
    uint8_t sof_state;
    user_tvlcom_send_fn_t send;
    void *send_user;
} user_tvlcom_context_t;

void UserTvlCom_Init(user_tvlcom_context_t *ctx, user_tvlcom_send_fn_t send, void *send_user);
void UserTvlCom_Feed(user_tvlcom_context_t *ctx, const uint8_t *data, uint16_t len);
uint16_t UserTvlCom_Crc16Modbus(const uint8_t *data, uint16_t len);
uint16_t UserTvlCom_BuildFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t payload_len, uint8_t *out, uint16_t out_cap);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_USER_TVLCOM_PROTOCOL_H */
