#include "pwm_control.h"
#define PWM_OUTPUTS (HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2|HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2)
#define PWM_TIMERS (HRTIM_TIMERID_MASTER|HRTIM_TIMERID_TIMER_E|HRTIM_TIMERID_TIMER_F)
static HRTIM_HandleTypeDef *s_hrtim;
static bool s_enabled;
void PwmControl_Init(HRTIM_HandleTypeDef *h)
{
    HRTIM_CompareCfgTypeDef compare={0};
    s_hrtim=h; s_enabled=false;
    if (h)
    {
        (void)HAL_HRTIM_WaveformOutputStop(h,PWM_OUTPUTS);
        /* No fixed duty is allowed before the fast loop owns the compare values. */
        compare.CompareValue=0u;
        (void)HAL_HRTIM_WaveformCompareConfig(h,HRTIM_TIMERINDEX_MASTER,HRTIM_COMPAREUNIT_1,&compare);
        (void)HAL_HRTIM_WaveformCompareConfig(h,HRTIM_TIMERINDEX_MASTER,HRTIM_COMPAREUNIT_4,&compare);
    }
}
HAL_StatusTypeDef PwmControl_Enable(void)
{
    HAL_StatusTypeDef st; if (!s_hrtim) return HAL_ERROR;
    st=HAL_HRTIM_WaveformCountStart(s_hrtim,PWM_TIMERS); if (st!=HAL_OK) return st;
    st=HAL_HRTIM_WaveformOutputStart(s_hrtim,PWM_OUTPUTS); s_enabled=(st==HAL_OK); return st;
}
HAL_StatusTypeDef PwmControl_Disable(void)
{
    HAL_StatusTypeDef st=HAL_OK; if (s_hrtim) { st=HAL_HRTIM_WaveformOutputStop(s_hrtim,PWM_OUTPUTS); (void)HAL_HRTIM_WaveformCountStop(s_hrtim,PWM_TIMERS); }
    s_enabled=false; return st;
}
bool PwmControl_IsEnabled(void) { return s_enabled; }
HAL_StatusTypeDef PwmControl_SetDutyQ15(uint16_t duty_q15)
{
    HRTIM_CompareCfgTypeDef compare={0};
    uint32_t value;
    if (!s_hrtim) return HAL_ERROR;
    if (duty_q15>32767u) duty_q15=32767u;
    value=((uint32_t)duty_q15*27200u+16384u)/32768u;
    if (value>=27200u) value=27199u;
    compare.CompareValue=value;
    if (HAL_HRTIM_WaveformCompareConfig(s_hrtim,HRTIM_TIMERINDEX_MASTER,HRTIM_COMPAREUNIT_1,&compare)!=HAL_OK) return HAL_ERROR;
    return HAL_HRTIM_WaveformCompareConfig(s_hrtim,HRTIM_TIMERINDEX_MASTER,HRTIM_COMPAREUNIT_4,&compare);
}
