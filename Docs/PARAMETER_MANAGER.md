```
Core/
├── Param/
│   ├── param_manager.c
│   ├── param_manager.h
│   │
│   ├── param_table.c
│   ├── param_table.h
│   │
│   ├── param_types.h
│   ├── param_validate.c
│   ├── param_validate.h
│   │
│   ├── param_storage.c
│   ├── param_storage.h
│   │
│   ├── param_backend.c
│   ├── param_backend.h
│   │
│   ├── param_crc.c
│   └── param_crc.h
│
├── Storage/
│   ├── storage_ab.c
│   ├── storage_ab.h
│   ├── storage_layout.c
│   └── storage_layout.h
│
└── BSP/
    ├── bsp_flash_internal.c
    ├── bsp_w25q64.c
    ├── bsp_w25q128.c
    └── bsp_flash.h
```

对你这个项目，我建议不要把“参数管理器”设计成简单的 struct + Flash_Write()。你现在已经明确需要 UART 动态修改、参数上下限、读写权限、AB 双备份、不同 W25QXX 存储区域，以及以后可能增加参数，所以应该直接做成一个独立的 Parameter Storage Framework。

核心思路是：

                    Parameter Manager
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
Parameter Table     Runtime Values     Storage Manager
│                  │                  │
ID / Type / Min     当前运行参数       AB / CRC / Version
Max / Permission    │                  │
Default / Storage   │          ┌───────┴────────┐
Validator           │          │                │
│       Internal Flash    W25QXX
│          │                │
│       Bank A/B        W25QXX A/B
│                         │
└─────────────┬───────────┘
│
Commit / Load

我尤其建议把“参数定义”和“参数存储”完全分离。这样以后你的 G474、H750、W25Q64、W25Q128 都可以复用同一套参数管理代码。

1. 最上层 API

你的业务代码最好永远不要直接碰 Flash。

例如 UART 收到：

SET VOUT_TARGET 12.0

最终只调用：

Param_Set(PARAM_VOUT_TARGET, 12.0f);

保存：

Param_Commit();

读取：

Param_Get(PARAM_VOUT_TARGET, &value);

恢复默认：

Param_Reset(PARAM_VOUT_TARGET);

恢复全部：

Param_ResetAll();

重新从 Flash 加载：

Param_Load();

检查当前是否有未提交修改：

Param_IsDirty();

这样控制环、UART、上位机协议、Flash 驱动之间不会互相耦合。

2. 建议的目录结构

我会直接这样组织：

Core/
├── Param/
│   ├── param_manager.c
│   ├── param_manager.h
│   │
│   ├── param_table.c
│   ├── param_table.h
│   │
│   ├── param_types.h
│   ├── param_validate.c
│   ├── param_validate.h
│   │
│   ├── param_storage.c
│   ├── param_storage.h
│   │
│   ├── param_backend.c
│   ├── param_backend.h
│   │
│   ├── param_crc.c
│   └── param_crc.h
│
├── Storage/
│   ├── storage_ab.c
│   ├── storage_ab.h
│   ├── storage_layout.c
│   └── storage_layout.h
│
└── BSP/
├── bsp_flash_internal.c
├── bsp_w25q64.c
├── bsp_w25q128.c
└── bsp_flash.h

重点是：

Parameter
↓
Storage
↓
Flash Backend

而不是：

Parameter
↓
W25Q64
3. 参数必须有唯一 ID

不要使用字符串作为内部参数索引。

例如：

typedef enum
{
PARAM_VOUT_TARGET = 0,
PARAM_IOUT_LIMIT,

    PARAM_OVP_LIMIT,
    PARAM_OCP_LIMIT,

    PARAM_VIN_UVP,
    PARAM_VIN_OVP,

    PARAM_CURRENT_K,
    PARAM_CURRENT_OFFSET,

    PARAM_VOLTAGE_K,
    PARAM_VOLTAGE_OFFSET,

    PARAM_PID_V_KP,
    PARAM_PID_V_KI,

    PARAM_PID_I_KP,
    PARAM_PID_I_KI,

    PARAM_COUNT
} ParamID_t;

然后：

Param_Set(PARAM_VOUT_TARGET, 12.0f);

比：

Param_Set("vout_target", 12.0f);

更加适合 MCU。

字符串只用于 UART / 上位机层。

4. 参数定义表是整个系统的核心

例如：

