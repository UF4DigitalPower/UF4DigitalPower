# FMAC 100 kHz 电流内环设计

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

## 8. 10 kHz 电压 PI 外环

外环实现位于 `User/1_Controller/Loop/voltage_loop.c`。每 100 us 读取经板级分压标定的输出电压，以毫伏为单位调用 `VoltageLoop_Update()`；返回值是 0...10 A 的本地输出电流参考，传给 100 kHz `CurrentLoop_SetReference_mA()` / `current_loop_sample_t.iref_mA`。误差采用 `Vref - Vout`，因此输出电压偏低时电流参考增加。功率方向由运行模式选择，外环输出始终是当前方向上的非负输出电流幅值。

默认参数按 470 uF 输出电容计算：

```text
F_voltage = 10 kHz
Kp        = 96900 / 2^16 mA/mV
Ki*Ts     = 100000 / 2^30 mA/mV/sample
Iout_ref  = 0...10 A
```

该默认 Kp 对应约 0.50 Hz 的理想电容电压环交越频率，PI 零点约 0.10 Hz。它是基于当前平均模型和内环最慢约 7.48 Hz 交越值的保守初值；负载模型、有效输出电容量、真实内环调度与板级电压采样标定仍需上板验证。电压 PI 支持条件积分抗饱和，并允许用 `VoltageLoop_SetGains_Q16_Q30()` 和 `VoltageLoop_SetCurrentLimit_mA()` 调整参数。更新函数和 PI 算术位于 `.RamFunc`，辅助限幅函数内联。

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
