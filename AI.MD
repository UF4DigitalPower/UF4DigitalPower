# AI.MD

## 项目定位

UF4DigitalPower 是基于 STM32G474 的数字电源固件。当前通信协议已切换到 `E:\PROJECT_C\UF4COM` 的 UF4COM V3。

## 当前维护状态

- 日期：2026-06-13
- 维护者：Codex
- 当前重点：删除旧 TVLCOM TLV/Modbus CRC 协议实现，接入 UF4COM V3，并绑定完整数据 ID。

## 本次变更

- `Core/Src/tvlcom.c` 重写为 UF4COM 传输适配层，保留 `TVLCOM_Init()`、`TVLCOM_RunTask()`、`TVLCOM_OnUsbCdcRx()`、`TVLCOM_SendBytes()` 等硬件接入口。
- 删除旧协议内部文件：`Core/Inc/tvlcom_internal.h`、`Core/Src/tvlcom_data.c`、`Core/Src/tvlcom_protocol.c`。
- 删除旧协议测试：`tests/test_user_tvlcom_protocol.c`。
- CMake 引入 `E:/PROJECT_C/UF4COM/uf4com` 的协议核心源码和头文件。
- 在 `Core/Src/tvlcom.c` 中绑定 UF4COM README 表里的全部数据 ID。
- 写入 ID 会应用到 `SET_Value`、`MAX_OTP_VAL`、`MAX_VOUT_OVP_VAL`、`MAX_VOUT_OCP_VAL`、`DF.OUTPUT_Flag` 和风扇 PWM。
- 通过 `UF4_SetWriteApplyCallback()` 只在成功处理 `WRITE_REQ` 后应用写入，READ/STREAM 不会误触发设置保存或风扇手动模式。

## 当前协议入口

- USB CDC RX：`USB_Device/App/usbd_cdc_if.c` -> `TVLCOM_OnUsbCdcRx()` -> `UF4_InputBuffer()`
- USART RX：`TVLCOM_RunTask()` 轮询 DMA 环形缓冲 -> `UF4_InputBuffer()`
- TX：`UF4_SendFrame()` -> `TVLCOM_SendBytes()` -> USB CDC 或 UART DMA
- 周期流：`TVLCOM_RunTask()` 每 20 ms 调用一次 `UF4_Process()`

## 验证记录

- 2026-06-13：`cmake --build E:\PROJECT_C\UF4DigitalPower\cmake-build-debug -- -j4` 构建通过，生成 `UF4DigitalPower.elf/.hex/.bin`。
- 2026-06-13：`E:\PROJECT_C\UF4COM\cmake-build-debug\UF4COM.exe` 自测通过。
