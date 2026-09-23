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


