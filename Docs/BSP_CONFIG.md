# 板级配置文件

## 1. 概述

板级配置文件用于配置硬件引脚信息、功能等。

### 1.1 引脚配置

| 引脚   | 配置/功能                           | 信号/网络        | 备注                             |
|------|---------------------------------|--------------|--------------------------------|
| PF0  | OSC_IN                          | RCC_OSC_IN   | 外部晶振输入                         |
| PF1  | OSC_OUT                         | RCC_OSC_OUT  | 外部晶振输出                         |
| PA0  | GPIO_Analog ADC1_IN1 COMP3_INP  | FORWARD_IIN  | ADC1_IN1 / COMP3_INP，正向输入电流采样  |
| PA1  | GPIO_Analog ADC1_IN2 COMP1_INP  | REVERSE_IIN  | ADC1_IN2 / COMP1_INP，反向输入电流采样  |
| PA2  | USART2_TX                       |              | USART2 发送                      |
| PA3  | USART2_RX                       |              | USART2 接收                      |
| PA5  | GPIO_Input                      | FAN_CESU     | 风扇 CESU 输入                     |
| PA6  | TIM16_CH1                       | FAN_PWN      | 原文为 FAN_PWN，疑似 FAN_PWM         |
| PA7  | GPIO_Analog ADC2_IN4 COMP2_INP  | VIN          | ADC2_IN4 / COMP2_INP，输入电压采样    |
| PC4  | USART1_TX                       |              | USART1 发送                      |
| PC5  | USART1_RX                       |              | USART1 接收                      |
| PB0  | GPIO_Analog COMP4_INP ADC3_IN12 | VOUT         | COMP4_INP / ADC3_IN12，输出电压采样   |
| PB1  | GPIO_Output                     | LED1         | LED1 输出                        |
| PB2  | GPIO_Output                     | LED2         | LED2 输出                        |
| PB11 | GPIO_Analog COMP6_INP ADC1_IN14 | REVERSE_IOUT | COMP6_INP / ADC1_IN14，反向输出电流采样 |
| PB12 | ADC4_IN3                        | VREF165      | ADC4_IN3，原文 VREF165            |
| PB14 | GPIO_Analog COMP7_INP ADC1_IN5  | FORWARD_IOUT | COMP7_INP / ADC1_IN5，正向输出电流采样  |
| PC6  | HRTIM1_CHF1                     |              | HRTIM1 通道 CHF1，正向通道的 BOOST H   |
| PC7  | HRTIM1_CHF2                     |              | HRTIM1 通道 CHF2，正向通道的 BOOST L   |
| PC8  | HRTIM1_CHE1                     |              | HRTIM1 通道 CHE1，正向通道的 BUCK H    |
| PC9  | HRTIM1_CHE2                     |              | HRTIM1 通道 CHE2，正向通道的 BUCK L    |
| PA8  | ADC5_IN1                        | TEMP2        | 温度采样 2                         |
| PA9  | ADC5_IN2                        | TEMP1        | 温度采样 1                         |
| PA11 | USB_DM                          |              | USB DM                         |
| PA12 | USB_DP                          |              | USB DP                         |
| PA13 | SYS_JTMS-SWDIO                  |              | SWDIO 调试接口                     |
| PA14 | SYS_JTCK-SWCLK                  |              | SWCLK 调试接口                     |
| PC10 | SPI3_SCK                        | FLASH_SCLK   | SPI3 时钟，Flash SCLK             |
| PC11 | SPI3_MISO                       | FLASH_MISO   | SPI3 MISO，Flash MISO           |
| PC12 | SPI3_MOSI                       | FLASH_MOSI   | SPI3 MOSI，Flash MOSI           |
| PD2  | GPIO_Output                     | FLASH_NSS    | Flash 片选 NSS                   |

### 1.2 补充说明

- `FORWARD_IIN` 和 `REVERSE_IIN` 分别是正向和反向输入电流采样信号，物理上连接到同一个 INA240A1 的输出。
- `FORWARD_IOUT` 和 `REVERSE_IOUT` 分别是正向和反向输出电流采样信号，物理上连接到另一个 INA240A1 的输出。
- `VIN`、`VOUT` 为输入/输出电压采样信号，方向随工作模式切换。
- `VREF165` 是参考电压采样信号。
- `TEMP1` 和 `TEMP2` 是温度采样信号。

双向 Buck-Boost 升降压电路，使用四开关典型拓扑。

无法采样电感电流，所有电流均通过采样电阻采样，采样电阻压降经 INA240A1 进行中点 1.65V 偏置放大后送入 MCU 的 ADC 口。

拓扑大概为：

`输出/输入端子 - RS - C - MOS - L - MOS - C - RS - 输入/输出端子`

MOS 有单独的高侧驱动电路，可实现 100% 占空比驱动。

输入输出采样均在输入输出端子上，使用差分采样电路，增益为 `RU/RL = 75K/3K = 25` 倍缩小后进入 ADC。

电流采样使用 INA240A1，增益为 `20V/V`，输入电流采样电阻为 `0.008Ω`，输出电流采样电阻为 `0.008Ω`。输入输出电流均用 INA240A1 采样。

单向电流为 0 时，INA240A1 输出 1.65V；ADC 采样之后减去 `VREF165` 的采样值应为 0，`VREF165` 不使用硬编码。

