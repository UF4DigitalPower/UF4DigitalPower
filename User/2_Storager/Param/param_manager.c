#include "param_manager.h"

#include "param_backend.h"
#include "param_validate.h"
#include "../Storage/storage_ab.h"

#include <string.h>

#define RECORD_HEADER_SIZE 4u

static uint8_t s_snapshot[PARAM_SNAPSHOT_CAPACITY];
static uint32_t s_dirty_mask;
static bool s_initialized;

static uint32_t storage_bit(ParamStorageID_t storage)
{
    return 1u << (uint32_t)storage;
}

static void mark_dirty(const ParamDescriptor_t *descriptor)
{
    if ((descriptor->flags & PARAM_FLAG_PERSISTENT) != 0u)
        s_dirty_mask |= storage_bit(descriptor->storage);
}

static ParamStatus_t set_default(const ParamDescriptor_t *descriptor,
                                 bool dirty)
{
    uint8_t value[sizeof(uint32_t)];
    ParamStatus_t status;
    if (descriptor->size > sizeof(value)) return PARAM_ERROR_TYPE;
    status = ParamValidate_MakeDefault(descriptor, value, descriptor->size);
    if (status != PARAM_OK) return status;
    status = ParamValidate_Value(descriptor, value, descriptor->size, false);
    if (status != PARAM_OK) return status;
    memcpy(descriptor->runtime, value, descriptor->size);
    if (dirty) mark_dirty(descriptor);
    return PARAM_OK;
}

ParamStatus_t Param_Init(const ParamDescriptor_t *table, size_t count)
{
    size_t i;
    ParamStatus_t status = ParamTable_Bind(table, count);
    if (status != PARAM_OK) return status;
    for (i = 0u; i < count; ++i)
    {
        status = ParamValidate_Descriptor(&table[i]);
        if (status != PARAM_OK) return status;
    }
    status = ParamBackend_InitAll();
    if (status != PARAM_OK) return status;
    s_dirty_mask = 0u;
    s_initialized = true;
    status = Param_ResetAll();
    s_dirty_mask = 0u;
    return status;
}

const ParamDescriptor_t *Param_GetDescriptor(ParamID_t id)
{
    return ParamTable_Find(id);
}

ParamStatus_t Param_Get(ParamID_t id, void *value, size_t size)
{
    const ParamDescriptor_t *descriptor;
    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;
    descriptor = ParamTable_Find(id);
    if (descriptor == NULL) return PARAM_ERROR_NOT_FOUND;
    if ((descriptor->flags & PARAM_FLAG_READ) == 0u) return PARAM_ERROR_ACCESS;
    if ((value == NULL) || (size != descriptor->size)) return PARAM_ERROR_ARGUMENT;
    memcpy(value, descriptor->runtime, size);
    return PARAM_OK;
}

ParamStatus_t Param_Set(ParamID_t id, const void *value, size_t size)
{
    const ParamDescriptor_t *descriptor;
    ParamStatus_t status;
    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;
    descriptor = ParamTable_Find(id);
    if (descriptor == NULL) return PARAM_ERROR_NOT_FOUND;
    status = ParamValidate_Value(descriptor, value, size, true);
    if (status != PARAM_OK) return status;
    if (memcmp(descriptor->runtime, value, size) != 0)
    {
        memcpy(descriptor->runtime, value, size);
        mark_dirty(descriptor);
    }
    return PARAM_OK;
}

ParamStatus_t Param_GetByName(const char *name, void *value, size_t size)
{
    const ParamDescriptor_t *descriptor = ParamTable_FindByName(name);
    return (descriptor == NULL) ? PARAM_ERROR_NOT_FOUND :
           Param_Get(descriptor->id, value, size);
}

ParamStatus_t Param_SetByName(const char *name, const void *value, size_t size)
{
    const ParamDescriptor_t *descriptor = ParamTable_FindByName(name);
    return (descriptor == NULL) ? PARAM_ERROR_NOT_FOUND :
           Param_Set(descriptor->id, value, size);
}

ParamStatus_t Param_Reset(ParamID_t id)
{
    const ParamDescriptor_t *descriptor;
    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;
    descriptor = ParamTable_Find(id);
    if (descriptor == NULL) return PARAM_ERROR_NOT_FOUND;
    if ((descriptor->flags & PARAM_FLAG_WRITE) == 0u) return PARAM_ERROR_ACCESS;
    return set_default(descriptor, true);
}

