#ifndef FLASH_INIT_H
#define FLASH_INIT_H

#include "../Param/param_backend.h"
#include "W25QXX.h"

#define PARAM_INTERNAL_FLASH_SIZE     0x00020000u
#define PARAM_INTERNAL_REGION_A       0x0001E000u
#define PARAM_INTERNAL_REGION_B       0x0001F000u
#define PARAM_INTERNAL_REGION_SIZE    0x00001000u

extern const ParamStorageBackend_t g_internal_flash_backend;

ParamStatus_t Flash_Init_RegisterInternal(void);
ParamStatus_t Flash_Init_RegisterW25QXX(ParamStorageID_t storage,
                                        w25qxx_handle_t *handle,
                                        uint32_t total_size,
                                        uint32_t region_a,
                                        uint32_t region_b,
                                        uint32_t region_size);

#endif
