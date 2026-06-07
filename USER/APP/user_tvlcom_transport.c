/**
 * @file user_tvlcom_transport.c
 * @brief F4CP TVLCOM transport fan-in.
 */

#include "user_tvlcom_transport.h"

#include "usart.h"
#include "usbd_cdc_if.h"
#include "user_tvlcom_protocol.h"

static USER_tvlcomContext_t s_USER_tvlcomContexts[USER_TVLCOM_TRANSPORT_COUNT];

static int s_USER_tvlcomTransportSend(const uint8_t *data, uint16_t len, void *user)
{
    USER_tvlcomTransport_t transport = (USER_tvlcomTransport_t)(uintptr_t)user;

    if ((data == NULL) || (len == 0U))
    {
        return -1;
    }

    switch (transport)
    {
        case USER_TVLCOM_TRANSPORT_USB_CDC:
            return (CDC_Transmit_FS((uint8_t *)data, len) == USBD_OK) ? 0 : -1;

        case USER_TVLCOM_TRANSPORT_USART1:
            return (HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 10U) == HAL_OK) ? 0 : -1;

        case USER_TVLCOM_TRANSPORT_USART2:
            return (HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 10U) == HAL_OK) ? 0 : -1;

        default:
            return -1;
    }
}

void USER_tvlcomTransportInit(void)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)USER_TVLCOM_TRANSPORT_COUNT; ++i)
    {
        USER_tvlcomInit(&s_USER_tvlcomContexts[i],
                        s_USER_tvlcomTransportSend,
                        (void *)(uintptr_t)i);
    }
}

void USER_tvlcomTransportRunTask(void)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)USER_TVLCOM_TRANSPORT_COUNT; ++i)
    {
        USER_tvlcomRunTask(&s_USER_tvlcomContexts[i]);
    }
}

void USER_tvlcomTransportOnRx(USER_tvlcomTransport_t transport, const uint8_t *data, uint16_t len)
{
    if ((transport >= USER_TVLCOM_TRANSPORT_COUNT) || (data == NULL) || (len == 0U))
    {
        return;
    }

    USER_tvlcomFeed(&s_USER_tvlcomContexts[transport], data, len);
}

void USER_tvlcomTransportOnUsbCdcRx(const uint8_t *data, uint16_t len)
{
    USER_tvlcomTransportOnRx(USER_TVLCOM_TRANSPORT_USB_CDC, data, len);
}

void USER_tvlcomTransportOnUartRx(USER_tvlcomTransport_t transport, const uint8_t *data, uint16_t len)
{
    if ((transport == USER_TVLCOM_TRANSPORT_USART1) || (transport == USER_TVLCOM_TRANSPORT_USART2))
    {
        USER_tvlcomTransportOnRx(transport, data, len);
    }
}
