### 硬件特性

- 主控平台：STM32G474
- 输出范围：0.5V - 50V
- 输出电流范围：0A - 10A
- 控制方式：电压环 + 电流环双闭环 PID
- 功率驱动：支持互补 PWM 输出与死区控制
- 数据存储：外接 SPI Flash，用于保存 PID 参数和系统状态
- 通信接口：XX
- 保护能力：支持过压、过流、过温等参数管理与保护阈值设定

### 采样与测量说明

- 电压采样采用分压方式进入 ADC，并由软件还原真实电压值。
- 电流采样基于 8mΩ 分流电阻、20 倍运放增益和 1.65V 中点偏置实现双向换算。
- 固件侧支持对输入输出电压、电流、温度等数据进行实时采集和上报。

### 软件分层建议

- `BSP`
  - 负责 ADC 原始结果、PWM 占空边界、GPIO、风扇、温度、板级缩放参数。
  - 对外提供 `VIN/VOUT/IIN/IOUT/TEMP` 的物理量换算接口。
- `CTRL`
  - 负责状态机、运行模式切换、故障管理、软启动、闭环控制。
  - 不直接操作寄存器，只调用 `BSP` 和 `HAL` 封装接口。
- `APP`
  - 负责上位机命令、参数下发、遥测上报、参数存储与恢复。

推荐的数据流是：

1. `ADC/HRTIM` 完成一次采样触发。
2. `BSP` 更新 `ADC_RESULT` 和测量值结构。
3. `CTRL` 根据当前状态和模式执行一次快环。
4. 电压外环按较低频率执行，更新电流给定。
5. `APP` 异步处理通信和参数管理。

### 运行状态机

建议主状态机至少包含以下状态：

- `POWER_STATE_IDLE`
  - PWM 关闭，输出禁止。
  - 等待使能命令和输入条件满足。
- `POWER_STATE_PRECHARGE`
  - 可选状态。
  - 用于输入有效性检查、采样稳定等待、偏置建立、继电器或前级准备。
- `POWER_STATE_SOFTSTART`
  - 从零占空比或安全占空比开始。
  - 电压给定或电流给定缓慢爬升。
- `POWER_STATE_RUN`
  - 正常闭环运行。
  - 根据输入输出关系在 `BUCK / MIX / BOOST` 之间切换。
- `POWER_STATE_FAULT`
  - 关闭 PWM。
  - 锁存故障码，等待手动清除或满足自动恢复条件。

状态转移建议：

- `IDLE -> PRECHARGE`
  - 收到使能命令。
  - 输入电压正常。
  - 无锁存故障。
- `PRECHARGE -> SOFTSTART`
  - ADC 偏置稳定。
  - 温度正常。
  - 关键传感器在线。
- `SOFTSTART -> RUN`
  - 输出进入目标附近。
  - 没有过流、过压、欠压等异常。
- `任意状态 -> FAULT`
  - 硬件过流、软件过流、输出过压、输入欠压、过温、驱动异常。
- `FAULT -> IDLE`
  - 故障被确认清除。
  - 等待重新使能。

### 故障管理建议

故障建议分为两层：

- `硬件快速故障`
  - 比如周期级过流。
  - 直接通过 `COMP/BKIN/HRTIM FAULT` 关断 PWM。
  - 软件只负责记录故障来源和复位流程。
- `软件慢速故障`
  - 输入欠压
  - 输出过压
  - 持续过流
  - 过温
  - 采样异常
  - 模式切换超时

建议故障码单独做位标志：

```c
typedef enum
{
    POWER_FAULT_NONE       = 0,
    POWER_FAULT_OCP_HW     = 1u << 0,
    POWER_FAULT_OCP_SW     = 1u << 1,
    POWER_FAULT_OVP_OUT    = 1u << 2,
    POWER_FAULT_UVP_IN     = 1u << 3,
    POWER_FAULT_OTP        = 1u << 4,
    POWER_FAULT_SENSOR     = 1u << 5,
    POWER_FAULT_MODE       = 1u << 6,
} power_fault_t;
```

故障处理原则：

- 一旦进入 `FAULT`，立刻关闭 PWM 输出。
- 锁存故障字，不要只保留最后一个故障。
- 区分 `可自动恢复` 和 `需人工清除` 的故障。
- 过流和输出过压建议默认人工清除。

### 运行模式划分

四开关 Buck-Boost 建议分为三种运行模式：

- `BUCK`
  - `VIN` 明显高于 `VOUT`
  - 由输入侧半桥主调制
  - 输出侧半桥作为同步整流或固定导通策略
- `BOOST`
  - `VIN` 明显低于 `VOUT`
  - 由输出侧半桥主调制
  - 输入侧半桥作为同步整流或固定导通策略
