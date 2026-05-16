/**
 * @file    : bsp_st7701.c
 * @brief   : TODO: 请填写模块功能简介
 * @author  : UF4
 * @date    : 2026/5/16 11:21
 * @version : CLion
 * @project : STM32H743
 * @details : 
 * TODO: 请填写详细说明
 */

#include "bsp_st7701.h"


HAL_StatusTypeDef ST7701Init(void){
    return ST7701InitRegs();
}


HAL_StatusTypeDef ST7701WriteCmd(const uint8_t data){
	uint16_t tx_data;
	HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_RESET);

	tx_data = (0<<8) | data;

	const HAL_StatusTypeDef ret = HAL_SPI_Transmit(&LCD_SPI_PORT, (const uint8_t *)&tx_data, 1, 10);

	HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_SET);
	return ret;
}

HAL_StatusTypeDef ST7701WriteData(const uint8_t data){
	uint16_t tx_data;
	HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_RESET);

	tx_data = (1<<8) | data;

	const HAL_StatusTypeDef ret = HAL_SPI_Transmit(&LCD_SPI_PORT, (const uint8_t *)&tx_data, 1, 10);

	HAL_GPIO_WritePin(LCD_SPI_CS_GPIO_Port, LCD_SPI_CS_Pin, GPIO_PIN_SET);
	return ret;
}

