#include "param_backend.h"

#include <string.h>

static const ParamStorageBackend_t *s_backends[PARAM_STORAGE_COUNT];

ParamStatus_t ParamBackend_Register(ParamStorageID_t id,
                                    const ParamStorageBackend_t *backend)
{
    if ((id >= PARAM_STORAGE_COUNT) || (backend == NULL) ||
        (backend->read == NULL) || (backend->write == NULL) ||
        (backend->erase == NULL) || (backend->erase_size == 0u) ||
        (backend->write_size == 0u)) return PARAM_ERROR_ARGUMENT;
    s_backends[id] = backend;
    return PARAM_OK;
}

const ParamStorageBackend_t *ParamBackend_Get(ParamStorageID_t id)
{
    return (id < PARAM_STORAGE_COUNT) ? s_backends[id] : NULL;
}

ParamStatus_t ParamBackend_InitAll(void)
{
    size_t i;
    for (i = 0u; i < PARAM_STORAGE_COUNT; ++i)
    {
        if ((s_backends[i] != NULL) && (s_backends[i]->init != NULL) &&
            !s_backends[i]->init(s_backends[i]->context))
            return PARAM_ERROR_STORAGE;
    }
    return PARAM_OK;
}

void ParamBackend_Clear(void)
{
    memset(s_backends, 0, sizeof(s_backends));
}
