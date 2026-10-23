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


## ADC 触发配置


| ADCX | ADC TRIGGER SOURCE    | HRTIM TRIGGER SOURCE | 1ST TRIGGER SOURCE | VALUE | Post scaler | 备注   |
|------|-----------------------|----------------------|--------------------|-------|-------------|------|
| ADC1 | HRTIM Trigger 3 event | Master Timer         | COMPARE 3          | 13600 | 1           | 电流   |
| ADC2 | HRTIM Trigger 5 event | Master Timer         | COMPARE 2          | 13600 | 1           | VIN  |
| ADC3 | HRTIM Trigger 5 event | Master Timer         | COMPARE 2          | 13600 | 1           | VOUT |
| ADC4 | HRTIM Trigger 3 event | Master Timer         | COMPARE 3          | 13600 | 1           | VREF |
| ADC5 | SoftWare Trigger      |                      |                    |       |             | 温度   |



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


## COMP和 EEV配置


基于 RM0440的28.3.2章的Table 223


物理接口 PA0和 PA1的物理引脚全部连接到INA240A1的输出端，其实采样的都是一个电流信号
  PB11 和 PB14 同理

首先，正向电流和反向电流OTP事件全部由COMP事件触发，由于有两个电流方向，所以FORWARD和REVERSE封为不同的COMP极性和EEV触发极性

| F/R          | COMPX | OUT Polarity                       |
|--------------|-------|------------------------------------|
| FORWARD_IIN  | COMP3 | COMP output is inverted |
| FORWARD_IOUT | COMP7 | COMP output is inverted |
| REVERSE_IIN  | COMP1 | COMP output on GPIO is inverted    |   
| REVERSE_IOUT | COMP6 | COMP output on GPIO is inverted    |    


COMP和EEVX对应图表

| COMPX | COMP Polarity | COMP Event       | EEVX  | Event Polarity                | Event Sensitivity                 |
|-------|---------------|------------------|-------|-------------------------------|-----------------------------------|
| COMP1 | inverted      | REVERSE_IIN OCP  | EEV4  | External event is active high | External event is active on level |
| COMP6 | inverted      | REVERSE_IOUT OCP | EEV3  | External event is active high | External event is active on level |
| COMP3 | inverted      | FORWARD_IIN OCP  | EEV5  | External event is active low  | External event is active on level |
| COMP7 | inverted      | FORWARD_IOUT OCP | EEV10 | External event is active low  | External event is active on level |
| COMP2 | NOT inverted  | VIN OVP          | EEV1  | External event is active high | External event is active on level |
| COMP4 | NOT inverted  | VOUT OVP         | EEV2  | External event is active high | External event is active on level |


> 在SRC2来源中，ADCX_AWD不能做为Source，只能配置COMP，所以ADC5的温度只能软件OTP

RM0440 Rev 9 第 768 页 Figure 168 中，HRTIM 分支连接在 COMP 的 POLARITY
选择器之前，因此 EEV 使用原始 VALUE，不能按 GPIO 上反相后的电平配置。
正向电流时 INA240 输出低于 VREF165，COMP3/COMP7 使用 DAC 低阈值：输入低于
阈值时原始 VALUE 为低，触发低有效的 EEV5/EEV10。反向电流使用 DAC 高阈值，
超过阈值时 COMP1/COMP6 原始 VALUE 为高，触发高有效的 EEV4/EEV3。
COMP2/COMP4 电压保护仍为高有效。软件保护也读取原始 VALUE，与该表一致。

## HRTIM 配置

|                              | Timer E                | Timer E                |
|------------------------------|------------------------|------------------------|
| Period                       | 27200                  | 27200                  |
| Period Enable                | √                      | √                      |
| Repetition Update            | √                      | √                      |
| Fault to Enable              | √                      | √                      |
| Output Config 1 Set Source   | Master Timer period    | Master Timer period    |
| Output Config 1 Reset Source | Master Timer Compare 4 | Master Timer Compare 1 |
| Fault Level                  | Inactive               | Inactive               |
| Dead Time Insertion          | √                      | √                      |
|                              |                        |                        |



## FAULT配置
不启用单独的FAULT配置，而是用EEV事件代替FAULT

Output1 Configuration

Polarity：Active HIGH
Set Source Selection：1
- 1st Set Source：
  - Master Timer period event forces the output to its active state
Reset Source Selection：7
- 1st Reset Source：
  - Master Timer compare 4 event forces the output to its inactive state