HAL_StatusTypeDef ST7701InitRegs(void){
    HAL_GPIO_WritePin(LCD_TP_RESET_GPIO_Port, LCD_TP_RESET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_SPI_RESET_GPIO_Port, LCD_SPI_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(30);

    HAL_GPIO_WritePin(LCD_TP_RESET_GPIO_Port, LCD_TP_RESET_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_SPI_RESET_GPIO_Port, LCD_SPI_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);

    HAL_GPIO_WritePin(LCD_TP_RESET_GPIO_Port, LCD_TP_RESET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_SPI_RESET_GPIO_Port, LCD_SPI_RESET_Pin, GPIO_PIN_SET);

	HAL_Delay(30);
	ST7701WriteCmd (0x11);
	HAL_Delay(100);

    ST7701WriteCmd (0xFF);
    ST7701WriteData (0x77);
    ST7701WriteData (0x01);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x13);
    ST7701WriteCmd (0xEF);
    ST7701WriteData (0x08);
    ST7701WriteCmd (0xFF);
    ST7701WriteData (0x77);
    ST7701WriteData (0x01);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x10);
    ST7701WriteCmd (0xC0);
    ST7701WriteData (0x4F);
    ST7701WriteData (0x00);
    ST7701WriteCmd (0xC1);
    ST7701WriteData (0x10);
    ST7701WriteData (0x0C);
    ST7701WriteCmd (0xC2);
    ST7701WriteData (0x01);
    ST7701WriteData (0x14);
    ST7701WriteCmd (0xCC);
    ST7701WriteData (0x10);
    ST7701WriteCmd (0xB0);
    ST7701WriteData (0x0A);
    ST7701WriteData (0x18);
    ST7701WriteData (0x1E);
    ST7701WriteData (0x12);
    ST7701WriteData (0x16);
    ST7701WriteData (0x0C);
    ST7701WriteData (0x0E);
    ST7701WriteData (0x0D);
    ST7701WriteData (0x0C);
    ST7701WriteData (0x29);
    ST7701WriteData (0x06);
    ST7701WriteData (0x14);
    ST7701WriteData (0x13);
    ST7701WriteData (0x29);
    ST7701WriteData (0x33);
    ST7701WriteData (0x1C);
    ST7701WriteCmd (0xB1);
    ST7701WriteData (0x0A);
    ST7701WriteData (0x19);
    ST7701WriteData (0x21);
    ST7701WriteData (0x0A);
    ST7701WriteData (0x0C);
    ST7701WriteData (0x00);
    ST7701WriteData (0x0C);
    ST7701WriteData (0x03);
    ST7701WriteData (0x03);
    ST7701WriteData (0x23);
    ST7701WriteData (0x01);
    ST7701WriteData (0x0E);
    ST7701WriteData (0x0C);
    ST7701WriteData (0x27);
    ST7701WriteData (0x2B);
    ST7701WriteData (0x1C);
    ST7701WriteCmd (0xFF);
    ST7701WriteData (0x77);
    ST7701WriteData (0x01);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x11);
    ST7701WriteCmd (0xB0);
    ST7701WriteData (0x5D);
    ST7701WriteCmd (0xB1);
    ST7701WriteData (0x4F);
    ST7701WriteCmd (0xB2);
    ST7701WriteData (0x00);
    ST7701WriteCmd (0xB3);
    ST7701WriteData (0x80);
    ST7701WriteCmd (0xB5);
    ST7701WriteData (0x0C);
    ST7701WriteCmd (0xB7);
    ST7701WriteData (0x85);
    ST7701WriteCmd (0xB8);
    ST7701WriteData (0x20);
    ST7701WriteCmd (0xC1);
    ST7701WriteData (0x78);
    ST7701WriteCmd (0xC2);
    ST7701WriteData (0x78);
    ST7701WriteCmd (0xD0);
    ST7701WriteData (0x88);
    ST7701WriteCmd (0xE0);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x02);
    ST7701WriteCmd (0xE1);
    ST7701WriteData (0x06);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x08);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x05);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x07);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x00);
    ST7701WriteData (0x44);
    ST7701WriteData (0x44);
    ST7701WriteCmd (0xE2);
    ST7701WriteData (0x20);
    ST7701WriteData (0x20);
    ST7701WriteData (0x44);
    ST7701WriteData (0x44);
    ST7701WriteData (0x96);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x96);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteCmd (0xE3);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x22);
    ST7701WriteData (0x22);
    ST7701WriteCmd (0xE4);
    ST7701WriteData (0x44);
    ST7701WriteData (0x44);
    ST7701WriteCmd (0xE5);
    ST7701WriteData (0x0D);
    ST7701WriteData (0x91);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x0F);
    ST7701WriteData (0x93);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x09);
    ST7701WriteData (0x8D);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x0B);
    ST7701WriteData (0x8F);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteCmd (0xE6);
    ST7701WriteData (0x00);
    ST7701WriteData (0x00);
    ST7701WriteData (0x22);
    ST7701WriteData (0x22);
    ST7701WriteCmd (0xE7);
    ST7701WriteData (0x44);
    ST7701WriteData (0x44);
    ST7701WriteCmd (0xE8);
    ST7701WriteData (0x0C);
    ST7701WriteData (0x90);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x0E);
    ST7701WriteData (0x92);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x08);
    ST7701WriteData (0x8C);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteData (0x0A);
    ST7701WriteData (0x8E);
    ST7701WriteData (0x0A);
    ST7701WriteData (0xA0);
    ST7701WriteCmd (0xE9);
    ST7701WriteData (0x36);
    ST7701WriteData (0x00);
    ST7701WriteCmd (0xEB);
    ST7701WriteData (0x00);
    ST7701WriteData (0x01);
    ST7701WriteData (0xE4);
    ST7701WriteData (0xE4);
    ST7701WriteData (0x44);
    ST7701WriteData (0x88);
    ST7701WriteData (0x40);
    ST7701WriteCmd (0xED);
    ST7701WriteData (0xFF);
    ST7701WriteData (0x45);
    ST7701WriteData (0x67);
    ST7701WriteData (0xFA);
    ST7701WriteData (0x01);
    ST7701WriteData (0x2B);
    ST7701WriteData (0xCF);
    ST7701WriteData (0xFF);
    ST7701WriteData (0xFF);
    ST7701WriteData (0xFC);
    ST7701WriteData (0xB2);
    ST7701WriteData (0x10);
    ST7701WriteData (0xAF);
    ST7701WriteData (0x76);
    ST7701WriteData (0x54);
    ST7701WriteData (0xFF);
    ST7701WriteCmd (0xEF);
    ST7701WriteData (0x10);
    ST7701WriteData (0x0D);
    ST7701WriteData (0x04);
    ST7701WriteData (0x08);
    ST7701WriteData (0x3F);
    ST7701WriteData (0x1F);
    ST7701WriteCmd (0x3A);
    ST7701WriteData (0x66);
    ST7701WriteCmd (0x11);

    HAL_Delay(120);

    ST7701WriteCmd (0x36);
    ST7701WriteData (ST7701_MADCTL_LANDSCAPE);
    ST7701WriteCmd (0x35);
    ST7701WriteData (0x00);

	return ST7701WriteCmd (0x29);
}