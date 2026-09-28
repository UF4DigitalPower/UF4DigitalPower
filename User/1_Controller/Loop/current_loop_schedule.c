#include "current_loop_schedule.h"
#include "control_config.h"

static const uint32_t s_vin_mV[13] = {
    12000u, 15000u, 20000u, 24000u, 28000u, 32000u, 36000u,
    40000u, 48000u, 56000u, 60000u, 64000u, 70000u
};

static const uint32_t s_vout_mV[18] = {
    1000u, 2000u, 3300u, 5000u, 8000u, 12000u, 16000u, 20000u,
    24000u, 28000u, 32000u, 36000u, 40000u, 48000u, 56000u,
    60000u, 64000u, 70000u
};

static const uint32_t s_iout_mA[8] = {
    500u, 1000u, 2000u, 3000u, 5000u, 7000u, 8000u, 10000u
};

#include "current_loop_schedule_data.inc"

CONTROL_FORCE_INLINE uint8_t nearest_index_u32(const uint32_t *values,
                                               uint8_t count,
                                               uint32_t value)
{
    uint8_t best = 0u;
    uint32_t best_distance = UINT32_MAX;
    uint8_t i;

    for (i = 0u; i < count; ++i) {
        uint32_t distance = (values[i] > value) ?
                            (values[i] - value) :
                            (value - values[i]);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return best;
}

CONTROL_RAMFUNC int16_t CurrentLoopSchedule_B0(control_region_t region,
                                                uint32_t vin_mV,
                                                uint32_t vout_mV,
                                                uint16_t iout_mA)
{
    uint8_t r = (region == CONTROL_REGION_BOOST) ? 1u : 0u;
    uint8_t iv = nearest_index_u32(s_vin_mV, 13u, vin_mV);
    uint8_t ov = nearest_index_u32(s_vout_mV, 18u, vout_mV);
    uint8_t ii = nearest_index_u32(s_iout_mA, 8u, iout_mA);
    int16_t b0 = s_b0_q15[r][iv][ov][ii];

    if (b0 != 0) return b0;

    /* A mode-transition sample can land on an invalid grid cell. Find the
       closest valid coefficient once, without weakening valid grid cells. */
    {
        uint32_t best_distance = UINT32_MAX;
        uint8_t best_r = 0u;
        uint8_t best_v = 0u;
        uint8_t best_o = 0u;
        uint8_t best_i = 0u;
        uint8_t rr, vv, oo, ii_scan;

        for (rr = 0u; rr < 2u; ++rr) {
            for (vv = 0u; vv < 13u; ++vv) {
                for (oo = 0u; oo < 18u; ++oo) {
                    for (ii_scan = 0u; ii_scan < 8u; ++ii_scan) {
                        uint32_t distance;
                        if (s_b0_q15[rr][vv][oo][ii_scan] == 0) continue;
                        distance = (uint32_t)(s_vin_mV[vv] > vin_mV ?
                            s_vin_mV[vv] - vin_mV : vin_mV - s_vin_mV[vv]);
                        distance += (uint32_t)(s_vout_mV[oo] > vout_mV ?
                            s_vout_mV[oo] - vout_mV : vout_mV - s_vout_mV[oo]);
                        distance += (uint32_t)(s_iout_mA[ii_scan] > iout_mA ?
                            s_iout_mA[ii_scan] - iout_mA : iout_mA - s_iout_mA[ii_scan]) * 2u;
                        if ((rr != r) && (distance < UINT32_MAX - 100000u)) {
                            distance += 100000u;
                        }
                        if (distance < best_distance) {
                            best_distance = distance;
                            best_r = rr;
                            best_v = vv;
                            best_o = oo;
                            best_i = ii_scan;
                        }
                    }
                }
            }
        }
        b0 = s_b0_q15[best_r][best_v][best_o][best_i];
    }

    return (b0 != 0) ? b0 : 128;
}
