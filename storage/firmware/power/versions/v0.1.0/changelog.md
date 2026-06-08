# Power v0.1.0 Changelog

## 2026-06-07

- 正式发布 v0.1.0
- 适用设备：F4CP-POWER / STM32G474CBT6
- 启用 PB5 栅极驱动器使能输出，上电后默认拉高 GATE_EN
- ADC1 规则采样切换为 VIN/IIN/VOUT/IOUT 四通道，使用 HRTIM 触发和 4x 过采样
- 控制环使用快速 ADC 采样值参与 PID，降低输出在 VIN 和 0 之间跳变的风险
- 保持 TVLCOM/USB CDC 通信和上下位机电源页协议对接
- 发布日期：2026-06-07
