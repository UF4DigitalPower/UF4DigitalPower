#include "protection_control.h"
#include "comp.h"
#include "dac.h"

#define PROTECTION_DAC_MAX_CODE 4095u
#define PROTECTION_ZERO_MIN_MV 1450u
#define PROTECTION_ZERO_MAX_MV 1850u
#define PROTECTION_STATUS_COMP1  (1u << 0)
#define PROTECTION_STATUS_COMP2  (1u << 1)
#define PROTECTION_STATUS_COMP3  (1u << 2)
#define PROTECTION_STATUS_COMP4  (1u << 3)
#define PROTECTION_STATUS_COMP6  (1u << 4)
#define PROTECTION_STATUS_COMP7  (1u << 5)
#define PROTECTION_STATUS_READY  (1u << 6)
#define PROTECTION_STATUS_HEADROOM_FAULT (1u << 7)

static bool s_ready;
static bool s_current_headroom;

static uint16_t clamp_dac_code(uint64_t code)
{
    if (code == 0u)
        return 1u;
    if (code > PROTECTION_DAC_MAX_CODE)
        return PROTECTION_DAC_MAX_CODE;
    return (uint16_t)code;
}

static uint32_t current_delta_code(uint16_t current_10mA, uint16_t shunt_10uohm,
                                   uint16_t gain_centi, uint16_t vref_10mV)
{
    const uint64_t current_mA = (uint64_t)current_10mA * 10u;
    const uint64_t shunt_uohm = (uint64_t)shunt_10uohm * 10u;
    const uint64_t vref_mV = (uint64_t)vref_10mV * 10u;

    if (vref_mV == 0u || shunt_uohm == 0u || gain_centi == 0u)
        return PROTECTION_DAC_MAX_CODE + 1u;

    /* mA * uOhm * gain/100 is nanovolts; scale directly to 12-bit DAC codes. */
    return (uint32_t)((current_mA * shunt_uohm * gain_centi * PROTECTION_DAC_MAX_CODE +
                       vref_mV * 50000000u) / (vref_mV * 100000000u));
}

static uint16_t voltage_to_dac_code(uint16_t voltage_10mV, uint16_t r_upper_100ohm,
                                    uint16_t r_lower_100ohm, uint16_t vref_10mV)
{
    const uint64_t voltage_mV = (uint64_t)voltage_10mV * 10u;
    const uint64_t vref_mV = (uint64_t)vref_10mV * 10u;

    if (vref_mV == 0u || r_upper_100ohm == 0u || r_lower_100ohm == 0u)
        return PROTECTION_DAC_MAX_CODE;

    return clamp_dac_code((voltage_mV * r_lower_100ohm * PROTECTION_DAC_MAX_CODE +
                           ((uint64_t)(r_upper_100ohm + (uint32_t)r_lower_100ohm) * vref_mV / 2u)) /
                          ((uint64_t)(r_upper_100ohm + (uint32_t)r_lower_100ohm) * vref_mV));
}

bool ProtectionControl_Apply(const protection_settings_t *settings)
{
    uint16_t iin_high;
    uint16_t iin_low;
    uint16_t iout_high;
    uint16_t iout_low;
    uint16_t vin_ovp;
    uint16_t vout_ovp;
    uint32_t iin_delta, iout_delta;

    if (settings == NULL || settings->ocp_10mA == 0u || settings->adc_vref_10mV == 0u ||
        settings->vin_ovp_10mV == 0u || settings->vout_ovp_10mV == 0u ||
        settings->vin_r_upper_100ohm == 0u || settings->vin_r_lower_100ohm == 0u ||
        settings->vout_r_upper_100ohm == 0u || settings->vout_r_lower_100ohm == 0u ||
        settings->shunt_in_10uohm == 0u || settings->shunt_out_10uohm == 0u ||
        settings->gain_in_centi == 0u || settings->gain_out_centi == 0u ||
        settings->current_zero_adc == 0u ||
        settings->current_zero_adc >= PROTECTION_DAC_MAX_CODE)
        return false;

    iin_delta = current_delta_code(settings->ocp_10mA, settings->shunt_in_10uohm,
                                   settings->gain_in_centi, settings->adc_vref_10mV);
    iout_delta = current_delta_code(settings->ocp_10mA, settings->shunt_out_10uohm,
                                    settings->gain_out_centi, settings->adc_vref_10mV);
    s_current_headroom =
                         (uint32_t)settings->current_zero_adc *
                             settings->adc_vref_10mV * 10u >=
                             PROTECTION_ZERO_MIN_MV * PROTECTION_DAC_MAX_CODE &&
                         (uint32_t)settings->current_zero_adc *
                             settings->adc_vref_10mV * 10u <=
                             PROTECTION_ZERO_MAX_MV * PROTECTION_DAC_MAX_CODE &&
                         iin_delta < settings->current_zero_adc &&
                         iout_delta < settings->current_zero_adc &&
                         iin_delta < PROTECTION_DAC_MAX_CODE - settings->current_zero_adc &&
                         iout_delta < PROTECTION_DAC_MAX_CODE - settings->current_zero_adc;
    iin_high = clamp_dac_code((uint64_t)settings->current_zero_adc + iin_delta);
    iin_low = clamp_dac_code(iin_delta < settings->current_zero_adc ?
                              settings->current_zero_adc - iin_delta : 0u);
    iout_high = clamp_dac_code((uint64_t)settings->current_zero_adc + iout_delta);
    iout_low = clamp_dac_code(iout_delta < settings->current_zero_adc ?
                               settings->current_zero_adc - iout_delta : 0u);
    vin_ovp = voltage_to_dac_code(settings->vin_ovp_10mV, settings->vin_r_upper_100ohm,
                                  settings->vin_r_lower_100ohm, settings->adc_vref_10mV);
    vout_ovp = voltage_to_dac_code(settings->vout_ovp_10mV, settings->vout_r_upper_100ohm,
                                   settings->vout_r_lower_100ohm, settings->adc_vref_10mV);

    return HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, iin_high) == HAL_OK &&
           HAL_DAC_SetValue(&hdac3, DAC_CHANNEL_1, DAC_ALIGN_12B_R, iin_low) == HAL_OK &&
           HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, iout_high) == HAL_OK &&
           HAL_DAC_SetValue(&hdac4, DAC_CHANNEL_1, DAC_ALIGN_12B_R, iout_low) == HAL_OK &&
           HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, vin_ovp) == HAL_OK &&
           HAL_DAC_SetValue(&hdac3, DAC_CHANNEL_2, DAC_ALIGN_12B_R, vout_ovp) == HAL_OK;
}

