/**
 * @file user_flash_store.c
 * @brief Persistent settings storage on W25Q64.
 */

#include "user_flash_store.h"

#include <string.h>

#include "bsp_w25q64.h"
#include "power_ctrl.h"

#define USER_FLASH_STORE_BASE_ADDR        0x000000UL
#define USER_FLASH_STORE_MAGIC           0x55463450UL
#define USER_FLASH_STORE_VERSION         1U
#define USER_FLASH_STORE_RECORD_SIZE     40U

static uint8_t s_USER_flashStoreReady = 0U;
static uint8_t s_USER_flashStoreSavePending = 0U;

static void s_USER_flashStoreWriteLe32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);
    data[2] = (uint8_t)((value >> 16) & 0xFFU);
    data[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint32_t s_USER_flashStoreReadLe32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint32_t s_USER_flashStoreCrc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i;

    for (i = 0U; i < len; ++i)
    {
        uint32_t bit;
        crc ^= data[i];
        for (bit = 0U; bit < 8U; ++bit)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

static void s_USER_flashStoreBuildRecord(const POWER_ctrlSettings_t *settings, uint8_t *record)
{
    uint32_t crc;

    memset(record, 0xFF, USER_FLASH_STORE_RECORD_SIZE);
    s_USER_flashStoreWriteLe32(&record[0], USER_FLASH_STORE_MAGIC);
    s_USER_flashStoreWriteLe32(&record[4], USER_FLASH_STORE_VERSION);
    s_USER_flashStoreWriteLe32(&record[8], settings->set_voltage_mv);
    s_USER_flashStoreWriteLe32(&record[12], settings->set_current_ma);
    s_USER_flashStoreWriteLe32(&record[16], (uint32_t)settings->otp_set_mc);
    s_USER_flashStoreWriteLe32(&record[20], settings->ovp_set_mv);
    s_USER_flashStoreWriteLe32(&record[24], settings->ocp_set_ma);
    s_USER_flashStoreWriteLe32(&record[28], settings->fan_set_permille);
    crc = s_USER_flashStoreCrc32(record, USER_FLASH_STORE_RECORD_SIZE - 4U);
    s_USER_flashStoreWriteLe32(&record[36], crc);
}

static uint8_t s_USER_flashStoreParseRecord(const uint8_t *record, POWER_ctrlSettings_t *settings)
{
    const uint32_t expected_crc = s_USER_flashStoreReadLe32(&record[36]);
    const uint32_t actual_crc = s_USER_flashStoreCrc32(record, USER_FLASH_STORE_RECORD_SIZE - 4U);

    if ((s_USER_flashStoreReadLe32(&record[0]) != USER_FLASH_STORE_MAGIC) ||
        (s_USER_flashStoreReadLe32(&record[4]) != USER_FLASH_STORE_VERSION) ||
        (actual_crc != expected_crc))
    {
        return 0U;
    }

    settings->set_voltage_mv = s_USER_flashStoreReadLe32(&record[8]);
    settings->set_current_ma = s_USER_flashStoreReadLe32(&record[12]);
    settings->otp_set_mc = (int32_t)s_USER_flashStoreReadLe32(&record[16]);
    settings->ovp_set_mv = s_USER_flashStoreReadLe32(&record[20]);
    settings->ocp_set_ma = s_USER_flashStoreReadLe32(&record[24]);
    settings->fan_set_permille = s_USER_flashStoreReadLe32(&record[28]);

    if (settings->fan_set_permille > 1000U)
    {
        return 0U;
    }

    return 1U;
}

static void s_USER_flashStoreSaveSettings(void)
{
    POWER_ctrlSnapshot_t snapshot;
    uint8_t record[USER_FLASH_STORE_RECORD_SIZE];

    POWER_getAppSnapshot(&snapshot);
    s_USER_flashStoreBuildRecord(&snapshot.settings, record);
    BSP_w25q64SectorEraseApp(USER_FLASH_STORE_BASE_ADDR);
    BSP_w25q64PageProgramAppData(USER_FLASH_STORE_BASE_ADDR, record, (uint16_t)sizeof(record));
}

void USER_flashStoreInitApp(void)
{
    uint8_t record[USER_FLASH_STORE_RECORD_SIZE];
    POWER_ctrlSettings_t settings;

    BSP_w25q64InitAppDevice();
    s_USER_flashStoreReady = BSP_w25q64ProbeAppDevice();
    s_USER_flashStoreSavePending = 0U;

    if (s_USER_flashStoreReady == 0U)
    {
        return;
    }

    BSP_w25q64ReadAppData(USER_FLASH_STORE_BASE_ADDR, record, (uint32_t)sizeof(record));
    if (s_USER_flashStoreParseRecord(record, &settings) != 0U)
    {
        POWER_applyAppSettings(&settings);
    }
    else
    {
        s_USER_flashStoreSaveSettings();
    }
}

void USER_flashStoreRequestAppSave(void)
{
    if (s_USER_flashStoreReady != 0U)
    {
        s_USER_flashStoreSavePending = 1U;
    }
}

void USER_flashStoreRunAppTask(void)
{
    if ((s_USER_flashStoreReady == 0U) || (s_USER_flashStoreSavePending == 0U))
    {
        return;
    }

    s_USER_flashStoreSavePending = 0U;
    s_USER_flashStoreSaveSettings();
}

uint8_t USER_flashStoreIsAppReady(void)
{
    return s_USER_flashStoreReady;
}
