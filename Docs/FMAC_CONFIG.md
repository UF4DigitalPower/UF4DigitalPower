# FMAC 2P2Z 电流内环设计

本文说明 STM32G474 如何用 FMAC 直接执行 2P2Z Type-II 电流补偿器。FMAC 的 IIR 单元虽然在 RM0440 中归在“滤波器”章节，但它实际执行的是递推差分方程。输入可以是电流误差，输出可以直接是 PWM 调制量，不需要再在 CPU 中实现一套 PI 或滤波器。

设计依据：

- `Docs/datesheet/AN5788.pdf`
- `Docs/datesheet/RM0440.pdf` 第 18 章 FMAC
- `Docs/BSP_CONFIG.md`
- `Docs/fmac_2p2z_current_loop.m`

## 1. 工程时序

板级设计中的开关频率为 200 kHz，电流内环目标执行频率为 50 kHz。因此电流控制每 4 个 PWM 周期执行一次：

```text
F_pwm   = 200 kHz
F_loop  = 50 kHz
T_loop  = 20 us
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
最大输入/输出电压 72 V
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

对于 50 kHz 电流环，第一版建议把 `Fc` 控制在 2.5 kHz 以下，再根据实测响应调整。验收时至少检查：

```text
相位裕量 PM > 45 deg
增益裕量 GM > 10 dB
```

实际设计不能只看 PM。若 PM 很大但 GM 很小，说明高频仍有额外增益交越，器件偏差和延迟可能使系统变得不稳定。

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

## 7. 50 kHz 执行流程

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

- 配置 50 kHz 电流环和 200 kHz PWM；
- 输入电感、等效电阻、母线电压和采样缩放；
- 设计 Type-II 2P2Z；
- Tustin 离散化；
- 生成 FMAC 的 B/A 系数、Q15 数值和 `R`；
- 生成 RM0440 的 X1/X2/Y 内存布局；
- 导出 `fmac_2p2z_coefficients.h`。

脚本中的电感和电阻来自 `BSP_CONFIG.md`，但四开关 Buck-Boost 的四个工作方向仍应分别用实测 plant 校核。生成的系数不能绕过硬件调试直接固化到量产参数中。

## 9. 调试检查

按以下顺序检查，能减少定位时间：

1. 用示波器确认 PWM 实际频率为 200 kHz，控制执行频率为 50 kHz。
2. 确认 ADC 触发发生在预期的 PWM 位置。
3. 静态测量 `VREF165`，确认正向和反向零电流时的 ADC 偏差已扣除。
4. 在 FMAC 停止状态下读回 X2，确认系数顺序为 `[B0 B1 B2 A1 A2]`。
5. 用已知误差序列验证 FMAC 输出与 MATLAB 差分方程逐点一致。
6. 先限制 PWM 输出，再逐步提高电流给定。
7. 检查 FMAC 的 `SAT`、`OVFL`、`UNFL` 标志。
8. 分别验证 Buck、Boost 和正反向切换时的历史状态处理。

当前工程的 `Core/Src/fmac.c` 只有 FMAC 时钟和基础初始化，尚未完成上述 2P2Z 配置和控制调度。

