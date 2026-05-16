/**
 * @file    : bsp_st7701.h
 * @brief   : TODO: 请填写模块功能简介
 * @author  : UF4
 * @date    : 2026/5/16 11:21
 * @version : CLion
 * @project : STM32H743
 * @details : 
 * TODO: 请填写详细说明
 */

#ifndef STM32H743_BSP_ST7701_H
#define STM32H743_BSP_ST7701_H

#include "main.h"
#include "spi.h"

#define LCD_SPI_PORT hspi6

#define ST7701_MADCTL_MY  0x80U
#define ST7701_MADCTL_MX  0x40U
#define ST7701_MADCTL_MV  0x20U
#define ST7701_MADCTL_ML  0x10U
#define ST7701_MADCTL_BGR 0x08U
#define ST7701_MADCTL_MH  0x04U

#define ST7701_MADCTL_LANDSCAPE ((uint8_t)(ST7701_MADCTL_MX | ST7701_MADCTL_MV | ST7701_MADCTL_BGR))

#ifdef __cplusplus
extern "C" {
#endif
/* Exported types ------------------------------------------------------------*/

HAL_StatusTypeDef ST7701Init(void);
HAL_StatusTypeDef ST7701WriteCmd(uint8_t data);
HAL_StatusTypeDef ST7701WriteData(uint8_t data);
HAL_StatusTypeDef ST7701InitRegs(void);

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported function prototypes ----------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* STM32H743_BSP_ST7701_H */