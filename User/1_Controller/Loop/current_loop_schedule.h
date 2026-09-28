#ifndef CURRENT_LOOP_SCHEDULE_H
#define CURRENT_LOOP_SCHEDULE_H

#include <stdint.h>
#include "control_config.h"
#include "control_types.h"

CONTROL_RAMFUNC int16_t CurrentLoopSchedule_B0(control_region_t region,
                                               uint32_t vin_mV,
                                               uint32_t vout_mV,
                                               uint16_t iout_mA);

#endif /* CURRENT_LOOP_SCHEDULE_H */