ParamStatus_t Param_ResetGroup(ParamGroup_t group)
{
    size_t i;
    ParamStatus_t status;
    const ParamDescriptor_t *descriptor;
    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;
    for (i = 0u; i < ParamTable_Count(); ++i)
    {
        descriptor = ParamTable_At(i);
        if ((descriptor->group == group) &&
            ((descriptor->flags & PARAM_FLAG_WRITE) != 0u))
        {
            status = set_default(descriptor, true);
            if (status != PARAM_OK) return status;
        }
    }
    return PARAM_OK;
}

ParamStatus_t Param_ResetAll(void)
{
    size_t i;
    ParamStatus_t status;
    const ParamDescriptor_t *descriptor;
    uint8_t value[sizeof(uint32_t)];
    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;

    /* Install all defaults first so cross-parameter validators see a coherent set. */
    for (i = 0u; i < ParamTable_Count(); ++i)
    {
        descriptor = ParamTable_At(i);
        status = ParamValidate_MakeDefault(descriptor, value, descriptor->size);
        if (status != PARAM_OK) return status;
        memcpy(descriptor->runtime, value, descriptor->size);
        mark_dirty(descriptor);
    }
    for (i = 0u; i < ParamTable_Count(); ++i)
    {
        descriptor = ParamTable_At(i);
        status = ParamValidate_Value(descriptor, descriptor->runtime,
                                     descriptor->size, false);
        if (status != PARAM_OK) return status;
    }
    return PARAM_OK;
}

bool Param_IsDirty(void) { return s_dirty_mask != 0u; }

bool Param_IsStorageDirty(ParamStorageID_t storage)
{
    return (storage < PARAM_STORAGE_COUNT) &&
           ((s_dirty_mask & storage_bit(storage)) != 0u);
}

static bool storage_has_parameters(ParamStorageID_t storage)
{
    size_t i;
    const ParamDescriptor_t *descriptor;
    for (i = 0u; i < ParamTable_Count(); ++i)
    {
        descriptor = ParamTable_At(i);
        if (((descriptor->flags & PARAM_FLAG_PERSISTENT) != 0u) &&
            (descriptor->storage == storage)) return true;
    }
    return false;
}

static ParamStatus_t make_snapshot(ParamStorageID_t storage, size_t *length)
{
    size_t i;
    size_t offset = 0u;
    const ParamDescriptor_t *descriptor;
    for (i = 0u; i < ParamTable_Count(); ++i)
    {
        descriptor = ParamTable_At(i);
        if (((descriptor->flags & PARAM_FLAG_PERSISTENT) == 0u) ||
            (descriptor->storage != storage)) continue;
        if ((offset + RECORD_HEADER_SIZE + descriptor->size) > sizeof(s_snapshot))
            return PARAM_ERROR_NO_SPACE;
        s_snapshot[offset] = (uint8_t)(descriptor->id & 0xFFu);
        s_snapshot[offset + 1u] = (uint8_t)(descriptor->id >> 8u);
        s_snapshot[offset + 2u] = (uint8_t)descriptor->type;
        s_snapshot[offset + 3u] = descriptor->size;
        memcpy(&s_snapshot[offset + RECORD_HEADER_SIZE], descriptor->runtime,
               descriptor->size);
        offset += RECORD_HEADER_SIZE + descriptor->size;
    }
    *length = offset;
    return PARAM_OK;
}