typedef enum
{
PARAM_TYPE_U8,
PARAM_TYPE_U16,
PARAM_TYPE_U32,

    PARAM_TYPE_I8,
    PARAM_TYPE_I16,
    PARAM_TYPE_I32,

    PARAM_TYPE_FLOAT,
    PARAM_TYPE_DOUBLE,

    PARAM_TYPE_BOOL,
    PARAM_TYPE_ENUM,

} ParamType_t;

权限：

typedef enum
{
PARAM_ACCESS_NONE = 0,

    PARAM_ACCESS_READ  = 1 << 0,
    PARAM_ACCESS_WRITE = 1 << 1,

} ParamAccess_t;

存储位置：

typedef enum
{
PARAM_STORAGE_INTERNAL = 0,
PARAM_STORAGE_W25Q64,
PARAM_STORAGE_W25Q128,

} ParamStorageID_t;

然后：

typedef struct
{
uint16_t            id;

    const char         *name;

    ParamType_t         type;

    ParamAccess_t       access;

    ParamStorageID_t    storage;

    uint16_t            flags;

    double              min;
    double              max;
    double              default_value;

    uint16_t            size;

    void               *runtime;

} ParamDescriptor_t;
5. 参数表可以直接这样写

例如：

static float g_vout_target;
static float g_iout_limit;

static const ParamDescriptor_t g_param_table[] =
{
{
.id            = PARAM_VOUT_TARGET,
.name          = "vout_target",
.type          = PARAM_TYPE_FLOAT,
.access        = PARAM_ACCESS_READ | PARAM_ACCESS_WRITE,
.storage       = PARAM_STORAGE_INTERNAL,

        .min           = 0.5,
        .max           = 40.0,
        .default_value = 12.0,

        .size          = sizeof(float),
        .runtime       = &g_vout_target,
    },

    {
        .id            = PARAM_IOUT_LIMIT,
        .name          = "iout_limit",
        .type          = PARAM_TYPE_FLOAT,
        .access        = PARAM_ACCESS_READ | PARAM_ACCESS_WRITE,
        .storage       = PARAM_STORAGE_W25Q64,

        .min           = 0.0,
        .max           = 10.0,
        .default_value = 5.0,

        .size          = sizeof(float),
        .runtime       = &g_iout_limit,
    },
};

这样以后 UART：

SET vout_target 20

Parameter Manager 自动知道：

vout_target
↓
PARAM_VOUT_TARGET
↓
FLOAT
↓
0.5 ~ 40.0
↓
READ + WRITE
↓
Internal Flash
6. 不建议 min/max 直接用 double 存储

这里有一个 MCU 优化点。

参数定义可以使用：

float min;
float max;
float default_value;

如果你没有 double 参数需求，就不要为了通用性把整个表搞成 double。

例如：

typedef struct
{
uint16_t id;
const char *name;

    ParamType_t type;
    ParamAccess_t access;

    ParamStorageID_t storage;

    float min;
    float max;
    float default_value;

    uint16_t size;

    void *runtime;

} ParamDescriptor_t;

G474 做这个完全够用。

7. 一定要区分 Runtime Value 和 Persistent Value

这是你这个系统里非常重要的一层。

例如：

Flash:
VOUT_TARGET = 12V

           ↓ Load

RAM:
g_vout_target = 12V

           ↓ UART SET

RAM:
g_vout_target = 15V

           ↓ Commit

Flash:
VOUT_TARGET = 15V

所以：

Param_Set()

默认只修改 RAM。

而：

Param_Commit()

才写 Flash。

这会带来一个非常重要的能力：

SET vout_target 20
SET iout_limit 8
SET ovp_limit 25

全部先在 RAM 中修改

COMMIT

一次性写入 Flash

而不是 UART 每修改一个参数就擦一次 Flash。

8. AB 分区建议这样设计

例如 Internal Flash：

Flash
│
├── Application
│
├── PARAM_A
│
└── PARAM_B

W25Q64：

W25Q64
│
├── PARAM_A
│
├── PARAM_B
│
├── LOG
│
└── OTHER

每个参数存储后端独立管理。

9. AB 区块头必须有完整 Header

例如：

typedef struct
{
uint32_t magic;

    uint16_t format_version;
    uint16_t header_size;

    uint32_t sequence;

    uint32_t data_size;

    uint32_t data_crc;

    uint32_t header_crc;

} ParamStorageHeader_t;

