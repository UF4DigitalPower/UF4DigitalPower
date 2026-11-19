# STM32G474RBT6 启动与 LED 排障

G474 固件修订：0.1.2。CH32/F4CP 版本不受本次改动影响。

## 板级依据

依据 `../../PCB/SCH_UF4DP3_POWER_2026-11-08.pdf` 第 5 页 `05_G474`：

- LED1 为 PC0，LED2 为 PC1，均有通往 GND 和 P3V3 的 LED 支路。
  不假定颜色，高低电平均可能有对应指示；交替电平用于观察状态变化。
- X2 为 8 MHz。保留 HSE / 2 × 85 / 2 = 170 MHz 配置，
  将 CMSIS 的 HSE_VALUE 与 HAL 的 8 MHz 定义统一。

旧代码及 IOC 错把 LED 配为 PC2/PC3，且主循环为空。
这一差异能解释 LED 不变化，不能据此认定 Flash 写入失败或成功。

## 指示行为

在 HSE/PLL 配置之前，只初始化 PC0/PC1，初始 LED1 高、LED2 低。
其余 GPIO 仍在原位置初始化。

- 正常进入主循环：LED1/LED2 每 500 ms 交换高低电平。
- HAL 初始化错误进入 Error_Handler：两个引脚同步切换高低电平，
  使用不依赖 SysTick/中断的近似延时。

HardFault、复位保持、错误启动地址等情况不保证出现上述报码。
CubeMX 的 IOC 同步使用 PC0/PC1，避免重新生成时恢复错误引脚。

## 构建及下载

```powershell
cmake --build cmake-build-debug-stm32 --parallel 6
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 6
```

有线 F4CP / pyOCD 选择 `build/release/STM32G474RBT6.hex`；
HEX 自带从 `0x08000000` 开始的 Flash 地址，不能选择 RAM 链接产物。
也可使用同目录 BIN（烧录基址为 `0x08000000`）。
下载后需同时检查 F4CP 的完整读回校验结果和实板 LED 变化。

本次交付只做编译及离线镜像检查，不自动连接、复位或刷写硬件。
没有实板烧录成功、晶振起振或 LED 运行通过的结论。

2026-11-18 验证记录：Debug / Release 编译通过；Flash 分别为 4920 / 4704 字节。
两种构建的 HEX / BIN 字节一致，Flash 起址及复位向量检查通过，
F4CP 的有线 G474 文件加载校验通过。Release CRC32 为 `3578F850`。
未运行硬件模拟测试；这些离线检查不代表实板已成功下载或运行。