static ParamStatus_t validate_snapshot(ParamStorageID_t storage, size_t length)
{
    size_t offset = 0u;
    ParamID_t id;
    uint8_t type;
    uint8_t size;
    const ParamDescriptor_t *descriptor;
    ParamStatus_t status;

    while (offset < length)
    {
        if ((length - offset) < RECORD_HEADER_SIZE) return PARAM_ERROR_FORMAT;
        id = (ParamID_t)s_snapshot[offset] |
             ((ParamID_t)s_snapshot[offset + 1u] << 8u);
        type = s_snapshot[offset + 2u];
        size = s_snapshot[offset + 3u];
        offset += RECORD_HEADER_SIZE;
        if ((size == 0u) || ((length - offset) < size)) return PARAM_ERROR_FORMAT;
        descriptor = ParamTable_Find(id);
        if (descriptor != NULL)
        {
            if (((descriptor->flags & PARAM_FLAG_PERSISTENT) == 0u) ||
                (descriptor->storage != storage) ||
                ((uint8_t)descriptor->type != type) || (descriptor->size != size))
                return PARAM_ERROR_FORMAT;
            status = ParamValidate_ValueBasic(descriptor, &s_snapshot[offset],
                                              size, false);
            if (status != PARAM_OK) return status;
        }
        offset += size;
    }
    return PARAM_OK;
}

static void apply_snapshot(ParamStorageID_t storage, size_t length)
{
    size_t offset = 0u;
    ParamID_t id;
    uint8_t size;
    const ParamDescriptor_t *descriptor;
    (void)storage;
    while (offset < length)
    {
        id = (ParamID_t)s_snapshot[offset] |
             ((ParamID_t)s_snapshot[offset + 1u] << 8u);
        size = s_snapshot[offset + 3u];
        offset += RECORD_HEADER_SIZE;
        descriptor = ParamTable_Find(id);
        if (descriptor != NULL) memcpy(descriptor->runtime, &s_snapshot[offset], size);
        offset += size;
    }
}

static ParamStatus_t validate_loaded_storage(ParamStorageID_t storage)
{
    size_t i;
    const ParamDescriptor_t *descriptor;
    ParamStatus_t status;
    for (i = 0u; i < ParamTable_Count(); ++i)
    {
        descriptor = ParamTable_At(i);
        if (((descriptor->flags & PARAM_FLAG_PERSISTENT) == 0u) ||
            (descriptor->storage != storage)) continue;
        status = ParamValidate_Value(descriptor, descriptor->runtime,
                                     descriptor->size, false);
        if (status != PARAM_OK) return status;
    }
    return PARAM_OK;
}

ParamStatus_t Param_Load(void)
{
    ParamStorageID_t storage;
    const ParamStorageLayout_t *layout;
    StorageABInfo_t info;
    size_t length;
    ParamStatus_t status;
    bool missing = false;

    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;
    status = Param_ResetAll();
    if (status != PARAM_OK) return status;

    for (storage = PARAM_STORAGE_INTERNAL; storage < PARAM_STORAGE_COUNT; ++storage)
    {
        if (!storage_has_parameters(storage)) continue;
        layout = StorageLayout_Get(storage);
        if (layout == NULL) return PARAM_ERROR_STORAGE;
        status = StorageAB_Load(layout, s_snapshot, sizeof(s_snapshot), &length, &info);
        if (status == PARAM_ERROR_NOT_FOUND)
        {
            missing = true;
            continue;
        }
        if (status != PARAM_OK) return status;
        status = validate_snapshot(storage, length);
        if (status != PARAM_OK) return status;
        apply_snapshot(storage, length);
        status = validate_loaded_storage(storage);
        if (status != PARAM_OK)
        {
            (void)Param_ResetAll();
            return status;
        }
        s_dirty_mask &= ~storage_bit(storage);
    }
    return missing ? PARAM_ERROR_NOT_FOUND : PARAM_OK;
}

ParamStatus_t Param_Commit(void)
{
    ParamStorageID_t storage;
    const ParamStorageLayout_t *layout;
    StorageABInfo_t info;
    size_t length;
    ParamStatus_t status;
    if (!s_initialized) return PARAM_ERROR_NOT_INITIALIZED;

    for (storage = PARAM_STORAGE_INTERNAL; storage < PARAM_STORAGE_COUNT; ++storage)
    {
        if (!Param_IsStorageDirty(storage)) continue;
        layout = StorageLayout_Get(storage);
        if (layout == NULL) return PARAM_ERROR_STORAGE;
        status = make_snapshot(storage, &length);
        if (status != PARAM_OK) return status;
        status = StorageAB_Commit(layout, s_snapshot, length, &info);
        if (status != PARAM_OK) return status;
        s_dirty_mask &= ~storage_bit(storage);
    }
    return PARAM_OK;
}

ParamStatus_t Param_Rollback(void)
{
    return Param_Load();
}
