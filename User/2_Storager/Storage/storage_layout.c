#include "storage_layout.h"

#include <string.h>

static ParamStorageLayout_t s_layouts[PARAM_STORAGE_COUNT];
static bool s_registered[PARAM_STORAGE_COUNT];

ParamStatus_t StorageLayout_Validate(const ParamStorageLayout_t *layout,
                                     const ParamStorageBackend_t *backend)
{
    uint64_t a_end;
    uint64_t b_end;
    if ((layout == NULL) || (backend == NULL) ||
        (layout->id >= PARAM_STORAGE_COUNT) || (layout->region_size == 0u) ||
        ((layout->region_a % backend->erase_size) != 0u) ||
        ((layout->region_b % backend->erase_size) != 0u) ||
        ((layout->region_size % backend->erase_size) != 0u))
        return PARAM_ERROR_ARGUMENT;
    a_end = (uint64_t)layout->region_a + layout->region_size;
    b_end = (uint64_t)layout->region_b + layout->region_size;
    if ((a_end > backend->total_size) || (b_end > backend->total_size) ||
        !((a_end <= layout->region_b) || (b_end <= layout->region_a)))
        return PARAM_ERROR_ARGUMENT;
    return PARAM_OK;
}

ParamStatus_t StorageLayout_Register(const ParamStorageLayout_t *layout)
{
    const ParamStorageBackend_t *backend;
    ParamStatus_t status;
    if (layout == NULL) return PARAM_ERROR_ARGUMENT;
    backend = ParamBackend_Get(layout->id);
    status = StorageLayout_Validate(layout, backend);
    if (status != PARAM_OK) return status;
    s_layouts[layout->id] = *layout;
    s_registered[layout->id] = true;
    return PARAM_OK;
}

const ParamStorageLayout_t *StorageLayout_Get(ParamStorageID_t id)
{
    return ((id < PARAM_STORAGE_COUNT) && s_registered[id]) ? &s_layouts[id] : NULL;
}

void StorageLayout_Clear(void)
{
    memset(s_layouts, 0, sizeof(s_layouts));
    memset(s_registered, 0, sizeof(s_registered));
}
