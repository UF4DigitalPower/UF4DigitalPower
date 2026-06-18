/**
  ******************************************************************************
  * @file    uf4_transport.h
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
#ifndef UF4_TRANSPORT_H
#define UF4_TRANSPORT_H

#include "main.h"
#include <stdint.h>

#define UF4_TRANSPORT_MAX_FRAME_SIZE           265U
#define UF4_TRANSPORT_UART_RX_DMA_SIZE         256U
#define UF4_TRANSPORT_UART_TX_DMA_SIZE         UF4_TRANSPORT_MAX_FRAME_SIZE

typedef enum
{
    UF4_TRANSPORT_PORT_USART1 = 0,
    UF4_TRANSPORT_PORT_USART2 = 1,
    UF4_TRANSPORT_PORT_CDC    = 2
} UF4Transport_Port;

#ifndef UF4_TRANSPORT_FIXED_PORT
#define UF4_TRANSPORT_FIXED_PORT UF4_TRANSPORT_PORT_USART1
#endif

void UF4Transport_Init(void);
void UF4Transport_RunTask(void);
void UF4Transport_StreamTick(void);  /* 流发送 tick：poll TX + flush pending + UF4_Process，可放定时器中断 */
void UF4Transport_SelectPort(UF4Transport_Port port);
UF4Transport_Port UF4Transport_GetPort(void);
HAL_StatusTypeDef UF4Transport_SendBytes(const uint8_t *data, uint16_t len);
void UF4Transport_OnUsbCdcRx(const uint8_t *data, uint16_t len);
void USER_uf4TransportOnUsbCdcRx(uint8_t *data, uint16_t len);
void UF4Transport_OnTxComplete(UF4Transport_Port port);

#endif //UF4_TRANSPORT_H
