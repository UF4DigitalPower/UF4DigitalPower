/**
 * @file bsp_w25q64.h
 * @brief W25Q64 SPI flash driver for SPI1.
 */

#ifndef UF4DIGITALPOWER_BSP_W25Q64_H
#define UF4DIGITALPOWER_BSP_W25Q64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BSP_W25Q64_JEDEC_MANUFACTURER_WINBOND 0xEFU

void BSP_w25q64InitAppDevice(void);
void BSP_w25q64ReadAppId(uint8_t *manufacturer_id, uint16_t *device_id);
uint8_t BSP_w25q64ProbeAppDevice(void);
void BSP_w25q64ReadAppData(uint32_t address, uint8_t *data, uint32_t count);
void BSP_w25q64PageProgramAppData(uint32_t address, const uint8_t *data, uint16_t count);
void BSP_w25q64SectorEraseApp(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* UF4DIGITALPOWER_BSP_W25Q64_H */
