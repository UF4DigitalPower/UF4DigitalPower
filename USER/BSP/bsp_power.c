/**
 * @file bsp_power.c
 * @brief Board-level power stage definitions and measurement helpers.
 */

#include "bsp_power.h"

#include <string.h>

BSP_DMA_RAM volatile uint16_t g_BSP_adc1RegularDma[BSP_POWER_ADC1_REGULAR_COUNT] = {0U};
volatile BSP_adcResult_t g_BSP_adcResult = {0U};

static float s_BSP_getAppAdcRawVoltage(uint16_t raw, float raw_full_scale)
{
    if (raw_full_scale <= 0.0F)
    {
        return 0.0F;
    }

    return ((float)raw * BSP_POWER_ADC_VREF_V) / raw_full_scale;
}

static float s_BSP_getAppCurrentFromRaw(uint16_t raw, float polarity)
{
    const float sense_voltage = BSP_getAppAdc1Voltage(raw);
    return ((sense_voltage - BSP_POWER_CURRENT_BIAS_V) / BSP_POWER_CURRENT_SENSE_V_PER_A) * polarity;
}

void BSP_initAppPower(void)
{
    memset((void *)g_BSP_adc1RegularDma, 0, sizeof(g_BSP_adc1RegularDma));
    memset((void *)&g_BSP_adcResult, 0, sizeof(g_BSP_adcResult));
}

void BSP_updateAppAdcResultFromBuffers(void)
{
    g_BSP_adcResult.iin_raw = g_BSP_adc1RegularDma[BSP_POWER_ADC1_REGULAR_IIN];
    g_BSP_adcResult.iout_raw = g_BSP_adc1RegularDma[BSP_POWER_ADC1_REGULAR_IOUT];
}

void BSP_setAppInjectedRaw(uint16_t vout_raw, uint16_t vin_raw)
{
    g_BSP_adcResult.vout_raw = vout_raw;
    g_BSP_adcResult.vin_raw = vin_raw;
}

void BSP_setAppAuxTemperatureRaw(uint16_t temp1_raw, uint16_t temp2_raw, uint16_t die_temp_raw)
{
    g_BSP_adcResult.temp1_raw = temp1_raw;
    g_BSP_adcResult.temp2_raw = temp2_raw;
    g_BSP_adcResult.die_temp_raw = die_temp_raw;
}

void BSP_getAppAdcResult(BSP_adcResult_t *result)
{
    if (result == NULL)
    {
        return;
    }

    BSP_updateAppAdcResultFromBuffers();

    result->iin_raw = g_BSP_adcResult.iin_raw;
    result->iout_raw = g_BSP_adcResult.iout_raw;
    result->vout_raw = g_BSP_adcResult.vout_raw;
    result->vin_raw = g_BSP_adcResult.vin_raw;
    result->temp1_raw = g_BSP_adcResult.temp1_raw;
    result->temp2_raw = g_BSP_adcResult.temp2_raw;
    result->die_temp_raw = g_BSP_adcResult.die_temp_raw;
}

void BSP_getAppMeasurement(const BSP_adcResult_t *adc_result, BSP_powerMeasurement_t *measurement)
{
    if ((adc_result == NULL) || (measurement == NULL))
    {
        return;
    }

    measurement->vin_v = BSP_getAppVinVoltage(adc_result->vin_raw);
    measurement->vout_v = BSP_getAppVoutVoltage(adc_result->vout_raw);
    measurement->iin_a = BSP_getAppIinCurrent(adc_result->iin_raw);
    measurement->iout_a = BSP_getAppIoutCurrent(adc_result->iout_raw);
    measurement->temp1_v = BSP_getAppAdc3Voltage(adc_result->temp1_raw);
    measurement->temp2_v = BSP_getAppAdc2Voltage(adc_result->temp2_raw);
    measurement->die_temp_c = BSP_getAppDieTemperature(adc_result->die_temp_raw);
}

float BSP_getAppAdc1Voltage(uint16_t raw)
{
    return s_BSP_getAppAdcRawVoltage(raw, BSP_POWER_ADC1_FULL_SCALE_RAW);
}

float BSP_getAppAdc2Voltage(uint16_t raw)
{
    return s_BSP_getAppAdcRawVoltage(raw, BSP_POWER_ADC2_FULL_SCALE_RAW);
}

float BSP_getAppAdc3Voltage(uint16_t raw)
{
    return s_BSP_getAppAdcRawVoltage(raw, BSP_POWER_ADC3_FULL_SCALE_RAW);
}

float BSP_getAppAdc5Voltage(uint16_t raw)
{
    return s_BSP_getAppAdcRawVoltage(raw, BSP_POWER_ADC5_FULL_SCALE_RAW);
}

float BSP_getAppVinVoltage(uint16_t raw)
{
    return BSP_getAppAdc1Voltage(raw) * BSP_POWER_VIN_SENSE_SCALE;
}

float BSP_getAppVoutVoltage(uint16_t raw)
{
    return BSP_getAppAdc1Voltage(raw) * BSP_POWER_VOUT_SENSE_SCALE;
}

float BSP_getAppIinCurrent(uint16_t raw)
{
    return s_BSP_getAppCurrentFromRaw(raw, BSP_POWER_IIN_SCALE);
}

float BSP_getAppIoutCurrent(uint16_t raw)
{
    return s_BSP_getAppCurrentFromRaw(raw, BSP_POWER_IOUT_SCALE);
}

float BSP_getAppDieTemperature(uint16_t raw)
{
    const float die_temp_raw_12bit = (float)raw / BSP_POWER_ADC5_TO_12BIT_SCALE;
    const float die_temp_raw_at_3v = die_temp_raw_12bit * (BSP_POWER_ADC_VREF_V / BSP_POWER_TS_CAL_VREF_V);
    const float temp_span = BSP_POWER_TS_CAL2_TEMP_C - BSP_POWER_TS_CAL1_TEMP_C;
    const float raw_span = (float)BSP_POWER_TS_CAL2_RAW - (float)BSP_POWER_TS_CAL1_RAW;

    if (raw_span <= 0.0F)
    {
        return BSP_POWER_TS_CAL1_TEMP_C;
    }

    return ((die_temp_raw_at_3v - (float)BSP_POWER_TS_CAL1_RAW) * temp_span / raw_span) + BSP_POWER_TS_CAL1_TEMP_C;
}

float BSP_getAppInnerCurrentA(const BSP_powerMeasurement_t *measurement, BSP_powerStageMode_t mode)
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
