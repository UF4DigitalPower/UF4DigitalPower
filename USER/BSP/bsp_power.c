/**
 * @file bsp_power.c
 * @brief Board-level power stage definitions and measurement helpers.
 */

#include "bsp_power.h"

#include <string.h>

CCMRAM volatile uint16_t g_power_adc1_regular_dma[BSP_POWER_ADC1_REGULAR_COUNT] = {0U};
volatile ADC_RESULT_t g_power_adc_result = {0U};

static float bsp_power_adc_raw_to_voltage(uint16_t raw, float raw_full_scale)
{
    if (raw_full_scale <= 0.0F)
    {
        return 0.0F;
    }

    return ((float)raw * BSP_POWER_ADC_VREF_V) / raw_full_scale;
}

static float bsp_power_current_from_raw(uint16_t raw, float polarity)
{
    const float sense_voltage = BSP_Power_Adc1RawToVoltage(raw);
    return ((sense_voltage - BSP_POWER_CURRENT_BIAS_V) / BSP_POWER_CURRENT_SENSE_V_PER_A) * polarity;
}

void BSP_Power_Init(void)
{
    memset((void *)g_power_adc1_regular_dma, 0, sizeof(g_power_adc1_regular_dma));
    memset((void *)&g_power_adc_result, 0, sizeof(g_power_adc_result));
}

void BSP_Power_RefreshAdcResultFromBuffers(void)
{
    g_power_adc_result.iin_raw = g_power_adc1_regular_dma[BSP_POWER_ADC1_REGULAR_IIN];
    g_power_adc_result.iout_raw = g_power_adc1_regular_dma[BSP_POWER_ADC1_REGULAR_IOUT];
}

void BSP_Power_SetInjectedRaw(uint16_t vout_raw, uint16_t vin_raw)
{
    g_power_adc_result.vout_raw = vout_raw;
    g_power_adc_result.vin_raw = vin_raw;
}

void BSP_Power_SetAuxTemperatureRaw(uint16_t temp1_raw, uint16_t temp2_raw, uint16_t die_temp_raw)
{
    g_power_adc_result.temp1_raw = temp1_raw;
    g_power_adc_result.temp2_raw = temp2_raw;
    g_power_adc_result.die_temp_raw = die_temp_raw;
}

void BSP_Power_GetAdcResult(ADC_RESULT_t *result)
{
    if (result == NULL)
    {
        return;
    }

    BSP_Power_RefreshAdcResultFromBuffers();

    result->iin_raw = g_power_adc_result.iin_raw;
    result->iout_raw = g_power_adc_result.iout_raw;
    result->vout_raw = g_power_adc_result.vout_raw;
    result->vin_raw = g_power_adc_result.vin_raw;
    result->temp1_raw = g_power_adc_result.temp1_raw;
    result->temp2_raw = g_power_adc_result.temp2_raw;
    result->die_temp_raw = g_power_adc_result.die_temp_raw;
}

void BSP_Power_ConvertToMeasurement(const ADC_RESULT_t *adc_result, POWER_MEASUREMENT_t *measurement)
{
    if ((adc_result == NULL) || (measurement == NULL))
    {
        return;
    }

    measurement->vin_v = BSP_Power_GetVinVoltage(adc_result->vin_raw);
    measurement->vout_v = BSP_Power_GetVoutVoltage(adc_result->vout_raw);
    measurement->iin_a = BSP_Power_GetIinCurrent(adc_result->iin_raw);
    measurement->iout_a = BSP_Power_GetIoutCurrent(adc_result->iout_raw);
    measurement->temp1_v = BSP_Power_Adc3RawToVoltage(adc_result->temp1_raw);
    measurement->temp2_v = BSP_Power_Adc2RawToVoltage(adc_result->temp2_raw);
    measurement->die_temp_c = BSP_Power_GetDieTemperature(adc_result->die_temp_raw);
}

float BSP_Power_Adc1RawToVoltage(uint16_t raw)
{
    return bsp_power_adc_raw_to_voltage(raw, BSP_POWER_ADC1_FULL_SCALE_RAW);
}

float BSP_Power_Adc2RawToVoltage(uint16_t raw)
{
    return bsp_power_adc_raw_to_voltage(raw, BSP_POWER_ADC2_FULL_SCALE_RAW);
}

float BSP_Power_Adc3RawToVoltage(uint16_t raw)
{
    return bsp_power_adc_raw_to_voltage(raw, BSP_POWER_ADC3_FULL_SCALE_RAW);
}

float BSP_Power_Adc5RawToVoltage(uint16_t raw)
{
    return bsp_power_adc_raw_to_voltage(raw, BSP_POWER_ADC5_FULL_SCALE_RAW);
}

float BSP_Power_GetVinVoltage(uint16_t raw)
{
    return BSP_Power_Adc1RawToVoltage(raw) * BSP_POWER_VIN_SENSE_SCALE;
}

float BSP_Power_GetVoutVoltage(uint16_t raw)
{
    return BSP_Power_Adc1RawToVoltage(raw) * BSP_POWER_VOUT_SENSE_SCALE;
}

float BSP_Power_GetIinCurrent(uint16_t raw)
{
    return bsp_power_current_from_raw(raw, BSP_POWER_IIN_SCALE);
}

float BSP_Power_GetIoutCurrent(uint16_t raw)
{
    return bsp_power_current_from_raw(raw, BSP_POWER_IOUT_SCALE);
}

float BSP_Power_GetDieTemperature(uint16_t raw)
{
    const float die_temp_raw_12bit = (float)raw / BSP_POWER_ADC5_TO_12BIT_SCALE;
    const float die_temp_raw_at_3v = die_temp_raw_12bit * (BSP_POWER_ADC_VREF_V / BSP_POWER_TS_CAL_VREF_V);
    const float temp_span = TS_CAL2_TEMP - TS_CAL1_TEMP;
    const float raw_span = (float)TS_CAL2 - (float)TS_CAL1;

    if (raw_span <= 0.0F)
    {
        return TS_CAL1_TEMP;
    }

    return ((die_temp_raw_at_3v - (float)TS_CAL1) * temp_span / raw_span) + TS_CAL1_TEMP;
}

float BSP_Power_GetInnerCurrentA(const POWER_MEASUREMENT_t *measurement, BSP_POWER_STAGE_MODE_t mode)
{
    if (measurement == NULL)
    {
        return 0.0F;
    }

    switch (mode)
    {
        case BSP_POWER_STAGE_MODE_BUCK:
            return measurement->iout_a;

        case BSP_POWER_STAGE_MODE_BOOST:
            return measurement->iin_a;

        case BSP_POWER_STAGE_MODE_MIXED:
        default:
            return measurement->iin_a;
    }
}
