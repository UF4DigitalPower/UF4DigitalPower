# 硬件连接
板载两个EC11编码器连接到TIM2.TIM4的硬件解码器上，还有PUSH按钮用来确认

五个按钮连接到E2-E6,外部中断

LTDC屏幕是640×480的ST7701的RGB666的屏幕，使用RGB565的LTDC驱动，使用DMA2D，没有用到I2C触摸，使用SPI6初始化

外部SDRAM使用W9825G6KH

外部QFLASH连接到W25Q64




fmc.c文件里面的SDRAM初始化 ，cubemx生成代码里面没有这个，需要加上

```
uint32_t tmpr = 0;
FMC_SDRAM_CommandTypeDef Command;

SDRAM_HandleTypeDef hsdram2;

// 第一步，给 SDRAM 提供时钟
Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
Command.AutoRefreshNumber = 1;
Command.ModeRegisterDefinition = 0;

HAL_SDRAM_SendCommand(&hsdram2, &Command, SDRAM_TIMEOUT);

// 第二步，延迟至少 200 us
HAL_Delay(1); // 延时 1 ms

// 第三步，对所有 Banks 预充电
Command.CommandMode = FMC_SDRAM_CMD_PALL;
Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
Command.AutoRefreshNumber = 1;
Command.ModeRegisterDefinition = 0;

HAL_SDRAM_SendCommand(&hsdram2, &Command, SDRAM_TIMEOUT);

// 第四步，插入 8 个自动刷新周期
Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
Command.AutoRefreshNumber = 8;
Command.ModeRegisterDefinition = 0;

HAL_SDRAM_SendCommand(&hsdram2, &Command, SDRAM_TIMEOUT);

// 第五步，编程 SDRAM 的加载模式寄存器
tmpr = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 |
SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
SDRAM_MODEREG_CAS_LATENCY_3 |
SDRAM_MODEREG_OPERATING_MODE_STANDARD |
SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

/* 配置命令：设置SDRAM寄存器 */
Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
Command.AutoRefreshNumber = 1;
Command.ModeRegisterDefinition = tmpr;

HAL_SDRAM_SendCommand(&hsdram2, &Command, SDRAM_TIMEOUT);

// 第六步，配置STM32FMC的 SDRAM 控制器的自动刷新周期
HAL_SDRAM_ProgramRefreshRate(&hsdram2, 918);

```
示例
 ```
/**
 * @brief  FMC SDRAM 模式配置的寄存器相关定义
 */
#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2 ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4 ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8 ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3 ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200)


```