bool ProtectionControl_Init(const protection_settings_t *settings)
{
    s_ready = false;
    if (!ProtectionControl_Apply(settings))
        return false;

    if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_Start(&hdac2, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_Start(&hdac3, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_Start(&hdac4, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_Start(&hdac1, DAC_CHANNEL_2) != HAL_OK ||
        HAL_DAC_Start(&hdac3, DAC_CHANNEL_2) != HAL_OK ||
        HAL_COMP_Start(&hcomp1) != HAL_OK || HAL_COMP_Start(&hcomp2) != HAL_OK ||
        HAL_COMP_Start(&hcomp3) != HAL_OK || HAL_COMP_Start(&hcomp4) != HAL_OK ||
        HAL_COMP_Start(&hcomp6) != HAL_OK || HAL_COMP_Start(&hcomp7) != HAL_OK)
        return false;

    s_ready = true;
    return true;
}

bool ProtectionControl_IsReady(void)
{
    return s_ready;
}

bool ProtectionControl_HasCurrentHeadroom(void)
{
    return s_current_headroom;
}

uint16_t ProtectionControl_GetStatus(void)
{
    uint16_t status = 0u;

    /* CSR VALUE is the raw comparison, before configured output polarity. */
    if (HAL_COMP_GetOutputLevel(&hcomp1) == COMP_OUTPUT_LEVEL_HIGH) status |= PROTECTION_STATUS_COMP1;
    if (HAL_COMP_GetOutputLevel(&hcomp2) == COMP_OUTPUT_LEVEL_HIGH) status |= PROTECTION_STATUS_COMP2;
    if (HAL_COMP_GetOutputLevel(&hcomp3) == COMP_OUTPUT_LEVEL_LOW) status |= PROTECTION_STATUS_COMP3;
    if (HAL_COMP_GetOutputLevel(&hcomp4) == COMP_OUTPUT_LEVEL_HIGH) status |= PROTECTION_STATUS_COMP4;
    if (HAL_COMP_GetOutputLevel(&hcomp6) == COMP_OUTPUT_LEVEL_HIGH) status |= PROTECTION_STATUS_COMP6;
    if (HAL_COMP_GetOutputLevel(&hcomp7) == COMP_OUTPUT_LEVEL_LOW) status |= PROTECTION_STATUS_COMP7;
    if (s_ready) status |= PROTECTION_STATUS_READY;
    if (!s_current_headroom) status |= PROTECTION_STATUS_HEADROOM_FAULT;
    return status;
}

bool ProtectionControl_IsTripActive(void)
{
    return (ProtectionControl_GetStatus() & 0x003fu) != 0u;
}

uint16_t ProtectionControl_GetDac3Ch1Code(void)
{
    return (uint16_t)(hdac3.Instance->DOR1 & DAC_DOR1_DACC1DOR);
}

uint16_t ProtectionControl_GetDac4Ch1Code(void)
{
    return (uint16_t)(hdac4.Instance->DOR1 & DAC_DOR1_DACC1DOR);
}
