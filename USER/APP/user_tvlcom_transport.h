/**
 * @file user_tvlcom_transport.h
 * @brief Physical transport fan-in for F4CP TVLCOM.
 */

#ifndef UF4DIGITALPOWER_USER_TVLCOM_TRANSPORT_H
#define UF4DIGITALPOWER_USER_TVLCOM_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    USER_TVLCOM_TRANSPORT_USB_CDC = 0U,
    USER_TVLCOM_TRANSPORT_USART1 = 1U,
    USER_TVLCOM_TRANSPORT_USART2 = 2U,
    USER_TVLCOM_TRANSPORT_COUNT,
} user_tvlcom_transport_t;

void UserTvlComTransport_Init(void);
void UserTvlComTransport_OnRx(user_tvlcom_transport_t transport, const uint8_t *data, uint16_t len);
void UserTvlComTransport_OnUsbCdcRx(const uint8_t *data, uint16_t len);
void UserTvlComTransport_OnUartRx(user_tvlcom_transport_t transport, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_USER_TVLCOM_TRANSPORT_H */