- 2nd Reset Source：
  - External event 1 forces the output to its inactive state
- 3rd Reset Source：
  - External event 2 forces the output to its inactive state
- 4th Reset Source：
  - External event 3 forces the output to its inactive state
- 5th Reset Source：
  - External event 4 forces the output to its inactive state
- 6th Reset Source：
  - External event 5 forces the output to its inactive state
- 7th Reset Source：
  - External event 10 forces the output to its inactive state

```EEV1  → VIN OVP
EEV2  → VOUT OVP
EEV3  → Reverse IOUT OCP
EEV4  → Reverse IIN OCP
EEV5  → Forward IIN OCP
EEV10 → Forward IOUT OCP
```

```
Output1
────────────────────────────

Polarity:
Active HIGH

Set Sources:
[1]
└─ Master Timer period

Reset Sources:
[7]
├─ Master Timer Compare 4
├─ External Event 1
├─ External Event 2
├─ External Event 3
├─ External Event 4
├─ External Event 5
└─ External Event 10

Idle Mode:
The output is not affected by the burst mode operation

Idle Level:
Output at inactive level when in IDLE state

Fault Level:
Output at inactive level when in FAULT state

Chopper:
Disabled

Burst Mode Entry Delayed:
Disabled
```
- 物理接口**PA0**和**PA1**的物理引脚全部连接到**INA240A1的输出端**，其实采样的都是一个电流信号
**PB11** 和 **PB14** 同理
- EEV10的COMP7使用软件或者HRTIM 置位触发锁存




## FMAC 100 kHz 电流内环设计

本文说明 STM32G474 如何用 FMAC 执行输出端口电流环。FMAC 的 IIR 单元实际执行递推差分方程，输入是归一化电流误差，输出是归一化占空比扰动。功率级没有电感电流传感器，电感电流只作为模型内部状态。

设计依据：

- `Docs/datesheet/AN5788.pdf`
- `Docs/datesheet/RM0440.pdf` 第 18 章 FMAC
- `Docs/BSP_CONFIG.md`
- `Docs/fmac_2p2z_current_loop.m`

## 1. 工程时序

板级设计中的开关频率为 200 kHz，电流内环执行频率为 100 kHz。因此电流控制每 2 个 PWM 周期执行一次：

```text
F_pwm   = 200 kHz
F_loop  = 100 kHz
T_loop  = 10 us
```

ADC 采样必须由 HRTIM 定时触发，采样点放在开关噪声较小的位置。当前工程使用 HRTIM Master Compare 3 触发 `ADC1` 和 `ADC4`，比较值为 `13600`。这个触发点需要结合实际 HRTIM 周期确认，不能只按寄存器数值假设采样位置正确。

当前工程的 `ADC1` 扫描顺序为：

```text
ADC1 DMA[0] = FORWARD_IIN
ADC1 DMA[1] = REVERSE_IIN
ADC1 DMA[2] = REVERSE_IOUT
ADC1 DMA[3] = FORWARD_IOUT
```

正向和反向电流环不能共用同一套 FMAC 历史状态。FMAC 的 X1、Y 缓冲区保存上一组输入和输出，两个方向切换时必须停机、清空或重新预装载对应状态。

## 2. 功率级和采样模型

`BSP_CONFIG.md` 给出的主要参数为：

```text
Vbus 标称值       48 V
实际输入电压范围 12...70 V
实际输出电压范围 1...70 V
电感 L            15 uH
电感电阻 RL       6.5 mOhm
输入/输出电容     470 uF
输入/输出电容 ESR 25 mOhm
电流采样电阻      8 mOhm
INA240A1 增益     20 V/V
硬件过流峰值      11 A
软件持续电流      10 A
```

这是四开关双向 Buck-Boost，正向和反向工作时功率级增益不同。电流内环设计至少要分别确认以下工况：

```text
正向 Buck
正向 Boost
反向 Buck
反向 Boost
```

不能只用一个标称点的 `Vbus/(L*s+RL)` 模型就认为四个工况都满足裕量。第一版可以用该模型计算初始系数，最终系数应通过每个方向、每个主要输入电压点的实测小信号响应校核。

电流采样使用 VREF165 做零点校准，不要在控制代码中固定使用 1.65 V：

```text
正向：I = (VREF165 - VADC) / (0.008 * 20)
反向：I = (VADC - VREF165) / (0.008 * 20)
```

电流环送入 FMAC 的不是 ADC 原始值，而是经过零点扣除、方向处理和缩放后的误差：