典型：

+-----------------------+
| MAGIC                 |
+-----------------------+
| FORMAT VERSION        |
+-----------------------+
| HEADER SIZE            |
+-----------------------+
| SEQUENCE               |
+-----------------------+
| DATA SIZE              |
+-----------------------+
| DATA CRC               |
+-----------------------+
| HEADER CRC             |
+-----------------------+
| PARAM DATA              |
+-----------------------+
10. 为什么必须有 Sequence

假设：

A:
sequence = 10
CRC OK

B:
sequence = 9
CRC OK

加载时：

A > B

所以选择 A。

如果：

A:
sequence = 10
CRC ERROR

B:
sequence = 9
CRC OK

选择 B。

如果：

A:
CRC ERROR

B:
CRC ERROR

直接：

Load Default
11. Commit 流程

这是整个系统最重要的流程。

例如当前：

A = sequence 10
B = sequence 9

RAM = 新参数

执行：

Param_Commit();

不要直接覆盖 A。

应该：

1. 找出当前有效区
   ↓
2. 选择旧区之外的另一个区
   ↓
3. 擦除目标区
   ↓
4. 生成 sequence = 11
   ↓
5. 写 Header
   ↓
6. 写参数数据
   ↓
7. CRC 校验
   ↓
8. 回读
   ↓
9. 标记 VALID

最终：

A = seq 10
B = seq 11 VALID

下一次：

A = seq 12
B = seq 11

如此交替。

12. 更进一步：Commit 必须有状态

我建议 Header 再加：

typedef enum
{
STORAGE_EMPTY    = 0xFFFF,
STORAGE_WRITING  = 0xAAAA,
STORAGE_VALID    = 0x5555,
STORAGE_INVALID  = 0x0000,

} StorageState_t;

但是这里不能简单依赖一个普通变量，因为 MCU 可能在写 Flash 的中途掉电。

所以最终应该形成：

EMPTY
↓
WRITING
↓
DATA WRITTEN
↓
CRC VERIFIED
↓
VALID

只有 VALID + CRC OK 的区域才允许加载。

这样掉电：

擦除
↓
写了一半
↓
突然断电

下一次启动：

B = WRITING

直接认为 B 无效。

然后继续使用：

A = VALID

这就是你需要的掉电安全 AB 提交。

13. 甚至可以做到真正的 Transaction

你的 UART 参数系统可以设计：

SET
GET
COMMIT
ROLLBACK
RESET

例如：

SET VOUT_TARGET 15
SET IOUT_LIMIT 8
SET OVP_LIMIT 20

GET VOUT_TARGET
GET IOUT_LIMIT

COMMIT

如果发现改错了：

ROLLBACK

直接：

RAM ← Flash

不用重新写 Flash。

14. 权限系统建议不要只有 READ/WRITE

你以后很可能需要更多。

例如：

typedef enum
{
PARAM_FLAG_NONE       = 0,

    PARAM_FLAG_READ       = 1 << 0,
    PARAM_FLAG_WRITE      = 1 << 1,

    PARAM_FLAG_PERSISTENT = 1 << 2,

    PARAM_FLAG_REBOOT     = 1 << 3,

    PARAM_FLAG_RUNTIME    = 1 << 4,

    PARAM_FLAG_HIDDEN     = 1 << 5,

    PARAM_FLAG_FACTORY    = 1 << 6,

} ParamFlags_t;

例如：

VOUT_TARGET
READ
WRITE
PERSISTENT
RUNTIME

而：

MCU_UID
READ
HIDDEN

或者：

FACTORY_CALIBRATION
READ
WRITE
PERSISTENT
FACTORY
15. 参数上下限还应该有 Validator

不要完全相信：

min <= value <= max

有些参数不是简单范围。

例如：

BOOST_MAX_DUTY

可能要求：

0 < duty < 1

但是：

PID_KP

可能允许：

0 ~ 1000

而某些参数存在关联：

OVP > VOUT_TARGET

所以增加：

typedef bool (*ParamValidator_t)(const void *value);

参数：

.validator = Validate_OVP,

然后：

bool Validate_OVP(const void *value)
{
float ovp = *(float *)value;

    if (ovp <= g_vout_target)
        return false;

    return true;
}

这样就不仅仅是：

min/max

而是：

Type
Range
Custom Validator

