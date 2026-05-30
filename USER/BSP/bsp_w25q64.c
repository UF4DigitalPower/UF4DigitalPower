/**
 * @file bsp_w25q64.c
 * @brief W25Q64 SPI flash driver for SPI1.
 */

#include "bsp_w25q64.h"

#include "main.h"
#include "spi.h"

#define BSP_W25Q64_WRITE_ENABLE             0x06U
#define BSP_W25Q64_READ_STATUS_REGISTER_1   0x05U
#define BSP_W25Q64_PAGE_PROGRAM             0x02U
#define BSP_W25Q64_SECTOR_ERASE_4KB         0x20U
#define BSP_W25Q64_JEDEC_ID                 0x9FU
#define BSP_W25Q64_READ_DATA                0x03U
#define BSP_W25Q64_DUMMY_BYTE               0xFFU
#define BSP_W25Q64_BUSY_MASK                0x01U
#define BSP_W25Q64_WAIT_BUSY_LIMIT          100000UL
#define BSP_W25Q64_SPI_TIMEOUT_MS           100U

static void s_BSP_w25q64SelectAppDevice(void)
{
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
}

static void s_BSP_w25q64DeselectAppDevice(void)
{
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
}

static uint8_t s_BSP_w25q64SwapAppByte(uint8_t tx)
{
    uint8_t rx = BSP_W25Q64_DUMMY_BYTE;

    if (HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1U, BSP_W25Q64_SPI_TIMEOUT_MS) != HAL_OK)
    {
        return BSP_W25Q64_DUMMY_BYTE;
    }

    return rx;
}

static void s_BSP_w25q64WriteEnableApp(void)
{
    s_BSP_w25q64SelectAppDevice();
    (void)s_BSP_w25q64SwapAppByte(BSP_W25Q64_WRITE_ENABLE);
    s_BSP_w25q64DeselectAppDevice();
}

static void s_BSP_w25q64WaitBusyApp(void)
{
    uint32_t timeout = BSP_W25Q64_WAIT_BUSY_LIMIT;

    s_BSP_w25q64SelectAppDevice();
    (void)s_BSP_w25q64SwapAppByte(BSP_W25Q64_READ_STATUS_REGISTER_1);
    while ((s_BSP_w25q64SwapAppByte(BSP_W25Q64_DUMMY_BYTE) & BSP_W25Q64_BUSY_MASK) != 0U)
    {
        --timeout;
        if (timeout == 0U)
        {
            break;
        }
    }
    s_BSP_w25q64DeselectAppDevice();
}

static void s_BSP_w25q64WriteAppAddress(uint32_t address)
{
    (void)s_BSP_w25q64SwapAppByte((uint8_t)(address >> 16));
    (void)s_BSP_w25q64SwapAppByte((uint8_t)(address >> 8));
    (void)s_BSP_w25q64SwapAppByte((uint8_t)address);
}

void BSP_w25q64InitAppDevice(void)
{
    s_BSP_w25q64DeselectAppDevice();
}

void BSP_w25q64ReadAppId(uint8_t *manufacturer_id, uint16_t *device_id)
{
    uint8_t mid;
    uint16_t did;

    s_BSP_w25q64SelectAppDevice();
    (void)s_BSP_w25q64SwapAppByte(BSP_W25Q64_JEDEC_ID);
    mid = s_BSP_w25q64SwapAppByte(BSP_W25Q64_DUMMY_BYTE);
    did = (uint16_t)s_BSP_w25q64SwapAppByte(BSP_W25Q64_DUMMY_BYTE) << 8;
    did |= s_BSP_w25q64SwapAppByte(BSP_W25Q64_DUMMY_BYTE);
    s_BSP_w25q64DeselectAppDevice();

    if (manufacturer_id != NULL)
    {
        *manufacturer_id = mid;
    }
    if (device_id != NULL)
    {
        *device_id = did;
    }
}

uint8_t BSP_w25q64ProbeAppDevice(void)
{
    uint8_t mid;
    uint16_t did;

    BSP_w25q64ReadAppId(&mid, &did);
    if ((mid == 0U) || (mid == 0xFFU) || (did == 0U) || (did == 0xFFFFU))
    {
        return 0U;
    }

    return 1U;
}

void BSP_w25q64ReadAppData(uint32_t address, uint8_t *data, uint32_t count)
{
    uint32_t i;

    if ((data == NULL) || (count == 0U))
    {
        return;
    }

    s_BSP_w25q64WaitBusyApp();
    s_BSP_w25q64SelectAppDevice();
    (void)s_BSP_w25q64SwapAppByte(BSP_W25Q64_READ_DATA);
    s_BSP_w25q64WriteAppAddress(address);
    for (i = 0U; i < count; ++i)
    {
        data[i] = s_BSP_w25q64SwapAppByte(BSP_W25Q64_DUMMY_BYTE);
    }
    s_BSP_w25q64DeselectAppDevice();
}

void BSP_w25q64PageProgramAppData(uint32_t address, const uint8_t *data, uint16_t count)
{
    uint16_t i;

    if ((data == NULL) || (count == 0U))
    {
        return;
    }

    s_BSP_w25q64WaitBusyApp();
    s_BSP_w25q64WriteEnableApp();

    s_BSP_w25q64SelectAppDevice();
    (void)s_BSP_w25q64SwapAppByte(BSP_W25Q64_PAGE_PROGRAM);
    s_BSP_w25q64WriteAppAddress(address);
    for (i = 0U; i < count; ++i)
    {
        (void)s_BSP_w25q64SwapAppByte(data[i]);
    }
    s_BSP_w25q64DeselectAppDevice();
}

void BSP_w25q64SectorEraseApp(uint32_t address)
{
    s_BSP_w25q64WaitBusyApp();
    s_BSP_w25q64WriteEnableApp();

    s_BSP_w25q64SelectAppDevice();
    (void)s_BSP_w25q64SwapAppByte(BSP_W25Q64_SECTOR_ERASE_4KB);
    s_BSP_w25q64WriteAppAddress(address);
    s_BSP_w25q64DeselectAppDevice();
}