```text
e[n] = Iref[n] - Imeas[n]
```

正向使用 `FORWARD_IOUT`，反向使用 `REVERSE_IOUT`。固件先按物理方向把输出电流换算成当前局部 source-to-load 方向的正值，再计算误差。反向功率级与正向功率级完全对称，因此两种方向共用同一张局部工作点调度表；切换方向时必须清空或预装载 FMAC 的历史状态。

## 3. 2P2Z 控制器方程

AN5788 使用的 2P2Z 形式为：

```text
u[n] = A1*u[n-1] + A2*u[n-2]
     + B0*e[n]  + B1*e[n-1] + B2*e[n-2]
```

其中：

```text
e[n]：归一化电流误差
u[n]：归一化 PWM 调制量
B0~B2：前馈系数
A1~A2：反馈系数
```

这就是 FMAC IIR Direct Form 1 的计算形式。系数区的顺序必须是：

```text
X2 = [B0, B1, B2, A1, A2]
```

如果从普通传递函数写法开始：

```text
C(z) = (b0 + b1*z^-1 + b2*z^-2)
       --------------------------------
       (1 + d1*z^-1 + d2*z^-2)
```

则装入 FMAC 的反馈系数是：

```text
A1 = -d1
A2 = -d2
```

如果系数由 ST-WDS 的 `FMAC (Fixed Point)` 页面直接导出，则按 ST-WDS 输出值使用，不要再次修改 A 系数符号。AN5788 中给出的 A 系数已经是 FMAC 方程所需的形式。

## 4. Type-II 补偿器设计

连续域可以先使用下面的 Type-II 结构：

```text
C(s) = K * (1 + s/wz1) * (1 + s/wz2)
             -----------------------------
             s * (1 + s/wp)
```

设计步骤：

1. 测量或建立四个工作方向下的电流功率级 plant。
2. 根据 plant 的极点和零点放置 `wz1`、`wz2`。
3. 将 `wp` 放在交越频率以上，用于衰减开关频率附近的增益。
4. 根据目标交越频率求 `K`。
5. 用 Tustin 变换离散化，得到 `B0~B2` 和 `A1~A2`。
6. 用离散模型检查相位裕量、增益裕量和采样延迟。
7. 将系数转换为 FMAC 定点格式。

AN5788 建议数字环路的目标交越频率满足：

```text
Fc < Fs / 20
```

对于 100 kHz 电流环，交越频率必须根据全范围功率级和邻域调度验证结果确定。当前最终设计使用每个工作点的积分增益调度，最慢邻域交越频率约 7.48 Hz，验收时至少检查：

```text
相位裕量 PM >= 50 deg
增益裕量 GM >= 6 dB
```

实际设计不能只看 PM。若 PM 很大但 GM 很小，说明高频仍有额外增益交越，器件偏差和延迟可能使系统变得不稳定。

最终脚本 `Docs/simulation/current_loop_robust_design_final.m` 会依次生成并检查：

- `current_loop_robust_integral_100k.csv`：BUCK/BOOST 的逐工作点积分增益和 Q15 系数；
- `current_loop_robust_midpoint_check`：相邻调度点之间的中点功率级；
- `current_loop_robust_step_validation`：输出端口电流的非线性阶跃；
- `current_loop_robust_step_validation_100k.csv`：代表性点的阶跃结果。

当前模型检查结果为 7536/7536 个 midpoint 邻域组合通过，最小 PM 为 64.29 deg，最小 GM 为 6.28 dB；5 个代表性非线性阶跃均未饱和，最终误差小于数值容差。最慢邻域交越频率约 7.48 Hz，因此 10 kHz 是电压环执行率，不代表电压环可以使用 10 kHz 交越频率；电压环仍需单独设计和验证。

固件实现位于 `User/1_Controller/Loop/current_loop.c`。100 kHz 采样点构造 `current_loop_sample_t` 后调用 `CurrentLoop_Update()`，返回 Q15 占空比。`vref_adc` 必须来自实时 VREF165 采样，不能在控制函数中固定为 1.65 V。`duty_base_q15` 由功率级调制器或电压环提供，电流环只叠加归一化占空比扰动。`CurrentLoop_SetMode()` 在 FWD/REV 或 BUCK/BOOST 切换时清除积分历史。

`CurrentLoop_Update()` 和调度查表路径放在 `.RamFunc`，链接脚本将该段作为初始化数据复制到 SRAM；Q15 调度表保留在 Flash。电流环内部使用输出电流传感器，不读取或估算电感电流作为反馈。