- `MIX`
  - `VIN` 接近 `VOUT`
  - 两侧共同参与调制
  - 用于平滑跨越 Buck 和 Boost 边界

结合当前板子的采样定义：

- `BUCK` 模式内环电流使用 `IOUT` 近似电感电流。
- `BOOST` 模式内环电流使用 `IIN` 近似电感电流。
- `MIX` 模式可优先沿用 `IIN`，或根据实验结果做加权切换。

### 模式切换条件建议

不要用单点比较直接切模式，必须加迟滞。

定义：

```c
vin_to_vout_ratio = VIN / VOUT;
```

建议切换门限：

- 进入 `BUCK`
  - `VIN > VOUT * 1.08`
- 进入 `BOOST`
  - `VIN < VOUT * 0.92`
- 进入 `MIX`
  - `0.92 <= VIN / VOUT <= 1.08`

也可以用绝对压差：

- `VIN - VOUT > +2V` 进入 `BUCK`
- `VIN - VOUT < -2V` 进入 `BOOST`
- `|VIN - VOUT| <= 2V` 进入 `MIX`

切换时建议增加以下保护：

- 连续满足门限 `N` 次再切换。
- 切换瞬间冻结积分项或限幅积分项。
- 切换后进入一个短暂的过渡窗口，限制占空比变化率。
- 如果切换后短时间内电流异常，立即退回 `FAULT` 或回退上一个模式。

### 控制环执行建议

建议采用快环和慢环分离：

- `电流内环`
  - 执行频率：每个 PWM 周期，或每 `1~2` 个 PWM 周期。
  - 输入：当前模式对应的近似电感电流。
  - 输出：主调制桥臂占空比。
- `电压外环`
  - 执行频率：`1kHz ~ 10kHz`
  - 输入：`VOUT`
  - 输出：电流给定 `Iref`

推荐控制链：

```text
Vref - Vout -> Voltage PID -> Iref
Iref - Imeas -> Current PID -> Duty
```

其中：

- `BUCK` 时 `Imeas = IOUT`
- `BOOST` 时 `Imeas = IIN`
- `MIX` 时 `Imeas` 可先使用 `IIN`

### 软启动建议

软启动不要直接拉升占空比，建议拉升参考值：

- 电压模式启动：
  - `Vref_cmd` 从 `0` 线性上升到目标值
- 电流模式启动：
  - `Iref_cmd` 从 `0` 线性上升到限制值

软启动期间建议：

- 限制最大占空比
- 限制电流参考上升斜率
- 若检测到异常，立即退出到 `FAULT`

### 推荐代码骨架

```c
typedef struct
{
    uint32_t fault_flags;
    uint8_t enabled;
    power_state_t state;
    power_mode_t mode;
    float vin_v;
    float vout_v;
    float iin_a;
    float iout_a;
    float iref_a;
    float vref_v;
    float duty_main;
} power_ctrl_t;
```

```c
void PowerCtrl_FastLoop(power_ctrl_t *ctrl)
{
    PowerCtrl_UpdateMeasurements(ctrl);
    PowerCtrl_CheckFaults(ctrl);

    if (ctrl->fault_flags != 0u)
    {
        PowerCtrl_EnterFault(ctrl);
        return;
    }

    switch (ctrl->state)
    {
        case POWER_STATE_IDLE:
            PowerCtrl_HandleIdle(ctrl);
            break;

        case POWER_STATE_PRECHARGE:
            PowerCtrl_HandlePrecharge(ctrl);
            break;

        case POWER_STATE_SOFTSTART:
            PowerCtrl_HandleSoftstart(ctrl);
            break;

        case POWER_STATE_RUN:
            PowerCtrl_UpdateMode(ctrl);
            PowerCtrl_RunCurrentLoop(ctrl);
            break;

        case POWER_STATE_FAULT:
        default:
            PowerCtrl_ShutdownPwm(ctrl);
            break;
    }
}
```

```c
void PowerCtrl_SlowLoop(power_ctrl_t *ctrl)
{
    if (ctrl->state != POWER_STATE_RUN)
    {
        return;
    }

    ctrl->iref_a = VoltagePid_Run(ctrl->vref_v, ctrl->vout_v);
}
```

### 当前项目下的落地建议

- `BSP`
  - 统一维护 `ADC_RESULT_t` 和物理量换算。
  - 提供 `Buck` 和 `Boost` 下内环电流选择接口。
- `CTRL`
  - 先完成状态机和模式切换，不急着一开始就把 PID 调到最优。
  - 先让 `IDLE -> SOFTSTART -> RUN -> FAULT` 跑通。
- `模式切换`
  - 先用固定阈值 + 迟滞。
  - 等基本运行稳定后，再加更细的前馈和积分处理。
