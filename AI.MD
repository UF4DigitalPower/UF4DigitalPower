# AI.MD

## 项目定位

UF4DigitalPower 是基于 STM32G474 的数字电源固件。当前通信协议已切换到 `E:\PROJECT_C\UF4COM` 的 UF4COM V3。

## 当前维护状态

- 日期：2026-06-13
- 维护者：Codex
- 当前重点：删除旧 TVL TLV/Modbus CRC 协议实现，接入 UF4COM V3，并绑定完整数据 ID。

## 本次变更

- `Core/Src/uf4_transport.c` 重写为 UF4COM 传输适配层，保留 `UF4Transport_Init()`、`UF4Transport_RunTask()`、`UF4Transport_OnUsbCdcRx()`、`UF4Transport_SendBytes()` 等硬件接入口。
- 删除旧协议内部实现文件和旧协议测试代码。
- CMake 引入 `E:/PROJECT_C/UF4COM/uf4com` 的协议核心源码和头文件。
- 在 `Core/Src/uf4_transport.c` 中绑定 UF4COM README 表里的全部数据 ID。
- 写入 ID 会应用到 `SET_Value`、`MAX_OTP_VAL`、`MAX_VOUT_OVP_VAL`、`MAX_VOUT_OCP_VAL`、`DF.OUTPUT_Flag` 和风扇 PWM。
- 通过 `UF4_SetWriteApplyCallback()` 只在成功处理 `WRITE_REQ` 后应用写入，READ/STREAM 不会误触发设置保存或风扇手动模式。
- 温度和 OTP 类 ID 使用摄氏度乘以 100 的协议值，界面保留两位小数，避免 `uint16_t` 下毫摄氏度量程不足。
- 参考 `Buck-Boost-Digital-Power` 的本地“按键 + OLED”交互后，当前工程已明确改为由 `UF4COM/F4CP` 接管人机交互：
  - 输出电压/电流设定：`UF4_ID_SET_VOLTAGE_LIMIT`、`UF4_ID_SET_CURRENT_LIMIT`
  - 输出开关：`UF4_ID_POWER_STATE`
  - 保护阈值：`UF4_ID_OTP_SET_VALUE`、`UF4_ID_OVP_SET_VALUE`、`UF4_ID_OCP_SET_VALUE`
  - 运行状态与测量显示：由 `READ/STREAM` 返回的状态类 ID 提供
- `Core/Inc/function.h` 中与本地面板耦合的旧接口声明 `Encoder()`、`Key_Process()`、`OLED_Display()` 和编辑态字段 `currentSetting/SET_bit` 已移除，避免后续误认为固件仍依赖本地按键/OLED 页面逻辑。

## 当前协议入口

- USB CDC RX：`USB_Device/App/usbd_cdc_if.c` -> `UF4Transport_OnUsbCdcRx()` -> `UF4_InputBuffer()`
- USART RX：`UF4Transport_RunTask()` 轮询 DMA 环形缓冲 -> `UF4_InputBuffer()`
- TX：`UF4_SendFrame()` -> `UF4Transport_SendBytes()` -> USB CDC 或 UART DMA
- 周期流：`UF4Transport_RunTask()` 每 20 ms 调用一次 `UF4_Process()`

## 验证记录

- 2026-06-13：`cmake --build E:\PROJECT_C\UF4DigitalPower\cmake-build-debug -- -j4` 构建通过，生成 `UF4DigitalPower.elf/.hex/.bin`。
- 2026-06-13：`E:\PROJECT_C\UF4COM\cmake-build-debug\UF4COM.exe` 自测通过。
