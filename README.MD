# F4CP / UF4DigitalPower 通信命令表

文档是上位机 `F4CP` 与功率固件 `UF4DigitalPower` 之间 TVLCOM 通信命令的统一契约。修改通信命令、数据类型、单位或访问权限时，先改这份表，再同步：

本项目作为一个脱机的`UF4DigitalPower` 上位机，可以使用串口发送到BLE与上位机联系
## 帧格式

```text
SOF(2) + Length(2, little-endian) + CMD(1) + SEQ(1) + Payload(N) + CRC16(2, little-endian)
```

- `SOF`: `0xAA 0x55`
- `Length`: `CMD + SEQ + Payload` 的字节数
- `SEQ`: 请求方递增序号；固件响应必须回同一个 `SEQ`
- `CRC16`: Modbus CRC16，初值 `0xFFFF`，多项式 `0xA001`，little-endian
- `Payload`: TLV 列表，格式为 `Type(1) + Length(2, little-endian) + Value(N)`

## 命令码

| 名称 | 值 | 方向 | Payload | 说明 |
| --- | ---: | --- | --- | --- |
| `ACK` | `0x00` | 固件 -> 上位机 | TLV 列表或空 | 请求成功响应。`READ` 成功时返回请求的数据；`WRITE` 成功时为空。 |
| `READ` | `0x01` | 上位机 -> 固件 | TLV 查询列表 | 每个 TLV 只填 `Type`，`Length=0`。 |
| `WRITE` | `0x02` | 上位机 -> 固件 | TLV 写入列表 | 每个 TLV 填 `Type`、长度和 little-endian 值。 |
| `REPORT` | `0x03` | 双向 | 请求空载荷；响应 TLV 列表 | 上位机发送空 payload 请求状态组，固件返回一帧 `REPORT`。 |
| `NACK` | `0xFF` | 固件 -> 上位机 | 空 | 命令、类型、长度、权限或载荷错误。 |

## 访问权限

| 名称 | 值 | 说明 |
| --- | ---: | --- |
| `READ` | `0x01` | 可读 |
| `WRITE` | `0x02` | 可写 |
| `READ_WRITE` | `0x03` | 可读可写 |

## 数据类型

