#ifndef STORAGE_LAYOUT_H
#define STORAGE_LAYOUT_H

#include "../Param/param_backend.h"

typedef struct
{
    ParamStorageID_t id;
    uint32_t region_a;
    uint32_t region_b;
    uint32_t region_size;
} ParamStorageLayout_t;

ParamStatus_t StorageLayout_Register(const ParamStorageLayout_t *layout);
const ParamStorageLayout_t *StorageLayout_Get(ParamStorageID_t id);
ParamStatus_t StorageLayout_Validate(const ParamStorageLayout_t *layout,
                                     const ParamStorageBackend_t *backend);
void StorageLayout_Clear(void);

#endif