### 2026-09-28 供电条件诊断记录

- 未给功率级供电时，CMSIS-DAP 的 VTREF 为 3.302 V，ADC4 `VREF165` 约 2729/4095，即约 2.20 V；四路零电流采样与其接近。当时 DAC 高阈值饱和到 4095 码，固件正确锁定输出。该读数不能单独判定参考电路硬件故障。
- 功率级供电、外部驱动 EN 硬件拉低后，VTREF 为 3.300 V，ADC4 `VREF165` 约 2048/4095，即约 1.65 V；四路电流采样约 2043-2052 码。10 A OCP 的 DAC 高/低输出寄存器分别为 4031 和 61 码，按 3.3 V 估算约 3.25 V 和 0.049 V，符合 8 mΩ、INA240A1 增益 20 所需的 1.60 V 检测摆幅。比较器状态无触发，固件处于 IDLE，PC6-PC9 为低。
- 固件使用实测 ADC4 零点设置 DAC 阈值，并检查 1.65 V 基准范围与双向阈值余量。输出禁用、使能请求为 0、采样稳定且四路电流读数接近零点时，可重新设置 DAC；原故障仍保持锁存，须按清故障流程处理。此处验证的是 DAC 寄存器码值，尚未测量模拟输出端电压或动态重新设定过程。
- 无功率级供电、外部驱动禁用时，两次只读计数相隔约 1 s，HRTIM 与 TIM6 分别增加约 101111 和 10111 次，符合 100 kHz/10 kHz。PC6-PC9 均为低电平。该结果只验证调度和关断状态，未验证门极波形、桥臂映射或死区时间。
- `CONTROL_POWER_STAGE_VALIDATED` 仍为 0。还需用示波器分别验证 TE1/TE2、TF1/TF2 的互补关系和死区，以及任一 COMP/EEV 故障触发时四路驱动均进入安全态。当前 EEV 仅复位输出 1；死区互补生成输出 2 时，其故障关断行为不能从配置推定。

- 当 IN+ 引脚相对于 IN- 引脚为负时，输出电压降低到低于基准电压。
- 当 IN+ 引脚相对于 IN- 引脚为正时，输出电压增加到高于基准电压。

正向电流时：
- `FORWARD_IIN` 采样输入电流，`FORWARD_IOUT` 采样输出电流；
- `VIN` 采样输入电压，`VOUT` 采样输出电压；
- 此时 E 桥臂为 BUCK 桥臂，F 桥臂为 BOOST 桥臂；
- IIN 的输入 ADC 采样值小于 VREF 的 1.65V，`I = (VREF165 - IIN) / 0.008 / 20`；
- IOUT 同理。

反向电流时：
- `REVERSE_IIN` 采样输入电流，`REVERSE_IOUT` 采样输出电流；
- `VIN` 采样输出电压，`VOUT` 采样输入电压；
- 此时 E 桥臂为 BOOST 桥臂，F 桥臂为 BUCK 桥臂；
- IIN 的输入 ADC 采样值大于 VREF 的 1.65V，`I = (IIN - VREF165) / 0.008 / 20`；
- IOUT 同理。

当前控制设计按输入 12...70 V、输出 1...70 V 建模；单向最大电流 11A 为硬件 OCP 峰值，软件持续电流限制为 10A。实际硬件耐压和 OVP 门限仍由器件与保护设计单独确认。


## 2. 设计描述

| 参数        | 数值                    | 单位  | 备注                            |
|-----------|-----------------------|-----|-------------------------------|
| 拓扑        | Buck-boost            | -   | 升降压拓扑                         |
| 输入电压最大值   | 72                    | V   | Input supply max              |
| 输入电压最小值   | 7                     | V   | Input supply min              |
| 输出最大电流    | 10                    | A   | Output maximum Current        |
| 电压纹波（峰峰值） | 0.5                   | %   | Voltage ripple (peak-to-peak) |
| 电压过冲      | 50                    | mV  | Voltage overshoot             |
| 需求效率      | 92                    | %   | Demand efficiency             |
| 控制模式      | PI 浮点电压外环 + FMAC 电流内环 | -   |                               |
| 开关频率      | 200                   | kHz | Switching frequency           |
| 最大占空比限制   | 100                   | %   | Maximum duty limit            |
| 最小占空比限制   | 0                     | %   | Minimum duty limit            |

## 3. 器件配置

| 器件        | 型号                                   | 数值    | 备注     |
|-----------|--------------------------------------|-------|--------|
| C_IN      |                                      | 470uF |        |
| C_IN_R    |                                      | 25mR  |        |
| C_OUT     |                                      | 470uF |        |
| C_OUT_R   |                                      | 25mR  |        |
| MOS       | [NCEP1545G](datesheet/NCEP1545G.pdf) |       | 具体看规格书 |
| RS_IN/OUT | R1225                                | 8mR   |        |
| L         | PQ2012                               | 15uH  |        |
| L_R       | PQ2012                               | 6.5mR |        |
| L_饱和电流    | PQ2012                               | 13A   |        |
| L_温升电流    | PQ2012                               | 16A   |        |
| 栅极驱动器     | [UCC27211](datesheet/UCC27211.pdf)   |       | 具体看规格书 |