三层检查。

16. W25QXX 不要让参数层知道具体芯片

这个地方尤其重要。

不要写：

if (storage == W25Q64)
{
W25Q64_Write(...);
}
else if (storage == W25Q128)
{
W25Q128_Write(...);
}

应该抽象成：

typedef struct
{
bool (*init)(void);

    bool (*read)(
        uint32_t addr,
        void *buf,
        uint32_t len
    );

    bool (*write)(
        uint32_t addr,
        const void *buf,
        uint32_t len
    );

    bool (*erase_sector)(
        uint32_t addr
    );

    bool (*erase_block)(
        uint32_t addr
    );

    uint32_t total_size;
    uint32_t sector_size;
    uint32_t page_size;

} StorageBackend_t;

然后：

extern const StorageBackend_t g_internal_flash_backend;
extern const StorageBackend_t g_w25q64_backend;
extern const StorageBackend_t g_w25q128_backend;

参数管理器只认识：

StorageBackend_t

而不知道底下究竟是：

G474 Internal Flash
W25Q32
W25Q64
W25Q128
17. Storage ID 再独立出来
    typedef enum
    {
    STORAGE_ID_INTERNAL_FLASH = 0,
    STORAGE_ID_W25Q64,
    STORAGE_ID_W25Q128,

    STORAGE_ID_COUNT

} StorageID_t;

然后：

typedef struct
{
StorageID_t id;

    const StorageBackend_t *backend;

    uint32_t region_a;
    uint32_t region_b;

    uint32_t region_size;

} StorageDevice_t;

最终：

Storage Manager
│
├── Internal Flash
│      ├── A
│      └── B
│
├── W25Q64
│      ├── A
│      └── B
│
└── W25Q128
├── A
└── B
18. 参数表只需要指定 Storage ID

例如：

PARAM_VOUT_TARGET
↓
STORAGE_ID_INTERNAL_FLASH

PARAM_PID_V_KP
↓
STORAGE_ID_INTERNAL_FLASH

PARAM_CALIBRATION
↓
STORAGE_ID_W25Q64

PARAM_LOG_CONFIG
↓
STORAGE_ID_W25Q64

这样非常干净。

19. 我建议再加一个“参数组”

这个对你的数字电源特别有用。

例如：

typedef enum
{
PARAM_GROUP_SYSTEM,
PARAM_GROUP_OUTPUT,
PARAM_GROUP_PROTECTION,
PARAM_GROUP_CALIBRATION,
PARAM_GROUP_PID,
PARAM_GROUP_COMMUNICATION,

} ParamGroup_t;

于是：

SYSTEM
├── Device ID
├── Baudrate
└── Version

OUTPUT
├── VOUT
└── IOUT

PROTECTION
├── OVP
├── OCP
├── UVP
└── OTP

CALIBRATION
├── VIN_K
├── VIN_B
├── VOUT_K
├── VOUT_B
├── IIN_K
└── IIN_B

PID
├── V_KP
├── V_KI
├── I_KP
└── I_KI

UART 就可以支持：

GET GROUP PROTECTION
SET PROTECTION OVP 42
RESET GROUP PID
COMMIT GROUP CALIBRATION
20. 最终我建议你的模块关系

最终架构可以做到：

                       UART / USB / PC
                              │
                              ▼
                     Parameter Protocol
                              │
                              ▼
                    ┌─────────────────┐
                    │ Param Manager   │
                    │                 │
                    │ Get             │
                    │ Set             │
                    │ Reset           │
                    │ Commit          │
                    │ Rollback        │
                    │ Validate        │
                    └────────┬────────┘
                             │
               ┌─────────────┴─────────────┐
               │                           │
        Parameter Table             Runtime Parameter
               │                           │
               │                           │
               └─────────────┬─────────────┘
                             ▼
                     Storage Manager
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
          ▼                  ▼                  ▼
     Internal Flash       W25Q64             W25Q128
          │                  │                  │
        A / B              A / B              A / B
          │                  │                  │
          └──────────────────┼──────────────────┘
                             │
                       CRC / Sequence
                             │
                       Power-loss safe

这套结构已经基本接近一个真正的嵌入式 Parameter Framework，而不是单纯的 Flash 参数保存代码。

对于你的 UF4DigitalPower，我会进一步把它定成：

