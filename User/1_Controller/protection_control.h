#ifndef PROTECTION_CONTROL_H
#define PROTECTION_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/* All public configuration values use the same units as the UF4 parameter table. */
typedef struct
{
    uint16_t ocp_10mA;
    uint16_t adc_vref_10mV;
    uint16_t current_zero_adc;
    uint16_t vin_ovp_10mV;
    uint16_t vout_ovp_10mV;
    uint16_t vin_r_upper_100ohm;
    uint16_t vin_r_lower_100ohm;
    uint16_t vout_r_upper_100ohm;
    uint16_t vout_r_lower_100ohm;
    uint16_t shunt_in_10uohm;
    uint16_t shunt_out_10uohm;
    uint16_t gain_in_centi;
    uint16_t gain_out_centi;
} protection_settings_t;

bool ProtectionControl_Init(const protection_settings_t *settings);
bool ProtectionControl_Apply(const protection_settings_t *settings);
bool ProtectionControl_IsReady(void);
bool ProtectionControl_HasCurrentHeadroom(void);
bool ProtectionControl_IsTripActive(void);
uint16_t ProtectionControl_GetStatus(void);
uint16_t ProtectionControl_GetDac3Ch1Code(void);
uint16_t ProtectionControl_GetDac4Ch1Code(void);

#endif