当前运行路径由 CPU 执行上述定点递推，FMAC 外设只完成初始化，本文的 FMAC 配置用于说明硬件实现方式。`CurrentLoopSchedule_B0()` 遇到无效格时，按相同有效电流前缀的连续输出电压行分为 47 个区间；正常控制范围内最多检查 47 个候选，保持原系数、距离权重和等距离时的取舍，不再每次扫描 3744 个格。`current_loop_schedule.c` 使用 GCC 单文件 `O3` 优化，工程其余 Debug 编译仍为 `-Og`，调度函数继续在 SRAM 执行。

调度优化固件的故障前后短时实机采样记录中，TIM6 外环 ISR 最大值为 10327 cycles，170 MHz / 10 kHz 的预算为 17000 cycles，`s_outer_isr_overrun_count` 为 0；该周期统计包含高优先级中断抢占。这只说明该短测窗口内没有外环超时，不代表长时间运行或新电压占空比 PI 已通过实机验证。

## 8. 10 kHz 电压 PI 外环

外环实现位于 `User/1_Controller/Loop/voltage_loop.c`。每 100 us 读取经板级分压标定的输出电压。`VoltageLoop_Update()` 以毫伏为单位计算 0...10 A 的本地输出电流参考，传给 100 kHz `CurrentLoop_SetReference_mA()` / `current_loop_sample_t.iref_mA`；误差采用 `Vref - Vout`。功率方向由运行模式选择，电流参考始终是当前方向上的非负输出电流幅值。当前 CV 修复另用 `VoltageLoop_DutyLimit()` 直接根据电压误差限制占空比。

电流参考 PI 的默认参数按理想 470 uF 输出电容计算，正确量纲为：

```text
F_voltage = 10 kHz
Kp        = 97 / 2^16 mA/mV
Ki*Ts     = 100 / 2^30 mA/mV/sample
Iout_ref  = 0...10 A
```

原说明中的 `96900` 和 `100000` 将这两项增益放大了约 1000 倍，不能用它们说明 0.50 Hz 交越频率。上述更正后的 Kp 约为 0.001480 mA/mV；若内环调节的是流入理想输出电容的净电流，则交越频率约 0.50 Hz，PI 零点约 0.10 Hz。这两项频率只适用于该理想电容电流源模型。

本板输出 shunt 位于 Cout 外侧，测到的是端口负载电流，空载时电容充电电流不经过该反馈。空载输出电压升高时，端口电流仍可接近零，因此“电压 PI 给电流参考，端口电流内环调占空比”的单独级联无法实现空载稳压。修正上述量纲不会消除这个观测问题，需要独立的电压 PI 占空比限定。电流参考 PI 保留条件积分抗饱和和 `VoltageLoop_SetGains_Q16_Q30()` / `VoltageLoop_SetCurrentLimit_mA()` 接口。

当前新增 `VoltageLoop_DutyLimit()` 使用电压误差构造等效变换电压，再映射为占空比上限：

```text
e[n]       = Vref[n] - Vout[n]                  // mV
Kp         = 655 / 2^16                        // mV/mV
Ki*Ts      = 337326 / 2^30                     // mV/mV/sample
Iv[n]      = Iv[n-1] + Ki*Ts * e[n]            // mV，条件积分
Vcmd[n]    = Vref[n] + Kp * e[n] + Iv[n]       // mV
BUCK:      Vcmd 限制到 0...Vin，Dcv = Vcmd / Vin
BOOST:     Vcmd 限制到 Vin...70 V，Dcv = 1 - Vin / Vcmd
D_applied  = min(D_current, Dcv)               // 再执行 PWM 合法范围限幅
```

电压比例项和积分量以 Q16 mV 运算，`Ki*Ts` 以 Q30 存储；这组增益用于直接电压占空比限定，不能套用前述 0.50 Hz 理想电容级联结论。电压 PI 在电流限定且误差为正、或继续推向变换电压上下限时停止积分。软启动前 100 ms 同时把目标电压和电流上限从零线性爬升到设定值。100 kHz 内环取电流控制占空比和电压上限的较小值，并在限幅时反算、裁剪积分状态以跟踪实际占空比，避免电压限定期间电流积分继续上升。

