/**
 * @file bsp_power.h
 * @brief Board-level power stage definitions and measurement helpers.
 */

#ifndef UF4DIGITALPOWER_BSP_POWER_H
#define UF4DIGITALPOWER_BSP_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef CCMRAM
#define CCMRAM __attribute__((section("ccmram")))
#endif

/* ADC topology ------------------------------------------------------------- */

#define BSP_POWER_ADC1_REGULAR_COUNT  2U
#define BSP_POWER_ADC1_INJECTED_COUNT 2U

typedef enum
{
    BSP_POWER_ADC1_REGULAR_IIN = 0U,
    BSP_POWER_ADC1_REGULAR_IOUT,
} BSP_POWER_ADC1_REGULAR_INDEX_t;

typedef enum
{
    BSP_POWER_ADC1_INJECTED_VOUT = 0U,
    BSP_POWER_ADC1_INJECTED_VIN,
} BSP_POWER_ADC1_INJECTED_INDEX_t;

typedef enum
{
    BSP_POWER_STAGE_MODE_BUCK = 0U,
    BSP_POWER_STAGE_MODE_BOOST,
    BSP_POWER_STAGE_MODE_MIXED,
} BSP_POWER_STAGE_MODE_t;

typedef struct
{
    uint16_t iin_raw;
    uint16_t iout_raw;
    uint16_t vout_raw;
    uint16_t vin_raw;
    uint16_t temp1_raw;
    uint16_t temp2_raw;
    uint16_t die_temp_raw;
} ADC_RESULT_t;

typedef struct
{
    float vin_v;
    float vout_v;
    float iin_a;
    float iout_a;
    float temp1_v;
    float temp2_v;
    float die_temp_c;
} POWER_MEASUREMENT_t;

/* ADC scaling -------------------------------------------------------------- */

#define BSP_POWER_ADC_12BIT_MAX_RAW          4095.0F
#define BSP_POWER_ADC1_FULL_SCALE_RAW        8190.0F
#define BSP_POWER_ADC2_FULL_SCALE_RAW        65520.0F
#define BSP_POWER_ADC3_FULL_SCALE_RAW        65520.0F
#define BSP_POWER_ADC5_FULL_SCALE_RAW        32760.0F
#define BSP_POWER_ADC5_TO_12BIT_SCALE        8.0F

#define BSP_POWER_ADC_VREF_V                 3.2806F
#define BSP_POWER_TS_CAL_VREF_V              3.0F

#define TS_CAL1_ADDR                         0x1FFF75A8UL
#define TS_CAL2_ADDR                         0x1FFF75CAUL
#define TS_CAL1                             (*(volatile uint16_t *)TS_CAL1_ADDR)
#define TS_CAL2                             (*(volatile uint16_t *)TS_CAL2_ADDR)
#define TS_CAL1_TEMP                         30.0F
#define TS_CAL2_TEMP                         130.0F

/*
 * Per this board's voltage front-end topology, the effective restore scale is
 * defined directly by the two matching resistors:
 *
 *   V_real = V_adc_pin * (R_upper / R_lower)
 *
 * Replace the resistor values below with the exact schematic values.
 */
#define BSP_POWER_STAGE_MAX_VOLTAGE_V        50.0F
#define BSP_POWER_DIVIDER_SCALE(r_upper, r_lower) ((r_upper) / (r_lower))

#define BSP_POWER_VIN_R_UPPER_OHM            47000.0F
#define BSP_POWER_VIN_R_LOWER_OHM            3300.0F
#define BSP_POWER_VOUT_R_UPPER_OHM           47000.0F
#define BSP_POWER_VOUT_R_LOWER_OHM           3300.0F

#define BSP_POWER_VIN_SENSE_SCALE            BSP_POWER_DIVIDER_SCALE(BSP_POWER_VIN_R_UPPER_OHM, BSP_POWER_VIN_R_LOWER_OHM)
#define BSP_POWER_VOUT_SENSE_SCALE           BSP_POWER_DIVIDER_SCALE(BSP_POWER_VOUT_R_UPPER_OHM, BSP_POWER_VOUT_R_LOWER_OHM)

/*
 * Current sensing model from README:
 * 8 mOhm shunt, gain of 20, and 1.65 V mid-bias for bidirectional sensing.
 * If channel polarity is inverted on hardware, flip the corresponding scale sign.
 */
#define BSP_POWER_CURRENT_SHUNT_OHM          0.008F
#define BSP_POWER_CURRENT_AMP_GAIN           20.0F
#define BSP_POWER_CURRENT_BIAS_V             1.650F
#define BSP_POWER_CURRENT_SENSE_V_PER_A      (BSP_POWER_CURRENT_SHUNT_OHM * BSP_POWER_CURRENT_AMP_GAIN)
#define BSP_POWER_IIN_SCALE                  1.0F
#define BSP_POWER_IOUT_SCALE                 1.0F

/* Compatibility aliases with the previous placeholder header. */
#define ADC_MAX_VALUE                        BSP_POWER_ADC1_FULL_SCALE_RAW
#define REF_3V3                              BSP_POWER_ADC_VREF_V

/* PWM duty limits ---------------------------------------------------------- */

#define MIN_BUCK_DUTY                        136U
#define MAX_BUCK_DUTY                        25840U
#define MAX_BUCK_DUTY1                       21760U
#define MIN_BOOST_DUTY                       136U
#define MIN_BOOST_DUTY1                      1800U
#define MAX_BOOST_DUTY                       17680U
#define MAX_BOOST_DUTY1                      25840U

/* Shared ADC buffers ------------------------------------------------------- */

extern CCMRAM volatile uint16_t g_power_adc1_regular_dma[BSP_POWER_ADC1_REGULAR_COUNT];
extern volatile ADC_RESULT_t g_power_adc_result;

/* Board helpers ------------------------------------------------------------ */

void BSP_Power_Init(void);
void BSP_Power_RefreshAdcResultFromBuffers(void);
void BSP_Power_SetInjectedRaw(uint16_t vout_raw, uint16_t vin_raw);
void BSP_Power_SetAuxTemperatureRaw(uint16_t temp1_raw, uint16_t temp2_raw, uint16_t die_temp_raw);
void BSP_Power_GetAdcResult(ADC_RESULT_t *result);
void BSP_Power_ConvertToMeasurement(const ADC_RESULT_t *adc_result, POWER_MEASUREMENT_t *measurement);

float BSP_Power_Adc1RawToVoltage(uint16_t raw);
float BSP_Power_Adc2RawToVoltage(uint16_t raw);
float BSP_Power_Adc3RawToVoltage(uint16_t raw);
float BSP_Power_Adc5RawToVoltage(uint16_t raw);

float BSP_Power_GetVinVoltage(uint16_t raw);
float BSP_Power_GetVoutVoltage(uint16_t raw);
float BSP_Power_GetIinCurrent(uint16_t raw);
float BSP_Power_GetIoutCurrent(uint16_t raw);
float BSP_Power_GetDieTemperature(uint16_t raw);
float BSP_Power_GetInnerCurrentA(const POWER_MEASUREMENT_t *measurement, BSP_POWER_STAGE_MODE_t mode);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_BSP_POWER_H */
