# UF4DigitalPower 下位机固件说明

本仓库是 UF4 数字电源的下位机固件工程，面向基于 `STM32G474CBT6` 的四开关同步整流 Buck-Boost 数字电源控制板。

固件负责 ADC 采样、PWM 调制、Buck/Mix/Boost 模式切换、电压电流闭环、保护逻辑、USB CDC 通信、DAPLink 辅助烧录和固件版本发布。

> 注意： 由于焊接的水平不同和实际硬件的差异，作者不对任何人使用本项目造成的任何直接或间接损失负责，使用前请务必做好充分的风险评估和安全防护措施。
---

## 目录

- [1. 项目总览](#1-项目总览)
- [2. 硬件介绍](#2-硬件介绍)
- [3. 下位机 C 代码解析](#3-下位机-c-代码解析)
- [4. 借鉴代码与参考资料](#4-借鉴代码与参考资料)
- [5. 项目结构](#5-项目结构)
- [6. 快速开始](#6-快速开始)

---

## 1. 项目总览

本项目是一套基于 STM32G474 的双向四管升降压数字电源
- 设计输入: `5V - 40V ` `10AMAX`
- 输出范围: `0.5V - 40V`，
- 功率拓扑: 四开关同步整流 Buck-Boost
- 正反向电流均可以到0-10A


固件提供电压/电流设定、实时状态上报、保护阈值配置、调试量读取和固件版本信息接口，便于接入外部控制或调试工具。
> 板载了DAPLINK(STM32F103)，如果您需要在线烧录固件，你还需要将[DAPLink固件](https://github.com/UF4OVER/DAPLINK_STM32F103XB)烧录到STM32F103上

> 注意: 作者使用的MOS可能不够理想 ，因此在高压低压两端的效率和稳定性可能不够好，你在可以选择更适合的MOSFET以提升性能。

> 实物图
> ![IMG_20260604_213728.jpg](https://img.hepi.ng/v2/NCb81Dz.jpeg)
> ![IMG_20260604_213704.jpg](https://img.hepi.ng/v2/VqxCobD.jpeg)
> ![IMG_20260616_153259.jpg](https://img.hepi.ng/v2/3x1zBgu.jpeg)
> ![86ec51f830c4e1613f834f868d97668d.jpg](https://img.hepi.ng/v2/NemKAHw.jpeg)

## 2. 硬件介绍

> 当前作者测试使用到的硬件参数及其计算详情见 [计算记录](https://blog.hepi.ng/posts/overview_of_hardware_parameters_and_loops)
### 2.1 主控与系统定位

- 主控 MCU: `STM32G474CBT6`
- 板载DEBUG: `STM32F103C8T6（DAPLink）`
- 栅极驱动器: `UCC27211`
- 辅助电源1: VIN->5V `LMR36520F`  用于运放，基准电压和风扇供电
- 辅助电源2: 5V->3V3 `LM1117-3.3`  用于MCU 与外设控制逻辑供电
- 辅助电源3: 5V->11.1V `SY7304DBC` 用于栅极驱动器供电

当前板卡定位是四开关同步整流 Buck-Boost 数字电源控制板，下位机负责控制、采样、保护和通信接口。

### 2.2 功率级与工作模式

当前功率拓扑是四开关 Buck-Boost，固件中划分三种模式：

- `Buck`
  - 输入明显高于输出
  - Buck 桥主调制
  - Boost 桥承担同步整流或固定导通辅助角色
- `Mix`
  - 输入电压接近输出电压
  - 两侧桥臂共同参与调制
  - 用于平滑跨越 Buck 和 Boost 边界
- `Boost`
  - 输入明显低于输出
  - Boost 桥主调制
  - Buck 桥承担同步整流或固定导通辅助角色

- ![功率级电路图](https://img.hepi.ng/v2/WKNQnxV.png "功率级电路图")

当前 IOC 与固件约定中：

- `HRTIM Timer A` 对应 Buck 桥
- `HRTIM Timer D` 对应 Boost 桥
- `PA8/PA9` 为一组互补 PWM
- `PB14/PB15` 为另一组互补 PWM

### 2.3 栅极驱动与 Bootstrap 特性

项目使用 `UCC27211` 半桥驱动器。该器件高边驱动采用 bootstrap 供电思路：

- 高边供电使用 `HB-HS`
- 需要 `HB-HS` 自举电容,使用100nF的贴片电容
- 需要 `HS` 周期性回到较低电位，bootstrap 电容才能补电

这意味着在 `Mix` 或 `Boost` 模式中，如果某一侧高边长期接近常开，不给 bootstrap 刷新窗口，就可能出现高边驱动电压下跌，进而表现为：

- 模式不稳定
- 输入被突然拉低
- 无法顺利进入 `Mix` / `Boost`
- 模式反复抖动

因此控制代码中不能只看理论占空比，还必须兼顾驱动器刷新条件。
- ![6f4cab3df3eb4a79ca3ce2c1fd22781c.png](https://img.hepi.ng/v2/ajeCH7l.png)
### 2.4 采样链路

当前固件的主要采样量如下：

- `VIN`
- `IIN`
- `VOUT`
- `IOUT`
- `Board TEMP1`
- `Board TEMP2`
- `CPU TEMP`

采样与换算特性：

- 电压采样采用分压进入 ADC，再由软件恢复真实电压值，使用 GS8558-SR 运放
- 电流采样基于 `7 mΩ` 分流电阻、`20x` 放大增益和 `1.65 V` 中点偏置,使用INA240A1 进行差分采样并放大
- `ADC1` 规则组用于 `VIN/IIN/VOUT/IOUT`
- 温度由 `ADC2/ADC3/ADC5` 配合 `temp.c` 完成换算 使用10k NTC热敏电阻

- ![电压采样](https://img.hepi.ng/v2/pWFbBcN.png)
- ![电流采样与基准](https://img.hepi.ng/v2/tMQ0m25.png)
- ![温度采样](https://img.hepi.ng/v2/X8Ft1Mr.png)

### 2.5 辅助硬件

- 外部参数存储：`W25Q64`
- 风扇：`TIM8_CH3` PWM 控制
- 三色 LED：红 / 黄 / 绿状态指示
- `DIV_SW`、驱动使能等 GPIO 作为板级辅助控制


### 2.6 PCB

- ![顶层](https://img.hepi.ng/v2/PldqpuZ.png)
- ![地层](https://img.hepi.ng/v2/zlATTa3.png)
- ![电层](https://img.hepi.ng/v2/UT0qUFB.png)
- ![底层](https://img.hepi.ng/v2/nbD0gaq.png)

### 效率实测
![img.png](https://img.hepi.ng/v2/6RHUOSX.png)
---

## 3. 下位机 C 代码解析

本章对应当前实际参与运行的 C 代码。

### 3.0 代码简介

固件主体是一个面向四开关 Buck-Boost 数字电源的实时控制程序。代码按职责可以分为五层：

1. 板级外设层
   - 由 `adc.c`、`hrtim.c`、`tim.c`、`gpio.c`、`spi.c`、`usart.c` 等文件完成。
   - 负责 ADC 触发采样、HRTIM 互补 PWM、死区、风扇 PWM、Flash、USB CDC 和串口等底层资源初始化。
2. 采样与物理量换算层
   - 主要在 `function.c` 中的 `ADCSample()` 和 `ADC_calculate()`。
   - `ADC1` 采样 `VIN / IIN / VOUT / IOUT`，再经过校准参数、分压比例和电流采样比例换算成实际电压电流。
3. 状态机与保护层
   - 主要在 `function.c`。
   - `StateM()` 管理 `Init / Wait / Rise / Run / Err` 主状态。
   - `ShortOff()`、`OVP()`、`OCP()`、`OTP()` 负责短路、过压、过流、过温保护。
   - `StateMRise()` 负责软启动，当前输出参考从 0 开始爬升，同时逐步放开 Buck/Boost 最大占空限制，降低启动输入电流过冲。
4. 模式判定与切模准备层
   - 主要在 `BBMode()` 和 `PowerControl_PrepareModeSwitch()`。
   - 固件根据实际输出电压和输入电压关系切换 `Buck / Mix / Boost`。
   - 模式判定带回差，避免在临界点抖动。
   - 切入 Mix 时，Boost 支路和 PID 种子从当前硬件状态起步，避免理论占空直接注入造成突跳。
5. 快速闭环控制层
   - 主要在 `pid.c` 的 `BuckBoostVILoopCtlPID()`。
   - 由 HRTIM 快环中断调用，负责电流环、电压环、anti-windup、模式对应 duty 计算和 HRTIM 比较值刷新。
   - Buck、Mix、Boost 分别有不同的输出路径；Mix 区有独立补偿器初值、专用限幅和 anti-windup。
   - 电压环采样加入轻量 IIR 滤波，`u0/u1` 内部状态会按当前模式可实现占空范围限幅，避免隐藏积分跑飞。
   - 电流环加入 CC/CV 滞回、释放保持时间和更温和的 Vref 拉低/释放步进，减少限流边界抖动。

当前固件的主要实时节拍如下：

```text
TIM6  1 ms   -> 毫秒计数、喂狗
TIM7  5 ms   -> ADC 平均、保护、主状态机、模式判定、UF4 流 tick
TIM16 10 ms  -> ADC 平均值换算成 VIN/VOUT/IIN/IOUT 浮点上报值
HRTIM 200kHz -> BuckBoostVILoopCtlPID() 快速闭环与 PWM 刷新
```

通信接口由 `uf4_transport.c` 承担。它把 `UF4COM V3` 数据 ID 映射到固件内部变量，支持参数设置、状态读取、实时流数据、固件版本信息和调试量上报。

### 3.1 总体执行结构

下位机执行节拍分为三层：

1. `main.c`
  - 负责所有外设初始化
  - 启动 ADC、DMA、USB、定时器、HRTIM、看门狗
2. `TIM7` 5 ms 中断
  - 负责采样平均、保护、主状态机推进、模式判断、协议流 tick
3. `HRTIM` 快环中断
  - 负责电流环、电压环、模式对应 duty 计算和 PWM 比较值刷新

整体上是“中断驱动 + 全局控制变量共享”的经典数字电源固件结构。

### 3.2 主要用户控制文件

#### `Core/Src/main.c`

作用：

- 系统入口
- 启动所有底层外设
- 启动风扇 PWM
- 启动 `TIM6 / TIM7 / TIM16`
- 启动 ADC 与 HRTIM
- 在主循环里运行慢速后台任务

关键点：

- `TIM6` 负责毫秒计数与喂狗
- `TIM7` 负责控制主节拍
- `TIM16` 负责把 ADC 平均值换算成物理量
- 主循环负责：
  - `StatusLed_Update()`
  - `Update_Flash()`
  - `UF4Transport_RunTask()`

#### `Core/Src/function.c`

作用：

- 保存全局控制变量
- 实现 ADC 平均
- 实现主状态机
- 实现 Buck / Mix / Boost 模式判定
- 实现 OVP / OCP / OTP / Short 保护
- 实现参数默认值、Flash 初始化与读写
- 实现风扇 PWM 控制
- 实现模式切换占空注入准备

核心职责可以分成六类：

1. 数据与状态变量
  - `CtrValue`
  - `DF`
  - `SADC`
  - `SET_Value`
2. ADC 采样平均
  - `ADCSample()`
  - `ADC_calculate()`
3. 主状态机
  - `StateM()`
  - `StateMInit()`
  - `StateMWait()`
  - `StateMRise()`
  - `StateMRun()`
  - `StateMErr()`
4. 模式切换
  - `BBMode()`
  - `PowerControl_PrepareModeSwitch()`
5. 保护
  - `ShortOff()`
  - `OVP()`
  - `OCP()`
  - `OTP()`
6. 辅助
  - `PowerControl_DisableOutput()`
  - `FAN_PWM_set()`
  - `Init_Flash() / Read_Flash() / Update_Flash()`

当前重要变更也集中在这个文件：

- 已删除输入欠压保护
- 已删除固件自动风扇策略，改为通过通信接口控制
- 新增切模前 duty 预计算与注入准备

#### `Core/Src/pid.c`

作用：

- 实现双环控制
- 根据当前模式输出 Buck / Boost 支路占空比
- 直接刷新 HRTIM 比较寄存器

当前快环结构：

1. 电流环先生成电压参考修正量
2. 电压环再根据当前模式生成主 duty
3. 根据 `Buck / Mix / Boost` 决定：
  - 哪一侧主调制
  - 哪一侧固定占空或平滑逼近目标占空
4. 最后写入：
  - `Timer A Compare1`
  - `Timer A Compare3`
  - `Timer D Compare1`

当前代码已经加入三类关键增强：

- `Buck` 模式下，Boost 支路对齐为同步整流安全占空
- `Mix` 模式进入改成平滑爬升，避免一刀切到高占空
- `Boost` 模式进入改成平滑爬升，避免 Buck 支路瞬时拉到极高占空

另外还增加了 TI 思路的切模注入：

- 切换到 `Buck` 前先按 `D = Vout / Vin` 计算目标 duty
- 切换到 `Mix` 前先按 `D = Vout / (Vin + Vout)` 计算目标 duty
- 切换到 `Boost` 前先按 `D = (Vout - Vin) / Vout` 计算目标 duty
- 切模时把 duty 与环路种子直接装入快环，避免沿用旧模式积分硬跳

#### `Core/Src/uf4_transport.c`

作用：

- 把固件控制变量绑定到 `UF4COM V3` 协议 ID
- 处理 USB CDC / USART DMA 收发
- 处理 `READ / WRITE / REPORT / STREAM`
- 负责把外部写入同步到控制变量

主要内容：

1. 固定传输端口选择
2. UF4 ID 表绑定
3. 读寄存器镜像刷新
4. 写寄存器应用
5. CDC / UART DMA 输入处理
6. 流发送调度

这部分是固件对外提供 `UF4COM V3` 控制与状态接口的桥。

#### `Core/Src/status_led.c`

作用：

- 按当前状态机、故障和输出状态更新三色 LED
- 保持项目当前定义的状态语义

该文件不参与功率控制本身，但负责把运行状态转换成板上可视反馈。

#### `Core/Src/temp.c`

作用：

- NTC 温度换算
- MCU 温度换算

它为 `OTP()`、上报温度和风扇控制数据源提供支持。

#### `Core/Src/W25Q64.c`

作用：

- SPI Flash 底层驱动
- 提供读、写、擦除接口

上层由 `function.c` 中的 `Init_Flash() / Read_Flash() / Update_Flash()` 调用。

### 3.3 主要 CubeMX/HAL 生成文件

这些文件大多由 CubeMX 或 HAL 模板生成，但在项目运行中实际承担关键底层角色，因此也应当纳入说明。

#### `Core/Src/adc.c`

- 配置 `ADC1 / ADC2 / ADC3 / ADC5`
- 定义采样通道、触发源、过采样、DMA 方式
- `ADC1` 是主功率量采样入口

#### `Core/Src/hrtim.c`

- 配置 `HRTIM1`
- 定义 `Timer A` 和 `Timer D`
- 配置死区
- 配置 ADC 触发点
- 配置互补输出引脚

这是功率控制的底层核心之一。

#### `Core/Src/tim.c`

- 配置 `TIM6`
- 配置 `TIM7`
- 配置 `TIM8`
- 配置 `TIM16`

其中：

- `TIM6`：1 ms
- `TIM7`：5 ms
- `TIM16`：10 ms
- `TIM8_CH3`：风扇 PWM

#### `Core/Src/gpio.c`

- 配置 LED
- 配置 PWM 引脚复用
- 配置 `DIV_SW` 等辅助引脚

#### `Core/Src/usart.c`

- 配置 `USART1 / USART2`
- 配合 `UF4Transport` 处理串口通信

#### `Core/Src/spi.c`

- 配置 `SPI1`
- 供 `W25Q64.c` 使用

#### `Core/Src/dma.c`

- 配置 DMA 控制器
- 为 `ADC1 DMA`、UART DMA 等提供底层支持

#### `Core/Src/iwdg.c`

- 配置独立看门狗
- 由固定节拍任务喂狗

#### `Core/Src/stm32g4xx_it.c`

- 中断入口
- 把 HRTIM、TIM、DMA、USB 等中断交给 HAL 和用户逻辑

#### `Core/Src/stm32g4xx_hal_msp.c`

- MSP 初始化
- GPIO AF、DMA 链接、时钟使能等底层板级装配

#### `Core/Src/system_stm32g4xx.c`

- 系统时钟基础支持
- 核心启动时钟框架

### 3.4 配套中间件与接口层 C 文件

除了 `Core/Src`，还有几类 C 文件同样是固件运行必需部分：

#### `USB_Device/App/usbd_cdc_if.c`

- USB CDC 收发桥接
- 把收到的 USB 数据转发到 `UF4Transport_OnUsbCdcRx()`

#### `Middlewares` / `Drivers`

- `STM32 HAL`
- `CMSIS`
- `USB Device`

这些代码不是本项目手写控制逻辑，但属于最终可运行固件的一部分。

### 3.5 当前固件实际调用链

可以把当前控制链概括为：

1. `main.c` 初始化外设
2. `TIM7` 周期调用：
  - `ADCSample()`
  - 保护逻辑
  - `StateM()`
  - `BBMode()`
3. `HRTIM` 快环中断调用：
  - `BuckBoostVILoopCtlPID()`
4. `UF4Transport`
  - 处理外部控制命令
  - 回传状态与流数据
5. 主循环
  - 更新状态灯
  - 保存参数
  - 处理通信后台任务

---

## 4. 借鉴代码与参考资料

本项目不是从零开始完全自写。下面按“来源 -> 用到什么”列出。

### 4.1 直接借鉴的代码工程

#### 参考代码 [Buck-Boost-Digital-Power](https://github.com/zeruns/Synchronous-Rectification-Buck-Boost-Digital-Power-Supply-Based-on-STM32)

这是当前 `UF4DigitalPower` 最直接的代码来源，借鉴内容包括：

- 控制变量组织方式
- 主状态机结构
- `Buck / Mix / Boost` 模式判定框架
- `pid.c` 的双环控制结构
- 软启动框架
- Flash 参数存储方式
- 温度换算与风扇 PWM 基础接口

当前工程并不是简单复制，而是做了以下方向上的重构：

- 从 `D/F` HRTIM 映射迁移到当前 `A/D` 映射
- 删除按键和 OLED 本地人机交互
- 改为基于 `UF4COM V3` 的外部控制接口
- 去掉 `FMAC / CRC` 硬件依赖，改为纯软件路径
- 修复多处模式切换、占空钳位、保护与上报问题

### 4.2 协议核心来源

#### [UF4COM V3](https://github.com/UF4OVER/UF4COM)

借鉴内容：

- `UF4COM V3` 帧结构
- CRC16-CCITT
- TV 数据项编码
- `READ / WRITE / REPORT / STREAM` 交互范式

当前固件的 `uf4_transport.c` 基于这一协议核心实现控制、状态与流数据传输。

### 4.3 ST 官方工具链与自动生成代码

#### STM32CubeMX / STM32CubeG4 / HAL / CMSIS

借鉴内容：

- 外设初始化代码骨架
- HRTIM、ADC、TIM、USART、SPI、USB CDC 配置模板
- 中断框架
- HAL 驱动与启动文件

这些内容主要体现在：

- `adc.c`
- `hrtim.c`
- `tim.c`
- `gpio.c`
- `usart.c`
- `spi.c`
- `dma.c`
- `stm32g4xx_it.c`
- `stm32g4xx_hal_msp.c`

### 4.4 控制策略参考资料

#### TI官方文档 [Multimode control for a four-switch buck-boost converter](https://www.ti.com/lit/an/slyt765/slyt765.pdf?ts=1781482881151)

标题：

- `Multimode control for a four-switch buck-boost converter`

借鉴内容：

- `Buck / Buck-Boost / Boost` 多模态 duty 公式
- 切模前先按目标模式计算 duty，再把目标 duty 注入控制环的思路
- 认识到模式切换时不应依赖旧模式积分慢慢追

当前项目里新增的“切模前预计算 duty 并注入”机制直接参考了这条思路。

#### ST官方文档 [Buck-boost converter using the STM32F334 Discovery kit](https://www.st.com/resource/en/application_note/an4449-buckboost-converter-using-the-stm32f334-discovery-kit-stmicroelectronics.pdf)

标题：

- `Buck-boost converter using the STM32F334 Discovery kit`

借鉴内容：

- HRTIM 在 Buck-Boost 电源中的使用方法
- ADC 触发和 PWM 协调方式
- 高分辨率定时器在数字电源中的组织思路

### 4.5 器件资料参考

#### `UCC27211` 数据手册

借鉴内容：

- 半桥高边 bootstrap 驱动机制
- `HB / HS / HO / LO` 含义
- 高边长期近似常开时 bootstrap 刷新约束

这对当前 `Mix / Boost` 模式稳定性分析非常重要。

---

## 5. 项目结构

```text
UF4DigitalPower/
├─ Core/
│  ├─ Inc/
│  └─ Src/
├─ Drivers/
├─ Middlewares/
├─ USB_Device/
├─ docs/
├─ scripts/
├─ storage/
└─ UF4DigitalPower.ioc
```

---

## 6. 快速开始

```powershell
git clone https://github.com/UF4OVER/UF4DigitalPower_Firmware
cd UF4DigitalPower_Firmware
cmake --build cmake-build-debug -- -j4
```