当前 CV 修复正在主机测试，尚无实机稳压验证；BOOST 占空比映射也未经实机验证。负载变化、空载启动、有效电容量、内外环交互、占空比跟踪范围及板级电压标定仍需实机闭环检查。更新函数和 PI 算术位于 `.RamFunc`，辅助限幅函数内联。

## 5. FMAC 定点格式

FMAC 的输入、输出和系数使用 Q1.15，累加器为 Q4.22。FMAC 计算结果为：

```text
Y = 2^R * (B*X + A*Y_delay)
```

`R` 的范围为 `0~7`。当某个系数的绝对值大于 1 时，应把 B、A 系数同时缩小 `2^R`，并设置相同的 FMAC `R` 进行补偿。

Q1.15 转换：

```c
q15 = round(value * 32768.0f);
```

限制范围：

```text
-1.0 <= value <= 0.999969
```

FMAC 的 `CLIPEN` 只能防止输出超出 Q1.15，它不是电流控制器的抗饱和功能。PWM 输出达到上下限时，必须在控制状态机中处理给定、模式切换和积分等效状态。

`pre_shift` 和 `post_shift` 要和 ADC、PWM 的实际数值范围配套。建议使用 ST-WDS 的 `FMAC (Fixed Point)` 页面生成这两个值；如果用 MATLAB 计算系数，也要在脚本中明确输入和输出的归一化基准。

## 6. FMAC 内存配置

2P2Z 的参数为：

```text
N = 3     // B0, B1, B2
M = 2     // A1, A2
P = 3
Q = 2
```

RM0440 对 IIR 的最小缓冲区要求为：

```text
X1_BUF_SIZE = N + d1
X2_BUF_SIZE = N + M
Y_BUF_SIZE  = M + d2
```

一种不重叠的布局为：

```text
X2_BASE = 0
X2_SIZE = 5

X1_BASE = 5
X1_SIZE = 4

Y_BASE  = 9
Y_SIZE  = 3
```

对应 HAL 配置：

```c
cfg.Filter             = FMAC_FUNC_IIR_DIRECT_FORM_1;
cfg.P                  = 3;
cfg.Q                  = 2;
cfg.R                  = fmac_r;
cfg.CoeffBSize         = 3;
cfg.CoeffASize         = 2;
cfg.CoeffBufferSize    = 5;
cfg.InputBufferSize    = 4;
cfg.OutputBufferSize   = 3;
```

启动前需要预装载：

```text
X2：B0, B1, B2, A1, A2
X1：初始误差历史值
Y ：初始控制量历史值
```

若 X1 或 Y 的历史值没有初始化，控制器启动时会使用未定义的延迟状态，可能出现一个很大的 PWM 脉冲。

## 7. 100 kHz 执行流程

推荐控制链路如下：

```text
HRTIM 触发 ADC
    -> ADC 完成采样并保存 DMA 数据
    -> 用 VREF165 扣除电流零点
    -> 根据正向/反向选择电流符号
    -> 计算 e[n] = Iref - Imeas
    -> 将 e[n] 缩放到 FMAC 输入范围
    -> 写入 FMAC X1 或触发 FMAC DMA
    -> 读取 FMAC Y 输出
    -> 还原输出缩放并限制 PWM 范围
    -> 更新 HRTIM 比较寄存器
```

FMAC 可以通过 DMA 读写 X1/Y，但 ADC 原始值不能直接作为 FMAC 输入，因为 FMAC 不负责：

- VREF165 零点扣除；
- 正反向方向判断；
- 电流给定和反馈相减；
- PWM 限幅；
- 过流保护和软启动。

这些功能仍由 CPU、DMA 配置、COMP 和 HRTIM Fault 配合完成。硬件 OCP 必须独立于 FMAC 保留。

## 8. 系数生成文件

工程提供了 MATLAB 设计脚本：

```text
Docs/fmac_2p2z_current_loop.m
```

脚本用于：

- 配置 100 kHz 电流环和 200 kHz PWM；
- 输入电感、等效电阻、母线电压和采样缩放；
- 使用 `current_loop_robust_design_final.m` 生成积分型 2P2Z 调度表；
- Tustin 离散化；
- 生成 FMAC 的 B/A 系数、Q15 数值和 `R`；
- 生成 RM0440 的 X1/X2/Y 内存布局；
- 导出 `fmac_2p2z_coefficients.h`。

脚本中的电感和电阻来自 `BSP_CONFIG.md`，但四开关 Buck-Boost 的四个工作方向仍应分别用实测 plant 校核。生成的系数不能绕过硬件调试直接固化到量产参数中。
