#include "temperature_sensor.h"

/* Nominal ADC codes at 0..150 C in 5 C steps with the NTC to ground. */
static const uint16_t s_ntc_codes[] = {
    3760, 3670, 3565, 3443, 3304, 3150, 2982, 2803,
    2615, 2423, 2230, 2040, 1856, 1680, 1514, 1360,
    1218, 1089, 972, 867, 773, 689, 614, 548,
    489, 437, 392, 351, 315, 284, 256
};

uint16_t TemperatureSensor_FromAdc(uint16_t raw, bool ntc_to_ground)
{
    uint16_t code;
    unsigned i;
    if (raw < 8u || raw > 4087u) return TEMPERATURE_SENSOR_INVALID;
    code = ntc_to_ground ? raw : (uint16_t)(4095u - raw);
    if (code >= s_ntc_codes[0]) return 0u;
    if (code <= s_ntc_codes[30]) return 15000u;
    for (i = 0u; i < 30u; ++i)
    {
        if (code >= s_ntc_codes[i + 1u])
            return (uint16_t)(i * 500u +
                (uint32_t)(s_ntc_codes[i] - code) * 500u /
                (s_ntc_codes[i] - s_ntc_codes[i + 1u]));
    }
    return TEMPERATURE_SENSOR_INVALID;
}
