#ifndef PARAM_BACKEND_H
#define PARAM_BACKEND_H

#include "param_types.h"

typedef struct
{
    bool (*init)(void *context);
    bool (*read)(void *context, uint32_t address, void *data, size_t length);
    bool (*write)(void *context, uint32_t address, const void *data, size_t length);
    bool (*erase)(void *context, uint32_t address, size_t length);
    void *context;
    uint32_t total_size;
    uint32_t erase_size;
    uint32_t write_size;
} ParamStorageBackend_t;

ParamStatus_t ParamBackend_Register(ParamStorageID_t id,
                                    const ParamStorageBackend_t *backend);
const ParamStorageBackend_t *ParamBackend_Get(ParamStorageID_t id);
ParamStatus_t ParamBackend_InitAll(void);
void ParamBackend_Clear(void);

#endif
