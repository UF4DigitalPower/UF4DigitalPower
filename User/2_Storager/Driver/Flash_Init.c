#include "Flash_Init.h"

#include "../Storage/storage_layout.h"
#include "stm32g4xx_hal.h"

#include <string.h>

#define INTERNAL_FLASH_BASE 0x08000000u

typedef struct
{
    w25qxx_handle_t *handle;
} W25QBackendContext_t;

static W25QBackendContext_t s_w25q_contexts[PARAM_STORAGE_COUNT];
static ParamStorageBackend_t s_w25q_backends[PARAM_STORAGE_COUNT];

static bool internal_init(void *context)
{
    (void)context;
    return true;
}

static bool valid_range(uint32_t address, size_t length)
{
    return ((uint64_t)address + length) <= PARAM_INTERNAL_FLASH_SIZE;
}

static bool internal_read(void *context, uint32_t address,
                          void *data, size_t length)
{
    (void)context;
    if ((data == NULL) || !valid_range(address, length)) return false;
    memcpy(data, (const void *)(uintptr_t)(INTERNAL_FLASH_BASE + address), length);
    return true;
}

static bool internal_erase(void *context, uint32_t address, size_t length)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0u;
    HAL_StatusTypeDef result;
    (void)context;
    if (!valid_range(address, length) || (length == 0u) ||
        ((address % FLASH_PAGE_SIZE) != 0u) ||
        ((length % FLASH_PAGE_SIZE) != 0u)) return false;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = address / FLASH_PAGE_SIZE;
    erase.NbPages = length / FLASH_PAGE_SIZE;

    if (HAL_FLASH_Unlock() != HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    result = HAL_FLASHEx_Erase(&erase, &page_error);
    (void)HAL_FLASH_Lock();
    return result == HAL_OK;
}

static bool internal_write(void *context, uint32_t address,
                           const void *data, size_t length)
{
    const uint8_t *source = (const uint8_t *)data;
    uint32_t aligned_address;
    size_t source_offset = 0u;
    size_t byte_index;
    uint8_t bytes[8];
    uint64_t double_word;
    HAL_StatusTypeDef result = HAL_OK;
    (void)context;

    if ((data == NULL) || !valid_range(address, length)) return false;
    if (length == 0u) return true;
    aligned_address = address & ~7u;
    if (HAL_FLASH_Unlock() != HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    while ((source_offset < length) && (result == HAL_OK))
    {
        uint32_t absolute = INTERNAL_FLASH_BASE + aligned_address;
        size_t first = (address + source_offset) - aligned_address;
        size_t count = 8u - first;
        if (count > (length - source_offset)) count = length - source_offset;
        memcpy(bytes, (const void *)(uintptr_t)absolute, sizeof(bytes));
        for (byte_index = 0u; byte_index < count; ++byte_index)
        {
            uint8_t old_value = bytes[first + byte_index];
            uint8_t new_value = source[source_offset + byte_index];
            if ((old_value & new_value) != new_value)
            {
                result = HAL_ERROR;
                break;
            }
            bytes[first + byte_index] = new_value;
        }
        if (result != HAL_OK) break;
        memcpy(&double_word, bytes, sizeof(double_word));
        result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                   absolute, double_word);
        aligned_address += 8u;
        source_offset += count;
    }
    (void)HAL_FLASH_Lock();
    return result == HAL_OK;
}

const ParamStorageBackend_t g_internal_flash_backend =
{
    .init = internal_init,
    .read = internal_read,
    .write = internal_write,
    .erase = internal_erase,
    .context = NULL,
    .total_size = PARAM_INTERNAL_FLASH_SIZE,
    .erase_size = FLASH_PAGE_SIZE,
    .write_size = 8u
};

ParamStatus_t Flash_Init_RegisterInternal(void)
{
    static const ParamStorageLayout_t layout =
    {
        .id = PARAM_STORAGE_INTERNAL,
        .region_a = PARAM_INTERNAL_REGION_A,
        .region_b = PARAM_INTERNAL_REGION_B,
        .region_size = PARAM_INTERNAL_REGION_SIZE
    };
    ParamStatus_t status = ParamBackend_Register(PARAM_STORAGE_INTERNAL,
                                                  &g_internal_flash_backend);
    if (status != PARAM_OK) return status;
    return StorageLayout_Register(&layout);
}

static bool w25q_init(void *context)
{
    W25QBackendContext_t *device = (W25QBackendContext_t *)context;
    if ((device == NULL) || (device->handle == NULL)) return false;
    return (device->handle->inited == 1u) || (w25qxx_init(device->handle) == 0u);
}

static bool w25q_read(void *context, uint32_t address,
                       void *data, size_t length)
{
    W25QBackendContext_t *device = (W25QBackendContext_t *)context;
    return (length <= UINT32_MAX) &&
           (w25qxx_read(device->handle, address, data, (uint32_t)length) == 0u);
}

static bool w25q_write(void *context, uint32_t address,
                        const void *data, size_t length)
{
    W25QBackendContext_t *device = (W25QBackendContext_t *)context;
    return (length <= UINT32_MAX) &&
           (w25qxx_write(device->handle, address, (uint8_t *)(uintptr_t)data,
                         (uint32_t)length) == 0u);
}

static bool w25q_erase(void *context, uint32_t address, size_t length)
{
    W25QBackendContext_t *device = (W25QBackendContext_t *)context;
    size_t offset;
    if (((address % 4096u) != 0u) || ((length % 4096u) != 0u)) return false;
    for (offset = 0u; offset < length; offset += 4096u)
    {
        if (w25qxx_sector_erase_4k(device->handle,
                                   address + (uint32_t)offset) != 0u) return false;
    }
    return true;
}

ParamStatus_t Flash_Init_RegisterW25QXX(ParamStorageID_t storage,
                                        w25qxx_handle_t *handle,
                                        uint32_t total_size,
                                        uint32_t region_a,
                                        uint32_t region_b,
                                        uint32_t region_size)
{
    ParamStorageLayout_t layout;
    ParamStorageBackend_t *backend;
    ParamStatus_t status;
    if (((storage != PARAM_STORAGE_W25Q64) &&
         (storage != PARAM_STORAGE_W25Q128)) || (handle == NULL))
        return PARAM_ERROR_ARGUMENT;

    s_w25q_contexts[storage].handle = handle;
    backend = &s_w25q_backends[storage];
    backend->init = w25q_init;
    backend->read = w25q_read;
    backend->write = w25q_write;
    backend->erase = w25q_erase;
    backend->context = &s_w25q_contexts[storage];
    backend->total_size = total_size;
    backend->erase_size = 4096u;
    backend->write_size = 1u;
    status = ParamBackend_Register(storage, backend);
    if (status != PARAM_OK) return status;

    layout.id = storage;
    layout.region_a = region_a;
    layout.region_b = region_b;
    layout.region_size = region_size;
    return StorageLayout_Register(&layout);
}
