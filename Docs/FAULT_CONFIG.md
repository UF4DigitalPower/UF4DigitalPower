## FAULT配置


| COMPX | COMP Polarity | COMP Event       | EEVX  | FAULT X |
|-------|---------------|------------------|-------|---------|
| COMP1 | inverted      | REVERSE_IIN OCP  | EEV4  | FAULT4  |
| COMP6 | inverted      | REVERSE_IOUT OCP | EEV3  | FAULT3  |
| COMP3 | NOT inverted  | FORWARD_IIN OCP  | EEV5  | FAULT5  |
| COMP7 | NOT inverted  | FORWARD_IOUT OCP | EEV10 | ?       |
| COMP2 | NOT inverted  | VIN OVP          | EEV1  | FAULT1  |
| COMP4 | NOT inverted  | VOUT OVP         | EEV2  | FAULT2  |

- 物理接口**PA0**和**PA1**的物理引脚全部连接到**INA240A1的输出端**，其实采样的都是一个电流信号
**PB11** 和 **PB14** 同理
- EEV10的COMP7使用软件或者HRTIM 置位触发锁存


