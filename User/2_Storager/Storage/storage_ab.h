#ifndef STORAGE_AB_H
#define STORAGE_AB_H

#include "storage_layout.h"

#define STORAGE_AB_MAGIC          0x55463450u
#define STORAGE_AB_FORMAT_VERSION 1u
#define STORAGE_AB_VALID_MARKER   0xA55A3CC396690FF0ull

typedef struct
{
    uint32_t sequence;
    uint32_t data_size;
    uint32_t data_crc;
    uint32_t address;
    bool valid;
} StorageABInfo_t;

ParamStatus_t StorageAB_Load(const ParamStorageLayout_t *layout,
                             void *data, size_t capacity, size_t *data_size,
                             StorageABInfo_t *info);
ParamStatus_t StorageAB_Commit(const ParamStorageLayout_t *layout,
                               const void *data, size_t data_size,
                               StorageABInfo_t *info);
ParamStatus_t StorageAB_Inspect(const ParamStorageLayout_t *layout,
                                StorageABInfo_t *active);

#endif