| 名称 | Type | 长度 | 权限 | 单位 | 说明 |
| --- | ---: | ---: | --- | --- | --- |
| `INPUT_VOLTAGE` | `10` | 4 | READ | mV | 输入电压 |
| `INPUT_CURRENT` | `11` | 4 | READ | mA | 输入电流，非负值；小于 0 的测量结果不编码，按 0 上报 |
| `OUTPUT_VOLTAGE` | `12` | 4 | READ | mV | 输出电压 |
| `OUTPUT_CURRENT` | `13` | 4 | READ | mA | 输出电流，非负值；小于 0 的测量结果不编码，按 0 上报 |
| `CORE_TEMPERATURE` | `14` | 4 | READ | mC | 核心温度，摄氏度 x1000 |
| `BOARD_TEMPERATURE` | `15` | 4 | READ | mC | 板载温度，摄氏度 x1000 |
| `SET_VOLTAGE_LIMIT` | `17` | 4 | READ_WRITE | mV | 输出电压设定 |
| `SET_CURRENT_LIMIT` | `18` | 4 | READ_WRITE | mA | 输出电流设定 |
| `CC_CV_MODE` | `20` | 1 | READ | enum | `0=CC`, `1=CV` |
| `POWER_STATE` | `21` | 1 | READ_WRITE | bool | `0=OFF`, `1=ON` |
| `FAULT_STATE` | `22` | 4 | READ | bitmask | 故障位图 |
| `STATE_MACHINE_FLAG_BITS` | `23` | 1 | READ | bitmask | `0x01=INIT`, `0x02=WAIT`, `0x04=RISE`, `0x08=RUN`, `0x0F=ERR` |
| `STATE_MACHINE_STATE` | `24` | 1 | READ | enum | 当前功率拓扑：`0=NA`, `1=BUCK`, `2=BOOST`, `3=MIX` |
| `INPUT_VOLTAGE_RAW` | `25` | 4 | READ | adc | 输入电压 ADC 原始值 |
| `INPUT_CURRENT_RAW` | `26` | 4 | READ | adc | 输入电流 ADC 原始值 |
| `OUTPUT_VOLTAGE_RAW` | `27` | 4 | READ | adc | 输出电压 ADC 原始值 |
| `OUTPUT_CURRENT_RAW` | `28` | 4 | READ | adc | 输出电流 ADC 原始值 |
| `OTP_VALUE` | `29` | 4 | READ | mC | 当前过温保护参考值 |
| `OTP_SET_VALUE` | `30` | 4 | READ_WRITE | mC | 过温保护设定 |
| `OVP_VALUE` | `31` | 4 | READ | mV | 当前过压保护参考值 |
| `OVP_SET_VALUE` | `32` | 4 | READ_WRITE | mV | 过压保护设定 |
| `OCP_VALUE` | `33` | 4 | READ | mA | 当前过流保护参考值 |
| `OCP_SET_VALUE` | `34` | 4 | READ_WRITE | mA | 过流保护设定 |
| `DUTY_CMD` | `35` | 4 | READ | tick | 当前控制占空比命令 |
| `PWM_A_COMPARE` | `36` | 4 | READ | tick | 输入端 A 侧 PWM 比较值/占空比监控 |
| `PWM_D_COMPARE` | `37` | 4 | READ | tick | 输出端 D 侧 PWM 比较值/占空比监控 |
| `FAN_SPEED` | `38` | 4 | READ | permille | 风扇当前值，0 到 1000 |
| `FAN_SET_VALUE` | `39` | 4 | READ_WRITE | permille | 风扇设定值，0 到 1000 |
| `DEBUG_SNAPSHOT` | `40` | 0 | READ | virtual | 虚拟读取项。请求 `Type=40, Length=0`；响应返回控制环调试 TLV 组合。 |
| `LOOP_CURRENT_FEEDBACK` | `41` | 4 | READ | mA | 控制环输出电流反馈值 |
| `LOOP_CURRENT_REFERENCE` | `42` | 4 | READ | mA | 控制环输出电流参考/限流值 |
| `VOLTAGE_LOOP_CURRENT_REFERENCE` | `43` | 4 | READ | mV | 控制环电压参考值；名称保留历史兼容 |

## 状态组上报

上位机发送：

```text
CMD=REPORT, Payload = empty
```

固件返回：

```text
CMD=REPORT, Payload =
  TLV(INPUT_VOLTAGE)
  TLV(INPUT_CURRENT)
  TLV(OUTPUT_VOLTAGE)
  TLV(OUTPUT_CURRENT)
  TLV(CORE_TEMPERATURE)
  TLV(BOARD_TEMPERATURE)
  TLV(SET_VOLTAGE_LIMIT)
  TLV(SET_CURRENT_LIMIT)
  TLV(CC_CV_MODE)
  TLV(POWER_STATE)
  TLV(FAULT_STATE)
  TLV(STATE_MACHINE_FLAG_BITS)
  TLV(STATE_MACHINE_STATE)
  TLV(INPUT_CURRENT_RAW)
  TLV(OUTPUT_CURRENT_RAW)
  TLV(OTP_VALUE)
  TLV(OTP_SET_VALUE)
  TLV(OVP_VALUE)
  TLV(OVP_SET_VALUE)
  TLV(OCP_VALUE)
  TLV(OCP_SET_VALUE)
  TLV(DUTY_CMD)
  TLV(PWM_A_COMPARE)
  TLV(PWM_D_COMPARE)
  TLV(FAN_SPEED)
  TLV(FAN_SET_VALUE)
  TLV(LOOP_CURRENT_FEEDBACK)
  TLV(LOOP_CURRENT_REFERENCE)
  TLV(VOLTAGE_LOOP_CURRENT_REFERENCE)
```

## 常用请求

读取状态：

```text
CMD=READ, Payload = TLV(Type=10, Len=0) + TLV(Type=11, Len=0) + ...
```

写输出设定并开机：

```text
CMD=WRITE
Payload =
  TLV(Type=17, Len=4, Value=set_voltage_mv)
  TLV(Type=18, Len=4, Value=set_current_ma)
  TLV(Type=21, Len=1, Value=1)
```