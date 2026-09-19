# 板级设计说明

## 板卡简称

- **G4**：STM32G474 所在板卡
- **F4**：STM32F429 所在板卡
- **DAPLink**：STM32F401 所在调试器板卡

## 串口连接

| 通信链路            | 设备端接口              | 对端接口                    | 用途                                  |
|-----------------|--------------------|-------------------------|-------------------------------------|
| G4 与 DAPLink    | G4 USART1：PC4/PC5  | DAPLink USART1：PA9/PA10 | PC 通过 `Virtual COM(target)` 与 G4 通信 |
| F4 与 DAPLink    | F4 USART1：PA9/PA10 | DAPLink USART6：PC6/PC7  | PC 通过 `Virtual COM(target)` 与 F4 通信 |
| G4 与 F4         | G4 USART2：PA2/PA3  | F4 USART6：PC6/PC7       | G4 与 F4 板间通信                        |
| ESP32 与 DAPLink | ESP32 下载串口         | DAPLink USART2：PA2/PA3  | PC 通过 `ESP32 COM(target)` 下载程序或通信   |

上述引脚按 `TX/RX` 顺序列出，板间连接时 TX 与对端 RX 交叉连接。

`Virtual COM(target)` 只连接当前由 `uf4-target` 选中的目标：

- 选择 `g474` 时，连接 DAPLink USART1 与 G4 USART1。
- 选择 `f429` 时，连接 DAPLink USART6 与 F4 USART1。
- DAPLink 上电后默认选择 F429。

目标串口使用 8 个数据位、1 个停止位、无校验（8N1）。波特率跟随 PC 串口工具的设置。

## PC 端 USB 接口

DAPLink 在 PC 端枚举为一个 USB 复合设备，提供以下功能接口：

- `CMSIS-DAP`：用于下载和调试 G4 或 F4。
- `Virtual COM(target)`：连接当前选中的 G4 或 F4 目标串口。
- `ESP32 COM(target)`：固定连接 DAPLink USART2 与 ESP32 下载串口。

切换目标时，SWD 调试接口和 `Virtual COM(target)` 会同步切换；`ESP32 COM(target)` 不受影响。

## 下载和调试

在烧录或调试 G4、F4 前，先使用 `uf4-target` 选择目标。以下命令均从 `Upper/STM32G474RBT6` 目录执行：

```powershell
.\tools\uf4-target\uf4-target.exe set f429
.\tools\uf4-target\uf4-target.exe set g474
.\tools\uf4-target\uf4-target.exe esp32-boot
.\tools\uf4-target\uf4-target.exe esp32-reset
.\tools\uf4-target\uf4-target.exe reset
.\tools\uf4-target\uf4-target.exe vtref
.\tools\uf4-target\uf4-target.exe vref-adc
.\tools\uf4-target\uf4-target.exe probe-idcode --target f429 --clock-khz 100 --repeat 3
```

### 使用 OpenOCD 烧录 F429

```powershell
.\tools\uf4-target\uf4-target.exe set f429
C:\OpenOCD\bin\openocd.exe -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "adapter speed 1000; program path\to\firmware.hex verify reset exit"
```

### 使用 OpenOCD 烧录 G474

```powershell
.\tools\uf4-target\uf4-target.exe set g474
C:\OpenOCD\bin\openocd.exe -f interface/cmsis-dap.cfg -f target/stm32g4x.cfg -c "adapter speed 1000; program path\to\firmware.hex verify reset exit"
```

`reset` 命令会依次复位 G4 和 F4，执行结束后保留 F429 为当前目标。

## ESP32 下载

使用 `esptool` 下载前，先让 ESP32 进入下载模式：

```powershell
.\tools\uf4-target\uf4-target.exe esp32-boot
```

下载完成后复位 ESP32，使其恢复正常运行：

```powershell
.\tools\uf4-target\uf4-target.exe esp32-reset
```
