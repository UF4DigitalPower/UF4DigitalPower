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

#ifndef BSP_DMA_RAM
#define BSP_DMA_RAM
#endif

/* ADC topology ------------------------------------------------------------- */

#define BSP_POWER_ADC1_REGULAR_COUNT  2U
#define BSP_POWER_ADC1_INJECTED_COUNT 2U

typedef enum
{
    BSP_POWER_ADC1_REGULAR_IIN = 0U,
    BSP_POWER_ADC1_REGULAR_IOUT,
} BSP_powerAdc1RegularIndex_t;

typedef enum
{
    BSP_POWER_ADC1_INJECTED_VOUT = 0U,
    BSP_POWER_ADC1_INJECTED_VIN,
} BSP_powerAdc1InjectedIndex_t;

typedef enum
{
    BSP_POWER_STAGE_MODE_BUCK = 0U,
    BSP_POWER_STAGE_MODE_BOOST,
    BSP_POWER_STAGE_MODE_MIXED,
} BSP_powerStageMode_t;

typedef struct
{
    uint16_t iin_raw;
    uint16_t iout_raw;
    uint16_t vout_raw;
    uint16_t vin_raw;
    uint16_t temp1_raw;
    uint16_t temp2_raw;
    uint16_t die_temp_raw;
} BSP_adcResult_t;

typedef struct
{
    float vin_v;
    float vout_v;
    float iin_a;
    float iout_a;
    float temp1_v;
    float temp2_v;
    float die_temp_c;
} BSP_powerMeasurement_t;

/* ADC scaling -------------------------------------------------------------- */

#define BSP_POWER_ADC_12BIT_MAX_RAW          4095.0F
#define BSP_POWER_ADC1_FULL_SCALE_RAW        BSP_POWER_ADC_12BIT_MAX_RAW
#define BSP_POWER_ADC2_FULL_SCALE_RAW        65520.0F
#define BSP_POWER_ADC3_FULL_SCALE_RAW        65520.0F
#define BSP_POWER_ADC5_FULL_SCALE_RAW        32760.0F
#define BSP_POWER_ADC5_TO_12BIT_SCALE        8.0F

#define BSP_POWER_ADC_VREF_V                 3.2806F
#define BSP_POWER_TS_CAL_VREF_V              3.0F

#define BSP_POWER_TS_CAL1_ADDR               0x1FFF75A8UL
#define BSP_POWER_TS_CAL2_ADDR               0x1FFF75CAUL
#define BSP_POWER_TS_CAL1_RAW               (*(volatile uint16_t *)BSP_POWER_TS_CAL1_ADDR)
#define BSP_POWER_TS_CAL2_RAW               (*(volatile uint16_t *)BSP_POWER_TS_CAL2_ADDR)
#define BSP_POWER_TS_CAL1_TEMP_C             30.0F
#define BSP_POWER_TS_CAL2_TEMP_C             130.0F

/*
 * The example project restores VIN/VOUT with:
 *
 *   V_real = V_adc_pin * (R_upper / R_lower)
 */
#define BSP_POWER_STAGE_MAX_VOLTAGE_V        50.0F
#define BSP_POWER_DIVIDER_SCALE(r_upper, r_lower) ((r_upper) / (r_lower))

#define BSP_POWER_VIN_R_UPPER_OHM            75000.0F
#define BSP_POWER_VIN_R_LOWER_OHM            4700.0F
#define BSP_POWER_VOUT_R_UPPER_OHM           75000.0F
#define BSP_POWER_VOUT_R_LOWER_OHM           4700.0F

#define BSP_POWER_VIN_SENSE_SCALE            BSP_POWER_DIVIDER_SCALE(BSP_POWER_VIN_R_UPPER_OHM, BSP_POWER_VIN_R_LOWER_OHM)
#define BSP_POWER_VOUT_SENSE_SCALE           BSP_POWER_DIVIDER_SCALE(BSP_POWER_VOUT_R_UPPER_OHM, BSP_POWER_VOUT_R_LOWER_OHM)

/*
 * Current sensing uses a 1.65 V midpoint. Values above the midpoint are
 * reverse current and are clipped to 0 A; 1.65 V down to 0 V is positive
 * current. Keep board-specific shunt/gain/trim here.
 */
#define BSP_POWER_CURRENT_SHUNT_OHM          0.005F
#define BSP_POWER_CURRENT_AMP_GAIN           62.0F
#define BSP_POWER_CURRENT_BIAS_V             1.650F
#define BSP_POWER_CURRENT_SENSE_V_PER_A      (BSP_POWER_CURRENT_SHUNT_OHM * BSP_POWER_CURRENT_AMP_GAIN)
#define BSP_POWER_IIN_SCALE                  1.0F
#define BSP_POWER_IOUT_SCALE                 1.0F

/* PWM duty limits ---------------------------------------------------------- */

#define BSP_POWER_HRTIM_PERIOD_TICK          27200U
#define BSP_POWER_BUCK_DUTY_MIN_TICK         136U
#define BSP_POWER_BUCK_DUTY_MAX_TICK         25840U
#define BSP_POWER_BUCK_DUTY_SYNC_MAX_TICK    21760U
#define BSP_POWER_BOOST_DUTY_MIN_TICK        136U
#define BSP_POWER_BOOST_DUTY_SYNC_MIN_TICK   1800U
#define BSP_POWER_BOOST_DUTY_MAX_TICK        17680U
#define BSP_POWER_BOOST_DUTY_SYNC_MAX_TICK   25840U

/* Example-project raw trims: y = raw * K / 4096 + B. */
#define BSP_POWER_VOUT_RAW_CAL_K             4099U
#define BSP_POWER_VOUT_RAW_CAL_B             1U
#define BSP_POWER_IOUT_RAW_CAL_K             4095U
#define BSP_POWER_IOUT_RAW_CAL_B             1U

/* Shared ADC buffers ------------------------------------------------------- */

extern BSP_DMA_RAM volatile uint16_t g_BSP_adc1RegularDma[BSP_POWER_ADC1_REGULAR_COUNT];
extern volatile BSP_adcResult_t g_BSP_adcResult;

/* Board helpers ------------------------------------------------------------ */

void BSP_initAppPower(void);
void BSP_updateAppAdcResultFromBuffers(void);
void BSP_setAppInjectedRaw(uint16_t vout_raw, uint16_t vin_raw);
void BSP_setAppAuxTemperatureRaw(uint16_t temp1_raw, uint16_t temp2_raw, uint16_t die_temp_raw);
void BSP_getAppAdcResult(BSP_adcResult_t *result);
void BSP_getAppMeasurement(const BSP_adcResult_t *adc_result, BSP_powerMeasurement_t *measurement);

float BSP_getAppAdc1Voltage(uint16_t raw);
float BSP_getAppAdc2Voltage(uint16_t raw);
float BSP_getAppAdc3Voltage(uint16_t raw);
float BSP_getAppAdc5Voltage(uint16_t raw);

float BSP_getAppVinVoltage(uint16_t raw);
float BSP_getAppVoutVoltage(uint16_t raw);
float BSP_getAppIinCurrent(uint16_t raw);
float BSP_getAppIoutCurrent(uint16_t raw);
float BSP_getAppDieTemperature(uint16_t raw);
float BSP_getAppInnerCurrentA(const BSP_powerMeasurement_t *measurement, BSP_powerStageMode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_BSP_POWER_H */
