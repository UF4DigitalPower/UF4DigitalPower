# uf4-target

`uf4-target` is a small Windows command-line helper for the UF4 STM32F401 CMSIS-DAP probe. It sends UF4 vendor commands through the CMSIS-DAP HID interface to select which on-board target is connected to SWD.

## Usage

```powershell
.\uf4-target.exe list
.\uf4-target.exe get
.\uf4-target.exe set f429
.\uf4-target.exe set g474
.\uf4-target.exe esp32-boot
.\uf4-target.exe esp32-reset
.\uf4-target.exe reset
.\uf4-target.exe vtref
.\uf4-target.exe vref-adc
.\uf4-target.exe adc
.\uf4-target.exe pins --swclk 1 --swdio 1 --nrst 1
.\uf4-target.exe probe-idcode --target f429 --clock-khz 100 --repeat 3
```

Typical F429 flashing flow:

```powershell
.\tools\uf4-target\uf4-target.exe set f429
C:\OpenOCD\bin\openocd.exe -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "adapter speed 1000; program path\to\firmware.hex verify reset exit"
```

## Commands

- `list`: list matching UF4 CMSIS-DAP HID devices.
- `get`: read the currently selected target.
- `set f429`: route SWD and target UART to F429.
- `set g474`: route SWD and target UART to G474.
- `esp32-boot`: pull ESP32 BOOT low and reset it into download mode.
- `esp32-reset`: reset ESP32 back to normal run mode.
- `reset`: pulse NRST once on G474 and F429, then leave F429 selected.
- `vtref` / `vref-adc` / `adc`: read the PA4 target reference voltage after 1/2-divider compensation.
- `pins`: manually read or drive SWCLK, SWDIO, and NRST for hardware checks.
- `probe-idcode`: run raw SWD initialization and read the target DP IDCODE.

## Build

Use MinGW-w64 on Windows:

```powershell
gcc -O2 -Wall -Wextra -o uf4-target.exe uf4-target.c -lsetupapi -lhid
```

The checked-in EXE is the native C version. It does not depend on Python.

## 中文说明

`uf4-target` 是 UF4 STM32F401 CMSIS-DAP 的目标切换工具。它通过 CMSIS-DAP HID 发送 UF4 自定义命令，在烧录前选择当前 SWD 连接到 F429 还是 G474。

常用流程：

```powershell
.\tools\uf4-target\uf4-target.exe set f429
C:\OpenOCD\bin\openocd.exe -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "adapter speed 1000; program path\to\firmware.hex verify reset exit"
```

这个目录只保留 C 语言单文件版本，不再使用 Python 冻结包。

ESP32 进入下载模式：

```powershell
.\tools\uf4-target\uf4-target.exe esp32-boot
```

ESP32 复位运行：

```powershell
.\tools\uf4-target\uf4-target.exe esp32-reset
```
