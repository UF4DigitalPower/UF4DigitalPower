#include "param_uf4com.h"

#include "uf4com_id_table.h"

#include <limits.h>
#include <string.h>

static uf4_id_t s_uf4_ids[PARAM_UF4COM_MAX_IDS];

static uf4_error_t status_to_uf4(ParamStatus_t status)
{
    switch (status)
    {
        case PARAM_OK: return UF4_OK;
        case PARAM_ERROR_NOT_FOUND: return UF4_ERR_INVALID_ID;
        case PARAM_ERROR_ACCESS: return UF4_ERR_WRITE_PROTECT;
        case PARAM_ERROR_RANGE:
        case PARAM_ERROR_TYPE:
        case PARAM_ERROR_VALIDATION:
        case PARAM_ERROR_ARGUMENT: return UF4_ERR_OUT_OF_RANGE;
        default: return UF4_ERR_INTERNAL;
    }
}

static bool default_encode(const ParamDescriptor_t *descriptor,
                           const void *value, uint16_t *wire_value)
{
    switch (descriptor->type)
    {
        case PARAM_TYPE_U8: *wire_value = *(const uint8_t *)value; return true;
        case PARAM_TYPE_U16: *wire_value = *(const uint16_t *)value; return true;
        case PARAM_TYPE_I8:
            *wire_value = (uint16_t)(int16_t)*(const int8_t *)value; return true;
        case PARAM_TYPE_I16:
            *wire_value = (uint16_t)*(const int16_t *)value; return true;
        case PARAM_TYPE_BOOL: *wire_value = *(const uint8_t *)value; return true;
        case PARAM_TYPE_U32:
            if (*(const uint32_t *)value > UINT16_MAX) return false;
            *wire_value = (uint16_t)*(const uint32_t *)value; return true;
        case PARAM_TYPE_I32:
        case PARAM_TYPE_ENUM:
            if ((*(const int32_t *)value < INT16_MIN) ||
                (*(const int32_t *)value > UINT16_MAX)) return false;
            *wire_value = (uint16_t)*(const int32_t *)value; return true;
        default: return false;
    }
}

static bool default_decode(const ParamDescriptor_t *descriptor,
                           uint16_t wire_value, void *value, size_t size)
{
    (void)size;
    switch (descriptor->type)
    {
        case PARAM_TYPE_U8:
            if (wire_value > UINT8_MAX) return false;
            *(uint8_t *)value = (uint8_t)wire_value; return true;
        case PARAM_TYPE_U16: *(uint16_t *)value = wire_value; return true;
        case PARAM_TYPE_I8:
            if ((wire_value > INT8_MAX) && (wire_value < (uint16_t)(int16_t)INT8_MIN))
                return false;
            *(int8_t *)value = (int8_t)wire_value; return true;
        case PARAM_TYPE_I16: *(int16_t *)value = (int16_t)wire_value; return true;
        case PARAM_TYPE_BOOL:
            if (wire_value > 1u) return false;
            *(uint8_t *)value = (uint8_t)wire_value; return true;
        case PARAM_TYPE_U32: *(uint32_t *)value = wire_value; return true;
        case PARAM_TYPE_I32:
        case PARAM_TYPE_ENUM: *(int32_t *)value = (int16_t)wire_value; return true;
        default: return false;
    }
}

static bool decode_value(const ParamDescriptor_t *descriptor,
                         uint16_t wire_value, uint32_t *value)
{
    memset(value, 0, sizeof(*value));
    if (descriptor->wire_decode != NULL)
        return descriptor->wire_decode(descriptor, wire_value, value,
                                       descriptor->size);
    return default_decode(descriptor, wire_value, value, descriptor->size);
}

static uf4_error_t read_object(const uf4_id_t *item, uint16_t *wire_value)
{
    const ParamDescriptor_t *descriptor = item->context;
    uint32_t value = 0u;
    ParamStatus_t status = Param_Get(descriptor->id, &value, descriptor->size);
    bool encoded;
    if (status != PARAM_OK) return status_to_uf4(status);
    encoded = (descriptor->wire_encode != NULL) ?
              descriptor->wire_encode(descriptor, &value, wire_value) :
              default_encode(descriptor, &value, wire_value);
    return encoded ? UF4_OK : UF4_ERR_OUT_OF_RANGE;
}

static uf4_error_t validate_object_write(const uf4_id_t *item,
                                         uint16_t wire_value)
{
    const ParamDescriptor_t *descriptor = item->context;
    uint32_t value;
    if (!decode_value(descriptor, wire_value, &value)) return UF4_ERR_OUT_OF_RANGE;
    return status_to_uf4(Param_ValidateSet(descriptor->id, &value,
                                           descriptor->size));
}

static uf4_error_t write_object(const uf4_id_t *item, uint16_t wire_value)
{
    const ParamDescriptor_t *descriptor = item->context;
    uint32_t value;
    if (!decode_value(descriptor, wire_value, &value)) return UF4_ERR_OUT_OF_RANGE;
    return status_to_uf4(Param_Set(descriptor->id, &value, descriptor->size));
}

ParamStatus_t ParamUF4COM_Bind(void)
{
    size_t i;
    size_t count = ParamTable_Count();
    const ParamDescriptor_t *descriptor;
    uf4_id_t *item;
    uint32_t probe;
    uint16_t wire_probe;

    if ((count == 0u) || (count > PARAM_UF4COM_MAX_IDS)) return PARAM_ERROR_NO_SPACE;
    memset(s_uf4_ids, 0, sizeof(s_uf4_ids));
    for (i = 0u; i < count; ++i)
    {
        descriptor = ParamTable_At(i);
        if (descriptor->id > UINT8_MAX) return PARAM_ERROR_ARGUMENT;
        probe = 0u;
        memcpy(&probe, descriptor->runtime, descriptor->size);
        if ((descriptor->wire_encode == NULL) &&
            !default_encode(descriptor, &probe, &wire_probe)) return PARAM_ERROR_TYPE;
        if (((descriptor->flags & PARAM_FLAG_WRITE) != 0u) &&
            (descriptor->wire_decode == NULL) &&
            (descriptor->type == PARAM_TYPE_FLOAT)) return PARAM_ERROR_TYPE;

        item = &s_uf4_ids[i];
        item->id = (uint8_t)descriptor->id;
        item->rw = ((descriptor->flags & PARAM_FLAG_WRITE) != 0u) ?
                   UF4_ID_PERMISSION_RW : UF4_ID_PERMISSION_RO;
        item->read = read_object;
        item->validate_write = validate_object_write;
        item->write = write_object;
        item->context = (void *)descriptor;
    }
    UF4_IDTableBind(s_uf4_ids, count);
    return PARAM_OK;
}