st7701相关
```
#ifdef BIST MODE  //1 open

SPI_write_comm(0xFF);
SPI_write_data(0x77);
SPI_write_data(0x01);
SPI_write_data(0x00);
SPI_write_data(0x00);
SPI_write_data(0x12);
SPI_write_comm(0xD1);
SPI_write_data(0x81);
SPI_write_data(0x08);
SPI_write_data(0x03);
SPI_write_data(0x20);
SPI_write_data(0x08);
SPI_write_data(0x01);
SPI_write_data(0xA0);
SPI_write_data(0x01);
SPI_write_data(0xE0);
SPI_write_data(0xA0);
SPI_write_data(0x01);
SPI_write_data(0xE0);
SPI_write_data(0x03);
SPI_write_data(0x20);
SPI_write_comm(0xD2);
/*0x08:colorbar,0X02: red*/
SPI_write_data(0x08);

#endif


SPI_write_comm(0x11);


delay 120ms

SPI_write_comm(0x29);

// -- 

// BOE2.8IPS(ZV028V0Q-N10-1QP0 )_ADS(480X640)_FW(Customer)-45Duty-190802.ini.txt
HW_Reset();               //VBP>=16, INT_VFP=12, EXT_VFP>=14
Delay(120);               //ms
WriteComm (0xFF);
WriteData (0x77);
WriteData (0x01);
WriteData (0x00);
WriteData (0x00);
WriteData (0x13);
WriteComm (0xEF);
WriteData (0x08);
WriteComm (0xFF);
WriteData (0x77);
WriteData (0x01);
WriteData (0x00);
WriteData (0x00);
WriteData (0x10);
WriteComm (0xC0);
WriteData (0x4F);
WriteData (0x00);
WriteComm (0xC1);
WriteData (0x10);
WriteData (0x0C);
WriteComm (0xC2);
WriteData (0x01);
WriteData (0x14);
WriteComm (0xCC);
WriteData (0x10);
WriteComm (0xB0);
WriteData (0x0A);
WriteData (0x18);
WriteData (0x1E);
WriteData (0x12);
WriteData (0x16);
WriteData (0x0C);
WriteData (0x0E);
WriteData (0x0D);
WriteData (0x0C);
WriteData (0x29);
WriteData (0x06);
WriteData (0x14);
WriteData (0x13);
WriteData (0x29);
WriteData (0x33);
WriteData (0x1C);
WriteComm (0xB1);
WriteData (0x0A);
WriteData (0x19);
WriteData (0x21);
WriteData (0x0A);
WriteData (0x0C);
WriteData (0x00);
WriteData (0x0C);
WriteData (0x03);
WriteData (0x03);
WriteData (0x23);
WriteData (0x01);
WriteData (0x0E);
WriteData (0x0C);
WriteData (0x27);
WriteData (0x2B);
WriteData (0x1C);
WriteComm (0xFF);
WriteData (0x77);
WriteData (0x01);
WriteData (0x00);
WriteData (0x00);
WriteData (0x11);
WriteComm (0xB0);
WriteData (0x5D);
WriteComm (0xB1);
WriteData (0x4F);
WriteComm (0xB2);
WriteData (0x00);
WriteComm (0xB3);
WriteData (0x80);
WriteComm (0xB5);
WriteData (0x0C);
WriteComm (0xB7);
WriteData (0x85);
WriteComm (0xB8);
WriteData (0x20);
WriteComm (0xC1);
WriteData (0x78);
WriteComm (0xC2);
WriteData (0x78);
WriteComm (0xD0);
WriteData (0x88);
WriteComm (0xE0);
WriteData (0x00);
WriteData (0x00);
WriteData (0x02);
WriteComm (0xE1);
WriteData (0x06);
WriteData (0xA0);
WriteData (0x08);
WriteData (0xA0);
WriteData (0x05);
WriteData (0xA0);
WriteData (0x07);
WriteData (0xA0);
WriteData (0x00);
WriteData (0x44);
WriteData (0x44);
WriteComm (0xE2);
WriteData (0x20);
WriteData (0x20);
WriteData (0x44);
WriteData (0x44);
WriteData (0x96);
WriteData (0xA0);
WriteData (0x00);
WriteData (0x00);
WriteData (0x96);
WriteData (0xA0);
WriteData (0x00);
WriteData (0x00);
WriteComm (0xE3);
WriteData (0x00);
WriteData (0x00);
WriteData (0x22);
WriteData (0x22);
WriteComm (0xE4);
WriteData (0x44);
WriteData (0x44);
WriteComm (0xE5);
WriteData (0x0D);
WriteData (0x91);
WriteData (0x0A);
WriteData (0xA0);
WriteData (0x0F);
WriteData (0x93);
WriteData (0x0A);
WriteData (0xA0);
WriteData (0x09);
WriteData (0x8D);
WriteData (0x0A);
WriteData (0xA0);
WriteData (0x0B);
WriteData (0x8F);
WriteData (0x0A);
WriteData (0xA0);
WriteComm (0xE6);
WriteData (0x00);
WriteData (0x00);
WriteData (0x22);
WriteData (0x22);
WriteComm (0xE7);
WriteData (0x44);
WriteData (0x44);
WriteComm (0xE8);
WriteData (0x0C);
WriteData (0x90);
WriteData (0x0A);
WriteData (0xA0);
WriteData (0x0E);
WriteData (0x92);
WriteData (0x0A);
WriteData (0xA0);
WriteData (0x08);
WriteData (0x8C);
WriteData (0x0A);
WriteData (0xA0);
WriteData (0x0A);
WriteData (0x8E);
WriteData (0x0A);
WriteData (0xA0);
WriteComm (0xE9);
WriteData (0x36);
WriteData (0x00);
WriteComm (0xEB);
WriteData (0x00);
WriteData (0x01);
WriteData (0xE4);
WriteData (0xE4);
WriteData (0x44);
WriteData (0x88);
WriteData (0x40);
WriteComm (0xED);
WriteData (0xFF);
WriteData (0x45);
WriteData (0x67);
WriteData (0xFA);
WriteData (0x01);
WriteData (0x2B);
WriteData (0xCF);
WriteData (0xFF);
WriteData (0xFF);
WriteData (0xFC);
WriteData (0xB2);
WriteData (0x10);
WriteData (0xAF);
WriteData (0x76);
WriteData (0x54);
WriteData (0xFF);
WriteComm (0xEF);
WriteData (0x10);
WriteData (0x0D);
WriteData (0x04);
WriteData (0x08);
WriteData (0x3F);
WriteData (0x1F);
WriteComm (0x3A);
WriteData (0x66);
WriteComm (0x11);
Delay_ms(120);
WriteComm (0x36);
WriteData (0x00);
WriteComm (0x35);
WriteData (0x00);
WriteComm (0x29);


```