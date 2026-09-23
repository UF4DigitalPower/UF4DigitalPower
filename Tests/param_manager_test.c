#include "../User/2_Storager/Param/param_manager.h"
#include "../User/2_Storager/Param/param_backend.h"
#include "../User/2_Storager/Param/param_crc.h"
#include "../User/2_Storager/Storage/storage_layout.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MOCK_SIZE 512u
#define MOCK_REGION_SIZE 256u

static uint8_t g_flash[MOCK_SIZE];
static float g_voltage;
static uint16_t g_current;

static bool mock_read(void *context, uint32_t address, void *data, size_t length)
{
    (void)context;
    if (((size_t)address + length) > sizeof(g_flash)) return false;
    memcpy(data, &g_flash[address], length);
    return true;
}

static bool mock_write(void *context, uint32_t address,
                       const void *data, size_t length)
{
    const uint8_t *source = data;
    size_t i;
    (void)context;
    if (((size_t)address + length) > sizeof(g_flash)) return false;
    for (i = 0u; i < length; ++i)
    {
        if ((g_flash[address + i] & source[i]) != source[i]) return false;
        g_flash[address + i] &= source[i];
    }
    return true;
}

static bool mock_erase(void *context, uint32_t address, size_t length)
{
    (void)context;
    if (((size_t)address + length) > sizeof(g_flash)) return false;
    memset(&g_flash[address], 0xFF, length);
    return true;
}

int main(void)
{
    static const ParamStorageBackend_t backend =
    {
        .read = mock_read, .write = mock_write, .erase = mock_erase,
        .total_size = MOCK_SIZE, .erase_size = MOCK_REGION_SIZE, .write_size = 1u
    };
    static const ParamStorageLayout_t layout =
    {
        .id = PARAM_STORAGE_INTERNAL, .region_a = 0u,
        .region_b = MOCK_REGION_SIZE, .region_size = MOCK_REGION_SIZE
    };
    static const ParamDescriptor_t parameters[] =
    {
        {
            .id = 0x0101u, .name = "vout_target", .type = PARAM_TYPE_FLOAT,
            .group = PARAM_GROUP_OUTPUT,
            .flags = PARAM_FLAG_READ | PARAM_FLAG_WRITE | PARAM_FLAG_PERSISTENT,
            .storage = PARAM_STORAGE_INTERNAL, .min_value = 0.5f,
            .max_value = 40.0f, .default_value = 12.0f,
            .size = sizeof(g_voltage), .runtime = &g_voltage
        },
        {
            .id = 0x0102u, .name = "iout_limit", .type = PARAM_TYPE_U16,
            .group = PARAM_GROUP_OUTPUT,
            .flags = PARAM_FLAG_READ | PARAM_FLAG_WRITE | PARAM_FLAG_PERSISTENT,
            .storage = PARAM_STORAGE_INTERNAL, .min_value = 0.0f,
            .max_value = 10000.0f, .default_value = 5000.0f,
            .size = sizeof(g_current), .runtime = &g_current
        }
    };
    float value;

    memset(g_flash, 0xFF, sizeof(g_flash));
    assert(ParamCRC32("123456789", 9u) == 0xCBF43926u);
    assert(ParamBackend_Register(PARAM_STORAGE_INTERNAL, &backend) == PARAM_OK);
    assert(StorageLayout_Register(&layout) == PARAM_OK);
    assert(Param_Init(parameters, sizeof(parameters) / sizeof(parameters[0])) == PARAM_OK);
    assert(Param_Load() == PARAM_ERROR_NOT_FOUND);
    assert(g_voltage == 12.0f && g_current == 5000u && Param_IsDirty());

    value = 41.0f;
    assert(Param_Set(0x0101u, &value, sizeof(value)) == PARAM_ERROR_RANGE);
    value = 20.0f;
    assert(Param_Set(0x0101u, &value, sizeof(value)) == PARAM_OK);
    assert(Param_Commit() == PARAM_OK && !Param_IsDirty());

    value = 30.0f;
    assert(Param_Set(0x0101u, &value, sizeof(value)) == PARAM_OK);
    assert(Param_Commit() == PARAM_OK);
    g_flash[MOCK_REGION_SIZE + 32u] ^= 1u;

    value = 10.0f;
    assert(Param_Set(0x0101u, &value, sizeof(value)) == PARAM_OK);
    assert(Param_Rollback() == PARAM_OK);
    assert(g_voltage == 20.0f && !Param_IsDirty());

    puts("parameter manager tests passed");
    return 0;
}
