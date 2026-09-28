#include "pwm_control.h"
#include "main.h"

#define PWM_PERIOD 27200u
#define PWM_OUTPUTS (HRTIM_OUTPUT_TE1 | HRTIM_OUTPUT_TE2 | HRTIM_OUTPUT_TF1 | HRTIM_OUTPUT_TF2)
#define PWM_TIMERS (HRTIM_TIMERID_MASTER | HRTIM_TIMERID_TIMER_E | HRTIM_TIMERID_TIMER_F)

/* Gate drive remains inhibited until the four waveforms and fault response
   are checked on the actual board with a current-limited supply. */
#ifndef CONTROL_POWER_STAGE_VALIDATED
#define CONTROL_POWER_STAGE_VALIDATED 0
#endif

static HRTIM_HandleTypeDef *s_hrtim;
static bool s_enabled;
static control_direction_t s_direction;
static control_region_t s_region;

void PwmControl_Init(HRTIM_HandleTypeDef *h)
{
    s_hrtim = h;
    s_enabled = false;
    s_direction = CONTROL_DIRECTION_FORWARD;
    s_region = CONTROL_REGION_BUCK;
    if (h != NULL)
    {
        (void)HAL_HRTIM_WaveformOutputStop(h, PWM_OUTPUTS);
        h->Instance->sMasterRegs.MCMP1R = 1u;
        h->Instance->sMasterRegs.MCMP4R = 1u;
        if (HAL_HRTIM_WaveformCountStart_IT(h, PWM_TIMERS) != HAL_OK)
            Error_Handler();
    }
}

void PwmControl_SetMode(control_direction_t direction, control_region_t region)
{
    s_direction = direction;
    s_region = region;
}

HAL_StatusTypeDef PwmControl_Enable(void)
{
    HAL_StatusTypeDef status;
    if (s_hrtim == NULL || !CONTROL_POWER_STAGE_VALIDATED)
        return HAL_ERROR;
    status = HAL_HRTIM_WaveformOutputStart(s_hrtim, PWM_OUTPUTS);
    s_enabled = (status == HAL_OK);
    if (status != HAL_OK)
        (void)HAL_HRTIM_WaveformOutputStop(s_hrtim, PWM_OUTPUTS);
    return status;
}

HAL_StatusTypeDef PwmControl_Disable(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    if (s_hrtim != NULL)
        status = HAL_HRTIM_WaveformOutputStop(s_hrtim, PWM_OUTPUTS);
    s_enabled = false;
    return status;
}

bool PwmControl_IsEnabled(void) { return s_enabled; }

HAL_StatusTypeDef PwmControl_SetDutyQ15(uint16_t duty_q15)
{
    uint32_t active, e_compare, f_compare;
    const uint32_t pass = PWM_PERIOD + 1u;
    if (s_hrtim == NULL) return HAL_ERROR;
    if (duty_q15 > 32767u) duty_q15 = 32767u;
    active = ((uint32_t)duty_q15 * PWM_PERIOD + 16384u) / 32768u;
    if (active == 0u) active = 1u;
    if (active >= PWM_PERIOD) active = PWM_PERIOD - 1u;

    /* E1/F1 drive the high sides. Boost switches the complementary low side. */
    if (s_direction == CONTROL_DIRECTION_FORWARD)
    {
        e_compare = (s_region == CONTROL_REGION_BUCK) ? active : pass;
        f_compare = (s_region == CONTROL_REGION_BUCK) ? pass : PWM_PERIOD - active;
    }
    else
    {
        e_compare = (s_region == CONTROL_REGION_BUCK) ? pass : PWM_PERIOD - active;
        f_compare = (s_region == CONTROL_REGION_BUCK) ? active : pass;
    }
    s_hrtim->Instance->sMasterRegs.MCMP4R = e_compare;
    s_hrtim->Instance->sMasterRegs.MCMP1R = f_compare;
    return HAL_OK;
}
