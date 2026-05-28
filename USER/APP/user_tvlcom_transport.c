/**
 * @file user_tvlcom_transport.c
 * @brief F4CP TVLCOM transport fan-in.
 */

#include "user_tvlcom_transport.h"

#include "usart.h"
#include "usbd_cdc_if.h"
#include "user_tvlcom_protocol.h"

static user_tvlcom_context_t g_user_tvlcom_contexts[USER_TVLCOM_TRANSPORT_COUNT];

static int user_tvlcom_transport_send(const uint8_t *data, uint16_t len, void *user)
{
    user_tvlcom_transport_t transport = (user_tvlcom_transport_t)(uintptr_t)user;

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

void UserTvlComTransport_Init(void)
{
    uint32_t i;

    for (i = 0U; i < (uint32_t)USER_TVLCOM_TRANSPORT_COUNT; ++i)
    {
        UserTvlCom_Init(&g_user_tvlcom_contexts[i], user_tvlcom_transport_send, (void *)(uintptr_t)i);
    }
}

void UserTvlComTransport_OnRx(user_tvlcom_transport_t transport, const uint8_t *data, uint16_t len)
{
    if ((transport >= USER_TVLCOM_TRANSPORT_COUNT) || (data == NULL) || (len == 0U))
    {
        return;
    }

    UserTvlCom_Feed(&g_user_tvlcom_contexts[transport], data, len);
}

void UserTvlComTransport_OnUsbCdcRx(const uint8_t *data, uint16_t len)
{
    UserTvlComTransport_OnRx(USER_TVLCOM_TRANSPORT_USB_CDC, data, len);
}

void UserTvlComTransport_OnUartRx(user_tvlcom_transport_t transport, const uint8_t *data, uint16_t len)
{
    if ((transport == USER_TVLCOM_TRANSPORT_USART1) || (transport == USER_TVLCOM_TRANSPORT_USART2))
    {
        UserTvlComTransport_OnRx(transport, data, len);
    }
}
