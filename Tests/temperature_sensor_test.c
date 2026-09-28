#include "temperature_sensor.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(TemperatureSensor_FromAdc(0, true) == TEMPERATURE_SENSOR_INVALID);
    assert(TemperatureSensor_FromAdc(4095, false) == TEMPERATURE_SENSOR_INVALID);
    assert(TemperatureSensor_FromAdc(3150, true) == 2500u);
    assert(TemperatureSensor_FromAdc(945, false) == 2500u);
    assert(TemperatureSensor_FromAdc(972, true) == 9000u);
    assert(TemperatureSensor_FromAdc(1020, true) > 8500u);
    assert(TemperatureSensor_FromAdc(1020, true) < 9000u);
    puts("temperature_sensor_test: OK");
    return 0;
}
