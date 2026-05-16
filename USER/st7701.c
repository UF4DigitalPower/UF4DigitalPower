//
// Created by UF4 on 2026/4/12.
//

#include "st7701.h"

static HAL_StatusTypeDef st7701WriteCmd(uint8_t data);
static HAL_StatusTypeDef st7701WriteData(uint8_t data);
static HAL_StatusTypeDef st7701InitRegs(void);


HAL_StatusTypeDef st7701Init(void)
{
  HAL_StatusTypeDef ret = st7701InitRegs();
  return ret;
}

HAL_StatusTypeDef st7701InitRegs(void)
{

    HAL_GPIO_WritePin(LCD_TP_RESET_GPIO_Port, LCD_TP_RESET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_SPI_RESET_GPIO_Port, LCD_SPI_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(30);
    HAL_GPIO_WritePin(LCD_TP_RESET_GPIO_Port, LCD_TP_RESET_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_SPI_RESET_GPIO_Port, LCD_SPI_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
    HAL_GPIO_WritePin(LCD_TP_RESET_GPIO_Port, LCD_TP_RESET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_SPI_RESET_GPIO_Port, LCD_SPI_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(30);
	st7701WriteCmd (0x11);
	HAL_Delay(100);

    st7701WriteCmd (0xFF);
    st7701WriteData (0x77);
    st7701WriteData (0x01);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x13);
    st7701WriteCmd (0xEF);
    st7701WriteData (0x08);
    st7701WriteCmd (0xFF);
    st7701WriteData (0x77);
    st7701WriteData (0x01);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x10);
    st7701WriteCmd (0xC0);
    st7701WriteData (0x4F);
    st7701WriteData (0x00);
    st7701WriteCmd (0xC1);
    st7701WriteData (0x10);
    st7701WriteData (0x0C);
    st7701WriteCmd (0xC2);
    st7701WriteData (0x01);
    st7701WriteData (0x14);
    st7701WriteCmd (0xCC);
    st7701WriteData (0x10);
    st7701WriteCmd (0xB0);
    st7701WriteData (0x0A);
    st7701WriteData (0x18);
    st7701WriteData (0x1E);
    st7701WriteData (0x12);
    st7701WriteData (0x16);
    st7701WriteData (0x0C);
    st7701WriteData (0x0E);
    st7701WriteData (0x0D);
    st7701WriteData (0x0C);
    st7701WriteData (0x29);
    st7701WriteData (0x06);
    st7701WriteData (0x14);
    st7701WriteData (0x13);
    st7701WriteData (0x29);
    st7701WriteData (0x33);
    st7701WriteData (0x1C);
    st7701WriteCmd (0xB1);
    st7701WriteData (0x0A);
    st7701WriteData (0x19);
    st7701WriteData (0x21);
    st7701WriteData (0x0A);
    st7701WriteData (0x0C);
    st7701WriteData (0x00);
    st7701WriteData (0x0C);
    st7701WriteData (0x03);
    st7701WriteData (0x03);
    st7701WriteData (0x23);
    st7701WriteData (0x01);
    st7701WriteData (0x0E);
    st7701WriteData (0x0C);
    st7701WriteData (0x27);
    st7701WriteData (0x2B);
    st7701WriteData (0x1C);
    st7701WriteCmd (0xFF);
    st7701WriteData (0x77);
    st7701WriteData (0x01);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x11);
    st7701WriteCmd (0xB0);
    st7701WriteData (0x5D);
    st7701WriteCmd (0xB1);
    st7701WriteData (0x4F);
    st7701WriteCmd (0xB2);
    st7701WriteData (0x00);
    st7701WriteCmd (0xB3);
    st7701WriteData (0x80);
    st7701WriteCmd (0xB5);
    st7701WriteData (0x0C);
    st7701WriteCmd (0xB7);
    st7701WriteData (0x85);
    st7701WriteCmd (0xB8);
    st7701WriteData (0x20);
    st7701WriteCmd (0xC1);
    st7701WriteData (0x78);
    st7701WriteCmd (0xC2);
    st7701WriteData (0x78);
    st7701WriteCmd (0xD0);
    st7701WriteData (0x88);
    st7701WriteCmd (0xE0);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x02);
    st7701WriteCmd (0xE1);
    st7701WriteData (0x06);
    st7701WriteData (0xA0);
    st7701WriteData (0x08);
    st7701WriteData (0xA0);
    st7701WriteData (0x05);
    st7701WriteData (0xA0);
    st7701WriteData (0x07);
    st7701WriteData (0xA0);
    st7701WriteData (0x00);
    st7701WriteData (0x44);
    st7701WriteData (0x44);
    st7701WriteCmd (0xE2);
    st7701WriteData (0x20);
    st7701WriteData (0x20);
    st7701WriteData (0x44);
    st7701WriteData (0x44);
    st7701WriteData (0x96);
    st7701WriteData (0xA0);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x96);
    st7701WriteData (0xA0);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteCmd (0xE3);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x22);
    st7701WriteData (0x22);
    st7701WriteCmd (0xE4);
    st7701WriteData (0x44);
    st7701WriteData (0x44);
    st7701WriteCmd (0xE5);
    st7701WriteData (0x0D);
    st7701WriteData (0x91);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteData (0x0F);
    st7701WriteData (0x93);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteData (0x09);
    st7701WriteData (0x8D);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteData (0x0B);
    st7701WriteData (0x8F);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteCmd (0xE6);
    st7701WriteData (0x00);
    st7701WriteData (0x00);
    st7701WriteData (0x22);
    st7701WriteData (0x22);
    st7701WriteCmd (0xE7);
    st7701WriteData (0x44);
    st7701WriteData (0x44);
    st7701WriteCmd (0xE8);
    st7701WriteData (0x0C);
    st7701WriteData (0x90);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteData (0x0E);
    st7701WriteData (0x92);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteData (0x08);
    st7701WriteData (0x8C);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteData (0x0A);
    st7701WriteData (0x8E);
    st7701WriteData (0x0A);
    st7701WriteData (0xA0);
    st7701WriteCmd (0xE9);
    st7701WriteData (0x36);
    st7701WriteData (0x00);
    st7701WriteCmd (0xEB);
    st7701WriteData (0x00);
    st7701WriteData (0x01);
    st7701WriteData (0xE4);
    st7701WriteData (0xE4);
    st7701WriteData (0x44);
    st7701WriteData (0x88);
    st7701WriteData (0x40);
    st7701WriteCmd (0xED);
    st7701WriteData (0xFF);
    st7701WriteData (0x45);
    st7701WriteData (0x67);
    st7701WriteData (0xFA);
    st7701WriteData (0x01);
    st7701WriteData (0x2B);
    st7701WriteData (0xCF);
    st7701WriteData (0xFF);
    st7701WriteData (0xFF);
    st7701WriteData (0xFC);
    st7701WriteData (0xB2);
    st7701WriteData (0x10);
    st7701WriteData (0xAF);
    st7701WriteData (0x76);
    st7701WriteData (0x54);
    st7701WriteData (0xFF);
    st7701WriteCmd (0xEF);
    st7701WriteData (0x10);
    st7701WriteData (0x0D);
    st7701WriteData (0x04);
    st7701WriteData (0x08);
    st7701WriteData (0x3F);
    st7701WriteData (0x1F);
    st7701WriteCmd (0x3A);
    st7701WriteData (0x66);
    st7701WriteCmd (0x11);
    HAL_Delay(120);
    st7701WriteCmd (0x36);
    st7701WriteData (0x68);
    st7701WriteCmd (0x35);
    st7701WriteData (0x00);
    HAL_StatusTypeDef ret = st7701WriteCmd (0x29);
    return ret;
}

HAL_StatusTypeDef st7701WriteCmd(uint8_t data){
  uint16_t tx_data;
  HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_RESET);

  tx_data = (0<<8) | data;

  HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi6, (const uint8_t *)&tx_data, 1, 10);

  HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_SET);
  return ret;
}

HAL_StatusTypeDef st7701WriteData(uint8_t data){
  uint16_t tx_data;
  HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_RESET);

  tx_data = (1<<8) | data;

  HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi6, (const uint8_t *)&tx_data, 1, 10);

  HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_SET);
  return ret;
}
