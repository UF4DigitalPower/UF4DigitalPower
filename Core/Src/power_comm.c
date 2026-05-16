#include "power_comm.h"

#include <stdio.h>
#include <string.h>

#define F4CP_SOF0 0xAAU
#define F4CP_SOF1 0x55U

#define F4CP_CMD_ACK    0x00U
#define F4CP_CMD_WRITE  0x02U
#define F4CP_CMD_REPORT 0x03U

#define F4CP_TYPE_INPUT_VOLTAGE       10U
#define F4CP_TYPE_INPUT_CURRENT       11U
#define F4CP_TYPE_OUTPUT_VOLTAGE      12U
#define F4CP_TYPE_OUTPUT_CURRENT      13U
#define F4CP_TYPE_CORE_TEMPERATURE    14U
#define F4CP_TYPE_SET_VOLTAGE_LIMIT   17U
#define F4CP_TYPE_SET_CURRENT_LIMIT   18U
#define F4CP_TYPE_CC_CV_MODE          20U
#define F4CP_TYPE_POWER_STATE         21U
#define F4CP_TYPE_FAULT_STATE         22U
#define F4CP_TYPE_FAN_SPEED           38U

#define F4CP_MAX_PAYLOAD 256U
#define F4CP_MAX_FRAME   (4U + 2U + F4CP_MAX_PAYLOAD + 2U)
#define F4CP_REPORT_PERIOD_MS 300U
#define F4CP_RX_TIMEOUT_MS 18U

static UART_HandleTypeDef *g_power_uart;
static uint8_t g_seq;
static uint32_t g_last_report_tick;

static uint16_t f4cp_crc16(const uint8_t *data, uint16_t len) {
	uint16_t crc = 0xFFFFU;
	uint16_t i;
	uint8_t bit;

	for (i = 0U; i < len; ++i) {
		crc ^= data[i];
		for (bit = 0U; bit < 8U; ++bit) {
			if ((crc & 0x0001U) != 0U) {
				crc = (uint16_t) ((crc >> 1U) ^ 0xA001U);
			} else {
				crc >>= 1U;
			}
		}
	}

	return crc;
}

static void put_u16_le(uint8_t *dst, uint16_t value) {
	dst[0] = (uint8_t) value;
	dst[1] = (uint8_t) (value >> 8U);
}

static void put_i32_le(uint8_t *dst, int32_t value) {
	dst[0] = (uint8_t) value;
	dst[1] = (uint8_t) ((uint32_t) value >> 8U);
	dst[2] = (uint8_t) ((uint32_t) value >> 16U);
	dst[3] = (uint8_t) ((uint32_t) value >> 24U);
}

static uint16_t get_u16_le(const uint8_t *src) {
	return (uint16_t) src[0] | (uint16_t) ((uint16_t) src[1] << 8U);
}

static int32_t get_i32_le(const uint8_t *src) {
	return (int32_t) ((uint32_t) src[0] |
					  ((uint32_t) src[1] << 8U) |
					  ((uint32_t) src[2] << 16U) |
					  ((uint32_t) src[3] << 24U));
}

static uint16_t f4cp_build_frame(uint8_t cmd, const uint8_t *payload, uint16_t payload_len,
								 uint8_t *frame, uint16_t frame_size) {
	const uint16_t body_len = (uint16_t) (2U + payload_len);
	uint16_t crc;

	if (frame_size < (uint16_t) (body_len + 6U)) {
		return 0U;
	}

	frame[0] = F4CP_SOF0;
	frame[1] = F4CP_SOF1;
	put_u16_le(&frame[2], body_len);
	frame[4] = cmd;
	frame[5] = g_seq++;
	if (payload_len > 0U && payload != NULL) {
		(void) memcpy(&frame[6], payload, payload_len);
	}

	crc = f4cp_crc16(frame, (uint16_t) (body_len + 4U));
	put_u16_le(&frame[body_len + 4U], crc);
	return (uint16_t) (body_len + 6U);
}

static bool f4cp_transmit(uint8_t cmd, const uint8_t *payload, uint16_t payload_len) {
	uint8_t frame[F4CP_MAX_FRAME];
	uint16_t len = f4cp_build_frame(cmd, payload, payload_len, frame, sizeof(frame));

	if (g_power_uart == NULL || len == 0U) {
		return false;
	}

	return HAL_UART_Transmit(g_power_uart, frame, len, 20U) == HAL_OK;
}

static bool f4cp_receive_frame(uint8_t *cmd, uint8_t *payload, uint16_t *payload_len, uint32_t timeout_ms) {
	uint8_t byte;
	uint8_t header[4];
	uint8_t body[F4CP_MAX_PAYLOAD + 2U];
	uint16_t body_len;
	uint16_t crc_rx;
	uint16_t crc_calc;
	uint16_t index;
	uint32_t start = HAL_GetTick();

	if (g_power_uart == NULL || cmd == NULL || payload == NULL || payload_len == NULL) {
		return false;
	}

	do {
		if (HAL_UART_Receive(g_power_uart, &byte, 1U, 1U) != HAL_OK) {
			continue;
		}
		if (byte != F4CP_SOF0) {
			continue;
		}
		if (HAL_UART_Receive(g_power_uart, &byte, 1U, 2U) != HAL_OK || byte != F4CP_SOF1) {
			continue;
		}
		header[0] = F4CP_SOF0;
		header[1] = F4CP_SOF1;
		if (HAL_UART_Receive(g_power_uart, &header[2], 2U, 4U) != HAL_OK) {
			continue;
		}
		body_len = get_u16_le(&header[2]);
		if (body_len < 2U || body_len > (uint16_t) (F4CP_MAX_PAYLOAD + 2U)) {
			continue;
		}
		if (HAL_UART_Receive(g_power_uart, body, (uint16_t) (body_len + 2U), timeout_ms) != HAL_OK) {
			continue;
		}

		crc_rx = get_u16_le(&body[body_len]);
		crc_calc = f4cp_crc16(header, 4U);
		for (index = 0U; index < body_len; ++index) {
			uint8_t tmp = body[index];
			uint8_t bit;
			crc_calc ^= tmp;
			for (bit = 0U; bit < 8U; ++bit) {
				if ((crc_calc & 0x0001U) != 0U) {
					crc_calc = (uint16_t) ((crc_calc >> 1U) ^ 0xA001U);
				} else {
					crc_calc >>= 1U;
				}
			}
		}
		if (crc_calc != crc_rx) {
			continue;
		}

		*cmd = body[0];
		*payload_len = (uint16_t) (body_len - 2U);
		if (*payload_len > 0U) {
			(void) memcpy(payload, &body[2], *payload_len);
		}
		return true;
	} while ((HAL_GetTick() - start) < timeout_ms);

	return false;
}

