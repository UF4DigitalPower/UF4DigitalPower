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

#define TVLCOM_MAX_FRAME_SIZE           265U
#define TVLCOM_UART_RX_DMA_SIZE         256U
#define TVLCOM_UART_TX_DMA_SIZE         TVLCOM_MAX_FRAME_SIZE

typedef enum
{
    TVLCOM_PORT_USART1 = 0,
    TVLCOM_PORT_USART2 = 1,
    TVLCOM_PORT_CDC    = 2
} TVLCOM_Port;

#ifndef TVLCOM_FIXED_PORT
#define TVLCOM_FIXED_PORT TVLCOM_PORT_CDC
#endif

void TVLCOM_Init(void);
void TVLCOM_RunTask(void);
void TVLCOM_SelectPort(TVLCOM_Port port);
TVLCOM_Port TVLCOM_GetPort(void);
HAL_StatusTypeDef TVLCOM_SendBytes(const uint8_t *data, uint16_t len);
void TVLCOM_OnUsbCdcRx(const uint8_t *data, uint16_t len);
void USER_tvlcomTransportOnUsbCdcRx(uint8_t *data, uint16_t len);
void TVLCOM_OnTxComplete(TVLCOM_Port port);

#endif //TVLCOM_H
