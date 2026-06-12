/**
  ******************************************************************************
  * @file    tvlcom.h
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
#ifndef TVLCOM_H
#define TVLCOM_H

#include "main.h"
#include <stdint.h>

#define TVLCOM_SOF0                     0xAAU
#define TVLCOM_SOF1                     0x55U
#define TVLCOM_CHANNEL_SEPARATOR0       0xFEU
#define TVLCOM_CHANNEL_SEPARATOR1       0xEDU

#define TVLCOM_MAX_PAYLOAD_SIZE         384U
#define TVLCOM_MAX_FRAME_SIZE           (TVLCOM_MAX_PAYLOAD_SIZE + 8U)
#define TVLCOM_UART_RX_DMA_SIZE         256U
#define TVLCOM_UART_TX_DMA_SIZE         TVLCOM_MAX_FRAME_SIZE
#define TVLCOM_STREAM_BUFFER_SIZE       96U

typedef enum
{
    TVLCOM_PORT_USART1 = 0,
    TVLCOM_PORT_USART2 = 1,
    TVLCOM_PORT_CDC    = 2
} TVLCOM_Port;

void TVLCOM_Init(void);
void TVLCOM_RunTask(void);
void TVLCOM_SelectPort(TVLCOM_Port port);
TVLCOM_Port TVLCOM_GetPort(void);
HAL_StatusTypeDef TVLCOM_SendBytes(const uint8_t *data, uint16_t len);
void TVLCOM_OnUsbCdcRx(const uint8_t *data, uint16_t len);
void USER_tvlcomTransportOnUsbCdcRx(uint8_t *data, uint16_t len);

#endif //TVLCOM_H
