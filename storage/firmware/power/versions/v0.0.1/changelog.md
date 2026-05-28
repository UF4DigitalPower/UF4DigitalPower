# Power v0.0.1 Changelog

## 2026-05-28

- 本地发布 v0.0.1
- 发布说明：完善骨架代码，完善触发逻辑，绑定虚拟串口
- 适用设备：F4CP-POWER
- 更新内容：添加板级功率测量、最小功率控制和 TVLCOM 协议集成
              - 增加bsp_power.c/h用于ADC结果处理和物理测量转换
              - 添加power_ctrl.C/H，实现最小功率控制状态和设置接口
              - 添加user_tvlcom_protocol.h和user_tvlcom_transport.c/h用于F4CP TVLCOM协议和传输扇入
              - 将TVLCOM的传输和电源控制集成到主固件和USB CDC接收路径中
              - 更新注入通道和协议支持的ADC、DMA、HRTIM和中断配置
              - 更新CMakeLists.txt以包含用户源和头部
              - 添加硬件、测量和控制架构文档 readme.md
              - Add test_user_tvlcom_protocol.C 用于主机端协议测试
- 发布日期：2026-05-28
