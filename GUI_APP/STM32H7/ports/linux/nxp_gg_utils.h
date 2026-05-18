/*
 * SPDX-License-Identifier: MIT
 * Copyright 2025 NXP
 */

#ifndef NXP_GG_UTILS
#define NXP_GG_UTILS

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR` */
uint32_t custom_tick_get(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NXP_GG_UTILS */
