#ifndef PARAM_TYPES_H
#define PARAM_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ParamID_t;

typedef enum
{
    PARAM_TYPE_U8 = 0,
    PARAM_TYPE_U16,
    PARAM_TYPE_U32,
    PARAM_TYPE_I8,
    PARAM_TYPE_I16,
    PARAM_TYPE_I32,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_BOOL, PARAM_TYPE_ENUM
} ParamType_t;  // 类型

typedef enum
{
    PARAM_STORAGE_INTERNAL = 0,
    PARAM_STORAGE_W25Q64,
    PARAM_STORAGE_W25Q128,
    PARAM_STORAGE_COUNT
} ParamStorageID_t; // W25QXX

typedef enum
{
    PARAM_GROUP_SYSTEM = 0,
    PARAM_GROUP_OUTPUT,
    PARAM_GROUP_PROTECTION,
    PARAM_GROUP_CALIBRATION,
    PARAM_GROUP_PID,
    PARAM_GROUP_COMMUNICATION
} ParamGroup_t;  // 组

enum
{
    PARAM_FLAG_READ       = (1u << 0),
    PARAM_FLAG_WRITE      = (1u << 1),
    PARAM_FLAG_PERSISTENT = (1u << 2),
    PARAM_FLAG_REBOOT     = (1u << 3),
    PARAM_FLAG_RUNTIME    = (1u << 4),
    PARAM_FLAG_HIDDEN     = (1u << 5),
    PARAM_FLAG_FACTORY    = (1u << 6)
};

typedef enum
{
    PARAM_OK = 0,
    PARAM_ERROR_ARGUMENT,
    PARAM_ERROR_NOT_INITIALIZED,
    PARAM_ERROR_NOT_FOUND,
    PARAM_ERROR_ACCESS,
    PARAM_ERROR_TYPE,
    PARAM_ERROR_RANGE,
    PARAM_ERROR_VALIDATION,
    PARAM_ERROR_STORAGE,
    PARAM_ERROR_FORMAT,
    PARAM_ERROR_NO_SPACE
} ParamStatus_t;  // 状态

struct ParamDescriptor;
typedef bool (*ParamValidator_t)(const struct ParamDescriptor *descriptor,
                                 const void *value, size_t size);
typedef bool (*ParamWireEncode_t)(const struct ParamDescriptor *descriptor,
                                  const void *value, uint16_t *wire_value);
typedef bool (*ParamWireDecode_t)(const struct ParamDescriptor *descriptor,
                                  uint16_t wire_value, void *value, size_t size);

typedef struct ParamDescriptor
{
    ParamID_t id;
    const char *name;
    ParamType_t type;
    ParamGroup_t group;
    uint16_t flags;
    ParamStorageID_t storage;
    float min_value;
    float max_value;
    float default_value;
    uint8_t size;
    void *runtime;
    ParamValidator_t validator;
    ParamWireEncode_t wire_encode;
    ParamWireDecode_t wire_decode;
} ParamDescriptor_t;

#ifdef __cplusplus
}
#endif
#endif
