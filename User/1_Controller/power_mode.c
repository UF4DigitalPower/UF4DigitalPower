#include "power_mode.h"
#include <string.h>

static power_mode_config_t s_config;
static bool elapsed(uint32_t now, uint32_t then, uint32_t delay) { return (uint32_t)(now - then) >= delay; }
static void set_state(power_mode_t *m, power_mode_state_t s, uint32_t now) { m->state=s; m->state_tick=now; }
static void pwm_disable(power_mode_t *m) { if (m->pwm_enabled && s_config.pwm_disable) s_config.pwm_disable(s_config.user); m->pwm_enabled=false; }
static void pwm_enable(power_mode_t *m)
{
    if (m->pwm_enabled) return;
    if (s_config.pwm_enable) s_config.pwm_enable(s_config.user);
    /* The action may report a hardware fault synchronously. */
    m->pwm_enabled = (m->state != POWER_MODE_FAULT);
}

void PowerMode_Init(power_mode_t *m, const power_mode_config_t *cfg, uint32_t now)
{
    if (!m) return;
    memset(m,0,sizeof(*m)); s_config.self_check_ms=50u; s_config.soft_start_ms=100u;
    s_config.stopping_ms=10u; s_config.direction_deadtime_ms=20u;
    if (cfg) s_config=*cfg;
    if (!s_config.self_check_ms) s_config.self_check_ms=50u;
    if (!s_config.soft_start_ms) s_config.soft_start_ms=100u;
    if (!s_config.stopping_ms) s_config.stopping_ms=10u;
    if (s_config.direction_deadtime_ms<20u) s_config.direction_deadtime_ms=20u;
    m->state=POWER_MODE_SELF_CHECK; m->state_tick=now;
    m->requested_direction=CONTROL_DIRECTION_FORWARD; m->active_direction=CONTROL_DIRECTION_FORWARD;
    pwm_disable(m);
}
void PowerMode_RequestEnable(power_mode_t *m, bool enable) { if (m) m->requested_enable=enable; }
void PowerMode_RequestDirection(power_mode_t *m, control_direction_t d) { if (m && d<=CONTROL_DIRECTION_REVERSE) m->requested_direction=d; }
void PowerMode_ReportFault(power_mode_t *m, power_fault_reason_t r)
{
    if (!m || r==POWER_FAULT_NONE) return;
    if (m->state!=POWER_MODE_FAULT) { m->fault=r; m->fault_count++; m->state=POWER_MODE_FAULT; }
    pwm_disable(m);
}
void PowerMode_ClearFault(power_mode_t *m)
{
    if (!m || m->state!=POWER_MODE_FAULT) return;
    m->fault=POWER_FAULT_NONE; m->requested_enable=false; m->state_tick=0u; m->state=POWER_MODE_IDLE;
}
void PowerMode_Process(power_mode_t *m, uint32_t now)
{
    if (!m) return;
    if (m->state==POWER_MODE_FAULT) { pwm_disable(m); return; }
    switch (m->state)
    {
        case POWER_MODE_SELF_CHECK:
            pwm_disable(m);
            if (s_config.self_check && !s_config.self_check(s_config.user)) { PowerMode_ReportFault(m,POWER_FAULT_SELF_CHECK); return; }
            if (elapsed(now,m->state_tick,s_config.self_check_ms)) { m->self_check_passed=true; set_state(m,POWER_MODE_IDLE,now); }
            break;
        case POWER_MODE_IDLE:
            pwm_disable(m);
            if (m->requested_enable) { pwm_enable(m); set_state(m,POWER_MODE_SOFT_START,now); }
            break;
        case POWER_MODE_SOFT_START:
            if (!m->requested_enable) { pwm_disable(m); set_state(m,POWER_MODE_STOPPING,now); }
            else if (m->requested_direction!=m->active_direction) { pwm_disable(m); set_state(m,POWER_MODE_DIRECTION_SWITCH,now); }
            else if (elapsed(now,m->state_tick,s_config.soft_start_ms)) set_state(m,POWER_MODE_RUN,now);
            break;
        case POWER_MODE_RUN:
            if (!m->requested_enable) { pwm_disable(m); set_state(m,POWER_MODE_STOPPING,now); }
            else if (m->requested_direction!=m->active_direction) { pwm_disable(m); set_state(m,POWER_MODE_DIRECTION_SWITCH,now); }
            break;
        case POWER_MODE_STOPPING:
            pwm_disable(m);
            if (m->requested_enable) { set_state(m,POWER_MODE_SOFT_START,now); pwm_enable(m); }
            else if (elapsed(now,m->state_tick,s_config.stopping_ms)) set_state(m,POWER_MODE_IDLE,now);
            break;
        case POWER_MODE_DIRECTION_SWITCH:
            pwm_disable(m);
            if (elapsed(now,m->state_tick,s_config.direction_deadtime_ms))
            {
                m->active_direction=m->requested_direction;
                if (m->requested_enable) { pwm_enable(m); set_state(m,POWER_MODE_SOFT_START,now); }
                else set_state(m,POWER_MODE_IDLE,now);
            }
            break;
        default: PowerMode_ReportFault(m,POWER_FAULT_EXTERNAL); break;
    }
}
power_mode_state_t PowerMode_GetState(const power_mode_t *m) { return m ? m->state : POWER_MODE_FAULT; }
power_fault_reason_t PowerMode_GetFault(const power_mode_t *m) { return m ? m->fault : POWER_FAULT_EXTERNAL; }
control_direction_t PowerMode_GetDirection(const power_mode_t *m) { return m ? m->active_direction : CONTROL_DIRECTION_FORWARD; }
bool PowerMode_IsPwmEnabled(const power_mode_t *m) { return m && m->pwm_enabled; }
uint16_t PowerMode_GetFlagBits(const power_mode_t *m)
{
    uint16_t f=0u; if (!m) return 0u; if (m->requested_enable) f|=1u; if (m->pwm_enabled) f|=2u;
    if (m->fault!=POWER_FAULT_NONE) f|=4u; if (m->active_direction==CONTROL_DIRECTION_REVERSE) f|=8u;
    if (m->self_check_passed) f|=16u; return f;
}
