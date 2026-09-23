#include "storage_ab.h"

#include "../Param/param_crc.h"

#include <stddef.h>
#include <string.h>

typedef struct
{
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t sequence;
    uint32_t data_size;
    uint32_t data_crc;
    uint32_t header_crc;
    uint64_t valid_marker;
} StorageABHeader_t;

static uint32_t header_crc(const StorageABHeader_t *header)
{
    return ParamCRC32(header, offsetof(StorageABHeader_t, header_crc));
}

static bool sequence_newer(uint32_t lhs, uint32_t rhs)
{
    return (int32_t)(lhs - rhs) > 0;
}

static bool read_info(const ParamStorageLayout_t *layout, uint32_t address,
                      StorageABInfo_t *info)
{
    const ParamStorageBackend_t *backend = ParamBackend_Get(layout->id);
    StorageABHeader_t header;
    uint8_t chunk[64];
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t offset = 0u;
    size_t count;

    memset(info, 0, sizeof(*info));
    info->address = address;
    if (!backend->read(backend->context, address, &header, sizeof(header))) return false;
    if ((header.magic != STORAGE_AB_MAGIC) ||
        (header.format_version != STORAGE_AB_FORMAT_VERSION) ||
        (header.header_size != sizeof(header)) ||
        (header.valid_marker != STORAGE_AB_VALID_MARKER) ||
        (header.header_crc != header_crc(&header)) ||
        (header.data_size > (layout->region_size - sizeof(header)))) return true;

    while (offset < header.data_size)
    {
        count = header.data_size - offset;
        if (count > sizeof(chunk)) count = sizeof(chunk);
        if (!backend->read(backend->context, address + sizeof(header) + offset,
                           chunk, count)) return false;
        crc = ParamCRC32_Update(crc, chunk, count);
        offset += (uint32_t)count;
    }
    crc ^= 0xFFFFFFFFu;
    if (crc != header.data_crc) return true;

    info->sequence = header.sequence;
    info->data_size = header.data_size;
    info->data_crc = header.data_crc;
    info->valid = true;
    return true;
}

static ParamStatus_t inspect_pair(const ParamStorageLayout_t *layout,
                                  StorageABInfo_t *a, StorageABInfo_t *b,
                                  StorageABInfo_t *active)
{
    if (!read_info(layout, layout->region_a, a) ||
        !read_info(layout, layout->region_b, b)) return PARAM_ERROR_STORAGE;
    memset(active, 0, sizeof(*active));
    if (a->valid && b->valid) *active = sequence_newer(a->sequence, b->sequence) ? *a : *b;
    else if (a->valid) *active = *a;
    else if (b->valid) *active = *b;
    return active->valid ? PARAM_OK : PARAM_ERROR_NOT_FOUND;
}

ParamStatus_t StorageAB_Inspect(const ParamStorageLayout_t *layout,
                                StorageABInfo_t *active)
{
    StorageABInfo_t a;
    StorageABInfo_t b;
    if ((layout == NULL) || (active == NULL)) return PARAM_ERROR_ARGUMENT;
    return inspect_pair(layout, &a, &b, active);
}

ParamStatus_t StorageAB_Load(const ParamStorageLayout_t *layout,
                             void *data, size_t capacity, size_t *data_size,
                             StorageABInfo_t *info)
{
    const ParamStorageBackend_t *backend;
    StorageABInfo_t a;
    StorageABInfo_t b;
    ParamStatus_t status;
    if ((layout == NULL) || (data == NULL) || (data_size == NULL))
        return PARAM_ERROR_ARGUMENT;
    status = inspect_pair(layout, &a, &b, info);
    if (status != PARAM_OK) return status;
    if (info->data_size > capacity) return PARAM_ERROR_NO_SPACE;
    backend = ParamBackend_Get(layout->id);
    if (!backend->read(backend->context,
                       info->address + sizeof(StorageABHeader_t),
                       data, info->data_size)) return PARAM_ERROR_STORAGE;
    *data_size = info->data_size;
    return PARAM_OK;
}

ParamStatus_t StorageAB_Commit(const ParamStorageLayout_t *layout,
                               const void *data, size_t data_size,
                               StorageABInfo_t *info)
{
    const ParamStorageBackend_t *backend;
    StorageABInfo_t a;
    StorageABInfo_t b;
    StorageABInfo_t active;
    StorageABInfo_t verified;
    StorageABHeader_t header;
    uint32_t target;
    uint64_t marker = STORAGE_AB_VALID_MARKER;
    ParamStatus_t status;

    if ((layout == NULL) || (data == NULL) || (info == NULL))
        return PARAM_ERROR_ARGUMENT;
    if (data_size > (layout->region_size - sizeof(header))) return PARAM_ERROR_NO_SPACE;
    backend = ParamBackend_Get(layout->id);
    status = inspect_pair(layout, &a, &b, &active);
    if ((status != PARAM_OK) && (status != PARAM_ERROR_NOT_FOUND)) return status;
    target = !active.valid ? layout->region_a :
             ((active.address == layout->region_a) ? layout->region_b : layout->region_a);

    memset(&header, 0xFF, sizeof(header));
    header.magic = STORAGE_AB_MAGIC;
    header.format_version = STORAGE_AB_FORMAT_VERSION;
    header.header_size = sizeof(header);
    header.sequence = active.valid ? active.sequence + 1u : 1u;
    header.data_size = (uint32_t)data_size;
    header.data_crc = ParamCRC32(data, data_size);
    header.header_crc = header_crc(&header);

    if (!backend->erase(backend->context, target, layout->region_size) ||
        !backend->write(backend->context, target, &header,
                        offsetof(StorageABHeader_t, valid_marker)) ||
        !backend->write(backend->context, target + sizeof(header), data, data_size) ||
        !backend->write(backend->context,
                        target + offsetof(StorageABHeader_t, valid_marker),
                        &marker, sizeof(marker))) return PARAM_ERROR_STORAGE;
    if (!read_info(layout, target, &verified) || !verified.valid ||
        (verified.sequence != header.sequence)) return PARAM_ERROR_STORAGE;
    *info = verified;
    return PARAM_OK;
}
