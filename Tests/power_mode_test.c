#include "power_mode.h"
#include <assert.h>
#include <stdio.h>

static unsigned enabled, disabled;
static bool ready = true;
static bool fail_enable;
static power_mode_t *active_mode;

static void turn_on(void *user)
{
    (void)user;
    enabled++;
    if (fail_enable)
        PowerMode_ReportFault(active_mode, POWER_FAULT_PWM);
}

static void turn_off(void *user)
{
    (void)user;
    disabled++;
}

static bool check_ready(void *user)
{
    (void)user;
    return ready;
}

int main(void)
{
    power_mode_t mode;
    power_mode_config_t config = {
        .self_check_ms = 50u,
        .soft_start_ms = 100u,
        .stopping_ms = 10u,
        .direction_deadtime_ms = 20u,
        .pwm_enable = turn_on,
        .pwm_disable = turn_off,
        .self_check = check_ready
    };
    active_mode = &mode;

    PowerMode_Init(&mode, &config, 100u);
    assert(PowerMode_GetState(&mode) == POWER_MODE_SELF_CHECK);
    PowerMode_Process(&mode, 150u);
    assert(PowerMode_GetState(&mode) == POWER_MODE_IDLE);
    PowerMode_RequestEnable(&mode, true);
    PowerMode_Process(&mode, 151u);
    assert(PowerMode_IsPwmEnabled(&mode) && enabled == 1u);
    PowerMode_ReportFault(&mode, POWER_FAULT_OVERCURRENT);
    assert(PowerMode_GetFault(&mode) == POWER_FAULT_OVERCURRENT);
    assert(!PowerMode_IsPwmEnabled(&mode) && disabled == 1u);
    PowerMode_ReportFault(&mode, POWER_FAULT_PWM);
    assert(PowerMode_GetFault(&mode) == POWER_FAULT_OVERCURRENT);
    PowerMode_Process(&mode, 200u);
    assert(enabled == 1u);

    PowerMode_ClearFault(&mode, 201u);
    assert(PowerMode_GetState(&mode) == POWER_MODE_SELF_CHECK);
    assert(!mode.self_check_passed && !mode.requested_enable);
    ready = false;
    PowerMode_Process(&mode, 202u);
    assert(PowerMode_GetFault(&mode) == POWER_FAULT_SELF_CHECK);
    ready = true;
    PowerMode_ClearFault(&mode, 203u);
    PowerMode_Process(&mode, 253u);
    assert(PowerMode_GetState(&mode) == POWER_MODE_IDLE);
    assert(enabled == 1u);
    fail_enable = true;
    PowerMode_RequestEnable(&mode, true);
    PowerMode_Process(&mode, 254u);
    assert(PowerMode_GetState(&mode) == POWER_MODE_FAULT);
    assert(PowerMode_GetFault(&mode) == POWER_FAULT_PWM);
    assert(!PowerMode_IsPwmEnabled(&mode));
    puts("power_mode_test: OK");
    return 0;
}
