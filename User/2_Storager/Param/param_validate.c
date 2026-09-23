#include "param_validate.h"

#include <math.h>

size_t ParamType_Size(ParamType_t type)
{
    switch (type)
    {
        case PARAM_TYPE_U8: case PARAM_TYPE_I8: case PARAM_TYPE_BOOL: return 1u;
        case PARAM_TYPE_U16: case PARAM_TYPE_I16: return 2u;
        case PARAM_TYPE_U32: case PARAM_TYPE_I32:
        case PARAM_TYPE_FLOAT: case PARAM_TYPE_ENUM: return 4u;
        default: return 0u;
    }
}

static bool value_as_double(const ParamDescriptor_t *d, const void *value,
                            double *number)
{
    switch (d->type)
    {
        case PARAM_TYPE_U8: *number = *(const uint8_t *)value; break;
        case PARAM_TYPE_U16: *number = *(const uint16_t *)value; break;
        case PARAM_TYPE_U32: *number = *(const uint32_t *)value; break;
        case PARAM_TYPE_I8: *number = *(const int8_t *)value; break;
        case PARAM_TYPE_I16: *number = *(const int16_t *)value; break;
        case PARAM_TYPE_I32: *number = *(const int32_t *)value; break;
        case PARAM_TYPE_FLOAT:
            *number = *(const float *)value;
            if (!isfinite(*number)) return false;
            break;
        case PARAM_TYPE_BOOL:
            if (*(const uint8_t *)value > 1u) return false;
            *number = *(const uint8_t *)value;
            break;
        case PARAM_TYPE_ENUM: *number = *(const int32_t *)value; break;
        default: return false;
    }
    return true;
}

ParamStatus_t ParamValidate_Descriptor(const ParamDescriptor_t *d)
{
    if ((d == NULL) || (d->runtime == NULL) || (d->name == NULL) ||
        (d->size != ParamType_Size(d->type)) ||
        (d->min_value > d->max_value) ||
        (((d->flags & PARAM_FLAG_PERSISTENT) != 0u) &&
         (d->storage >= PARAM_STORAGE_COUNT))) return PARAM_ERROR_ARGUMENT;
    return PARAM_OK;
}

ParamStatus_t ParamValidate_ValueBasic(const ParamDescriptor_t *d,
                                       const void *value, size_t size,
                                       bool check_write_access)
{
    double number;
    if ((d == NULL) || (value == NULL) || (size != d->size))
        return PARAM_ERROR_ARGUMENT;
    if (check_write_access && ((d->flags & PARAM_FLAG_WRITE) == 0u))
        return PARAM_ERROR_ACCESS;
    if (!value_as_double(d, value, &number)) return PARAM_ERROR_TYPE;
    if ((number < d->min_value) || (number > d->max_value))
        return PARAM_ERROR_RANGE;
    return PARAM_OK;
}

ParamStatus_t ParamValidate_Value(const ParamDescriptor_t *d,
                                  const void *value, size_t size,
                                  bool check_write_access)
{
    ParamStatus_t status = ParamValidate_ValueBasic(d, value, size,
                                                     check_write_access);
    if (status != PARAM_OK) return status;
    if ((d->validator != NULL) && !d->validator(d, value, size))
        return PARAM_ERROR_VALIDATION;
    return PARAM_OK;
}

ParamStatus_t ParamValidate_MakeDefault(const ParamDescriptor_t *d,
                                        void *value, size_t size)
{
    if ((d == NULL) || (value == NULL) || (size != d->size))
        return PARAM_ERROR_ARGUMENT;
    switch (d->type)
    {
        case PARAM_TYPE_U8: *(uint8_t *)value = (uint8_t)d->default_value; break;
        case PARAM_TYPE_U16: *(uint16_t *)value = (uint16_t)d->default_value; break;
        case PARAM_TYPE_U32: *(uint32_t *)value = (uint32_t)d->default_value; break;
        case PARAM_TYPE_I8: *(int8_t *)value = (int8_t)d->default_value; break;
        case PARAM_TYPE_I16: *(int16_t *)value = (int16_t)d->default_value; break;
        case PARAM_TYPE_I32:
        case PARAM_TYPE_ENUM: *(int32_t *)value = (int32_t)d->default_value; break;
        case PARAM_TYPE_FLOAT: *(float *)value = d->default_value; break;
        case PARAM_TYPE_BOOL: *(uint8_t *)value = d->default_value != 0.0f; break;
        default: return PARAM_ERROR_TYPE;
    }
    return ParamValidate_ValueBasic(d, value, size, false);
}