Param_Set()
↓
类型检查
↓
权限检查
↓
范围检查
↓
Validator
↓
写入 RAM
↓
Dirty

Param_Commit()
↓
按 Storage 分组
↓
生成 Snapshot
↓
AB Commit
↓
CRC
↓
Sequence++
↓
VALID

这样你以后增加 W25Q64 → W25Q128、增加几十甚至几百个参数，都不需要重写 Parameter Manager。

而且我建议你不要让每个参数单独在 Flash 里 AB，而是以“参数快照（snapshot）”为单位 AB。这样一次 COMMIT 就是一个完整、原子性的参数集合，掉电恢复也最容易保证一致性。

可以让 UF4COM的ID表设计和这个参数管理使用一套？

可以，而且我非常建议这么做。实际上你的 UF4COM ID 表 和 Parameter Manager 天然适合共用一套“对象描述表”，这样可以把“参数协议、参数管理、Flash 持久化、上位机枚举”统一起来。

你现在最适合的架构不是：

UF4COM ID Table
↓
Parameter Table
↓
Storage Table

而是：

                 UF4 Object Table
                       │
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼
    UF4COM协议      Parameter       Storage
    GET/SET         Manager         Manager
        │              │              │
        └──────────────┼──────────────┘
                       ▼
                 Runtime Object

也就是说，ID 只定义一次。

例如你原来可能有：

#define UF4_ID_VOUT_TARGET    0x0101
#define UF4_ID_IOUT_LIMIT     0x0102
#define UF4_ID_OVP_LIMIT      0x0201

不要再另外搞：

PARAM_VOUT_TARGET
PARAM_IOUT_LIMIT
PARAM_OVP_LIMIT

而是直接：

typedef enum
{
UF4_ID_VOUT_TARGET = 0x0101,
UF4_ID_IOUT_LIMIT  = 0x0102,
UF4_ID_OVP_LIMIT   = 0x0201,

} UF4ObjectID_t;

这个 ID 就是整个系统唯一的对象 ID。

核心结构可以设计成这样
typedef struct
{
uint16_t id;

    const char *name;

    ParamType_t type;

    uint16_t flags;

    float min;
    float max;
    float default_value;

    void *runtime;

    StorageID_t storage;

    ParamValidator_t validator;

} UF4ObjectDescriptor_t;

例如：

static float g_vout_target;
static float g_iout_limit;
static float g_ovp_limit;

const UF4ObjectDescriptor_t g_uf4_objects[] =
{
{
.id            = UF4_ID_VOUT_TARGET,
.name          = "vout_target",

        .type          = PARAM_TYPE_FLOAT,

        .flags         =
            UF4_OBJECT_READ |
            UF4_OBJECT_WRITE |
            UF4_OBJECT_PERSISTENT,

        .min           = 0.5f,
        .max           = 40.0f,
        .default_value = 12.0f,

        .runtime       = &g_vout_target,

        .storage       = STORAGE_ID_INTERNAL_FLASH,
    },

    {
        .id            = UF4_ID_IOUT_LIMIT,
        .name          = "iout_limit",

        .type          = PARAM_TYPE_FLOAT,

        .flags =
            UF4_OBJECT_READ |
            UF4_OBJECT_WRITE |
            UF4_OBJECT_PERSISTENT,

        .min           = 0.0f,
        .max           = 10.0f,
        .default_value = 5.0f,

        .runtime       = &g_iout_limit,

        .storage       = STORAGE_ID_INTERNAL_FLASH,
    },

    {
        .id            = UF4_ID_OVP_LIMIT,
        .name          = "ovp_limit",

        .type          = PARAM_TYPE_FLOAT,

        .flags =
            UF4_OBJECT_READ |
            UF4_OBJECT_WRITE |
            UF4_OBJECT_PERSISTENT,

        .min           = 0.0f,
        .max           = 45.0f,
        .default_value = 42.0f,

        .runtime       = &g_ovp_limit,

        .storage       = STORAGE_ID_W25Q64,
    },
};

然后 UF4COM 收到：

SET 0x0101 15.0

直接：

UF4Object_Set(0x0101, &value);

Parameter Manager 根本不需要知道这是 UF4COM 发来的。

这样 UF4COM 会变得非常漂亮

例如：

UF4COM_GetObject(id);
UF4COM_SetObject(id, data);

内部：

