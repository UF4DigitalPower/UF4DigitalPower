#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#define TEMPERATURE_SENSOR_INVALID UINT16_MAX

/* 10 kOhm NTC, B=3950, 3 kOhm divider. */
uint16_t TemperatureSensor_FromAdc(uint16_t raw, bool ntc_to_ground);

#endif
