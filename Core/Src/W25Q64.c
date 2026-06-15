#include "W25Q64.h"
#include "W25Q64_Ins.h"
#include "spi.h"
#include "function.h"

/**
 * @brief 开始 W25Q64 SPI 通信。
 * 在开始与 W25Q64 存储器通信之前，需要先调用此函数。
 * 该函数会将 Flash_CS_Pin 引脚设置为低电平，以启动 SPI 通信。
 */
void W25Q64_SPIStart(void)
{
	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 停止W25Q64 SPI通信。
 * 将Flash_CS_Pin设置为高电平，停止W25Q64通信。
 */
void W25Q64_SPIStop(void)
{
	HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief SPI 交换字节。
 * 通过 SPI 接口发送一个字节数据，并接收返回的数据。
 * @param ByteSend 要发送的字节数据
 * @return 接收到的字节数据
 */
uint8_t MySPI_SwapByte(uint8_t ByteSend)
{
	uint8_t rxData = 0;													   // 用于接收数据的变量
	HAL_SPI_TransmitReceive(&hspi1, &ByteSend, &rxData, 1, HAL_MAX_DELAY); // SPI发送数据并接收数据
	return rxData;
}

/**
 * @brief 读取 W25Q64 的 JEDEC 设备 ID。
 * 读取厂商 ID 和器件 ID，用于确认外部 Flash 是否通信正常。
 */
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
{
	W25Q64_SPIStart();
	MySPI_SwapByte(W25Q64_JEDEC_ID);
	*MID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	*DID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	*DID <<= 8;
	*DID |= MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	W25Q64_SPIStop();
}

/**
 * @brief 发送写使能命令。
 * 在页编程、扇区擦除等写操作前必须先执行此函数。
 */
void W25Q64_WriteEnable(void)
{
	W25Q64_SPIStart();
	MySPI_SwapByte(W25Q64_WRITE_ENABLE);
	W25Q64_SPIStop();
}

/**
 * @brief 等待 W25Q64 退出忙状态。
 * 轮询状态寄存器 1 的 BUSY 位，直到器件空闲或超时退出。
 */
void W25Q64_WaitBusy(void)
{
	W25Q64_SPIStart();
	MySPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);
	uint32_t Timeout = 100000;
	while ((MySPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)
	{
		Timeout--;
		if (Timeout == 0)
		{
			break;
		}
	}
	W25Q64_SPIStop();
}

/**
 * @brief 按页写入一段数据到 Flash。
 * 从指定地址开始连续写入 Count 个字节，不跨页拆分由上层保证。
 */
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
	uint16_t i;

	W25Q64_WaitBusy();

	W25Q64_WriteEnable();

	W25Q64_SPIStart();
	MySPI_SwapByte(W25Q64_PAGE_PROGRAM);
	MySPI_SwapByte(Address >> 16);
	MySPI_SwapByte(Address >> 8);
	MySPI_SwapByte(Address);
	for (i = 0; i < Count; i++)
	{
		MySPI_SwapByte(DataArray[i]);
	}
	W25Q64_SPIStop();
}

/**
 * @brief 擦除包含指定地址的 4KB 扇区。
 * 擦除前会等待器件空闲并自动发送写使能命令。
 */
void W25Q64_SectorErase(uint32_t Address)
{
	W25Q64_WaitBusy();

	W25Q64_WriteEnable();

	W25Q64_SPIStart();
	MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);
	MySPI_SwapByte(Address >> 16);
	MySPI_SwapByte(Address >> 8);
	MySPI_SwapByte(Address);
	W25Q64_SPIStop();
}

/**
 * @brief 从 Flash 中读取一段连续数据。
 * 从指定地址开始读取 Count 个字节到目标缓冲区。
 */
void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
	uint32_t i;
	
	W25Q64_WaitBusy();

	W25Q64_SPIStart();
	MySPI_SwapByte(W25Q64_READ_DATA);
	MySPI_SwapByte(Address >> 16);
	MySPI_SwapByte(Address >> 8);
	MySPI_SwapByte(Address);
	for (i = 0; i < Count; i++)
	{
		DataArray[i] = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	}
	W25Q64_SPIStop();
}
