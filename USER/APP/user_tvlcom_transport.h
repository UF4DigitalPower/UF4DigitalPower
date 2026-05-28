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
} USER_tvlcomTransport_t;

void USER_tvlcomTransportInit(void);
void USER_tvlcomTransportOnRx(USER_tvlcomTransport_t transport, const uint8_t *data, uint16_t len);
void USER_tvlcomTransportOnUsbCdcRx(const uint8_t *data, uint16_t len);
void USER_tvlcomTransportOnUartRx(USER_tvlcomTransport_t transport, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_USER_TVLCOM_TRANSPORT_H */