UF4COM
↓
Object ID
↓
Object Registry
↓
Descriptor
↓
权限检查
↓
类型检查
↓
范围检查
↓
Validator
↓
Runtime Value

如果这个对象带：

UF4_OBJECT_PERSISTENT

那么：

SET
↓
RAM修改
↓
Dirty

而：

COMMIT
↓
Parameter Manager
↓
Storage Manager
↓
AB Snapshot
更关键的是：不是所有 UF4COM ID 都必须是参数

这是我建议你设计时特别注意的一点。

你的 UF4COM 以后肯定不只有参数。

例如：

0x0100  VOUT_TARGET
0x0101  IOUT_LIMIT
0x0102  OVP_LIMIT

0x1000  VOUT
0x1001  IOUT
0x1002  VIN
0x1003  IIN
0x1004  TEMPERATURE

0x2000  DEVICE_INFO
0x2001  FIRMWARE_VERSION

0x3000  START
0x3001  STOP
0x3002  CLEAR_FAULT

0x4000  PARAM_COMMIT
0x4001  PARAM_RESET

这里：

VOUT_TARGET
IOUT_LIMIT
OVP_LIMIT

是 Parameter。

而：

VOUT
IOUT
VIN
IIN

是 Runtime Measurement。

START
STOP
CLEAR_FAULT

是 Command。

所以你的 UF4COM 最好定义的是：

Object Registry，而不是 Parameter Table。

可以增加 Object Type

例如：

typedef enum
{
UF4_OBJECT_PARAMETER = 0,
UF4_OBJECT_MEASUREMENT,
UF4_OBJECT_COMMAND,
UF4_OBJECT_STATUS,
UF4_OBJECT_INFORMATION,

} UF4ObjectType_t;

然后：

typedef struct
{
uint16_t id;

    const char *name;

    UF4ObjectType_t object_type;

    ParamType_t data_type;

    uint16_t flags;

    float min;
    float max;
    float default_value;

    void *runtime;

    StorageID_t storage;

    ParamValidator_t validator;

} UF4ObjectDescriptor_t;

这样一个 Registry 就统一整个 UF4COM。

Flags 也统一

比如：

#define UF4_FLAG_READ          (1U << 0)
#define UF4_FLAG_WRITE         (1U << 1)
#define UF4_FLAG_PERSISTENT    (1U << 2)
#define UF4_FLAG_RUNTIME       (1U << 3)
#define UF4_FLAG_REBOOT        (1U << 4)
#define UF4_FLAG_FACTORY       (1U << 5)
#define UF4_FLAG_HIDDEN        (1U << 6)
#define UF4_FLAG_READONLY      (1U << 7)

于是上位机甚至可以直接通过协议查询：

ID
Name
Type
Flags
Min
Max
Default

然后 F4CP 上位机可以动态生成参数页面。

这对你现在的 F4CP 特别有价值。

你甚至不需要在 Python 里硬编码：

if id == 0x0101:
...
elif id == 0x0102:
...

而是：

G474
↓
UF4COM
↓
GET_OBJECT_TABLE
↓
F4CP
↓
动态构建参数 UI

最终就变成：

                 ┌─────────────────────┐
                 │ UF4 Object Registry │
                 └──────────┬──────────┘
                            │
             ┌──────────────┼──────────────┐
             ↓              ↓              ↓
          UF4COM       Parameter       F4CP
          Protocol      Manager       Upper Computer
             │              │              │
             ↓              ↓              ↓
          GET/SET        AB Flash       Dynamic UI

这套架构非常适合你现在的 UF4 系列项目。

甚至以后 UF4FOC、UF4POWER、UF4DAP 不同固件，只需要各自提供自己的 Object Registry，而 UF4COM 协议层、Parameter Manager、Storage Manager 和 F4CP 的对象解析机制都可以复用。

我会把最终的核心 API 定成：

UF4Object_Find(id);

UF4Object_Get(id, ...);
UF4Object_Set(id, ...);

UF4Object_GetInfo(id, ...);

Param_Load();
Param_Save();
Param_Commit();
Param_Rollback();
Param_Reset();
Param_ResetAll();

Storage_Init();
Storage_Load();
Storage_Commit();
Storage_Verify();

其中最重要的原则就是：

ID 表只有一份，Descriptor 只有一份；UF4COM、参数管理器、F4CP 都消费这张表，而不是各自维护一套 ID。

这会让你的整个 UF4 软件栈统一很多。