static void f4cp_parse_tlvs(const uint8_t *payload, uint16_t payload_len, ui_power_snapshot_t *snapshot) {
	uint16_t offset = 0U;

	while ((uint16_t) (offset + 3U) <= payload_len) {
		const uint8_t type = payload[offset];
		const uint16_t len = get_u16_le(&payload[offset + 1U]);
		const uint8_t *value = &payload[offset + 3U];

		offset = (uint16_t) (offset + 3U);
		if ((uint16_t) (offset + len) > payload_len) {
			break;
		}

		if (len == 4U) {
			const int32_t v = get_i32_le(value);
			switch (type) {
				case F4CP_TYPE_INPUT_VOLTAGE: snapshot->vin_mv = v; break;
				case F4CP_TYPE_INPUT_CURRENT: snapshot->iin_ma = v; break;
				case F4CP_TYPE_OUTPUT_VOLTAGE: snapshot->vout_mv = v; break;
				case F4CP_TYPE_OUTPUT_CURRENT: snapshot->iout_ma = v; break;
				case F4CP_TYPE_CORE_TEMPERATURE: snapshot->temp_dC = v / 100; break;
				case F4CP_TYPE_SET_VOLTAGE_LIMIT: snapshot->vset_mv = v; break;
				case F4CP_TYPE_SET_CURRENT_LIMIT: snapshot->iset_ma = v; break;
				case F4CP_TYPE_FAULT_STATE: snapshot->fault_code = (uint16_t) v; break;
				case F4CP_TYPE_FAN_SPEED: snapshot->fan_permille = (uint16_t) v; break;
				default: break;
			}
		} else if (len == 1U) {
			switch (type) {
				case F4CP_TYPE_CC_CV_MODE: snapshot->cc_mode = (value[0] == 0U); break;
				case F4CP_TYPE_POWER_STATE: snapshot->output_enabled = (value[0] != 0U); break;
				default: break;
			}
		}

		offset = (uint16_t) (offset + len);
	}

	(void) snprintf(snapshot->status_text, sizeof(snapshot->status_text), "ONLINE");
}

void PowerComm_Init(UART_HandleTypeDef *uart) {
	g_power_uart = uart;
	g_seq = 0U;
	g_last_report_tick = 0U;
}

bool PowerComm_Tick(ui_power_snapshot_t *snapshot) {
	uint8_t cmd;
	uint8_t payload[F4CP_MAX_PAYLOAD];
	uint16_t payload_len;
	const uint32_t now = HAL_GetTick();

	if (snapshot == NULL || g_power_uart == NULL) {
		return false;
	}
	if ((now - g_last_report_tick) < F4CP_REPORT_PERIOD_MS) {
		return false;
	}
	g_last_report_tick = now;

	if (!f4cp_transmit(F4CP_CMD_REPORT, NULL, 0U)) {
		(void) snprintf(snapshot->status_text, sizeof(snapshot->status_text), "TX ERROR");
		return true;
	}

	if (!f4cp_receive_frame(&cmd, payload, &payload_len, F4CP_RX_TIMEOUT_MS)) {
		(void) snprintf(snapshot->status_text, sizeof(snapshot->status_text), "NO LINK");
		return true;
	}

	if (cmd == F4CP_CMD_REPORT || cmd == F4CP_CMD_ACK) {
		f4cp_parse_tlvs(payload, payload_len, snapshot);
		return true;
	}

	(void) snprintf(snapshot->status_text, sizeof(snapshot->status_text), "BAD CMD");
	return true;
}

void PowerComm_WriteSettings(int32_t vset_mv, int32_t iset_ma, bool output_enabled) {
	uint8_t payload[3U + 4U + 3U + 4U + 3U + 1U];
	uint16_t offset = 0U;

	payload[offset++] = F4CP_TYPE_SET_VOLTAGE_LIMIT;
	put_u16_le(&payload[offset], 4U);
	offset = (uint16_t) (offset + 2U);
	put_i32_le(&payload[offset], vset_mv);
	offset = (uint16_t) (offset + 4U);

	payload[offset++] = F4CP_TYPE_SET_CURRENT_LIMIT;
	put_u16_le(&payload[offset], 4U);
	offset = (uint16_t) (offset + 2U);
	put_i32_le(&payload[offset], iset_ma);
	offset = (uint16_t) (offset + 4U);

	payload[offset++] = F4CP_TYPE_POWER_STATE;
	put_u16_le(&payload[offset], 1U);
	offset = (uint16_t) (offset + 2U);
	payload[offset++] = output_enabled ? 1U : 0U;

	(void) f4cp_transmit(F4CP_CMD_WRITE, payload, offset);
}
