# LVGL 演示

## 将示例添加到您的项目中
1. 克隆 LVGL 后，可以在“演示”文件夹中找到演示。

2. 在 ***lv_conf.h*** 或等效的地方，您可以找到与演示相关的宏，更改其值以启用或禁用指定的演示：

'''c
...
/*===================
 * 演示用法
 ====================*/

/*显示一些小部件。可能需要增加“LV_MEM_SIZE”*/
#define LV_USE_DEMO_WIDGETS 0
#if LV_USE_DEMO_WIDGETS
#define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/*演示编码器和键盘的用法*/
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/*对您的系统进行基准测试*/
#define LV_USE_DEMO_BENCHMARK 0

/*LVGL的压力测试*/
#define LV_USE_DEMO_STRESS 0

/*音乐播放器演示*/
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
# 定义 LV_DEMO_MUSIC_SQUARE 0
# 定义 LV_DEMO_MUSIC_LANDSCAPE 0
# 定义 LV_DEMO_MUSIC_ROUND 0
# 定义 LV_DEMO_MUSIC_LARGE 0
# 定义 LV_DEMO_MUSIC_AUTO_PLAY 0
#endif
...
```

3. 如果您的开发环境或工具链没有自动在“***lvgl***”文件夹中添加源文件，请确保包含“demos”文件夹进行编译。
4. 在应用程序源文件中包含“***demos/lv_demos.h***”，例如：

'''c
//!main.c
#include “lvgl.h”
#include “demos/lv_demos.h”
...
```

## 演示

### 小部件
显示使用内置材质主题的控件开箱即用的外观。

请参阅 [widgets]（https://github.com/lvgl/lvgl/tree/master/demos/widgets） 文件夹。

<img src=“https://github.com/lvgl/lvgl/tree/master/demos/widgets/screenshot1.png?raw=true” width=600px alt=“展示 LVGL 小部件的基本演示”>

为了正确运行此演示，请确保 **LV_MEM_SIZE** 至少为 **38KB**（建议使用 **48KB**）：

'''c
#define LV_MEME_SIZE（38ul*1024ul）
```

### 音乐播放器
音乐播放器演示展示了在 LVGL 上可以创建什么样的现代、类似智能手机的用户界面。它最适合分辨率为 480x272 或 272x480 的显示器。

请参阅 [music]（https://github.com/lvgl/lvgl/tree/master/demos/music） 文件夹。

<img src=“https://github.com/lvgl/lvgl/tree/master/demos/music/screenshot1.gif?raw=true” width=600px alt=“带 LVGL 的音乐播放器演示”>

### 键盘和编码器
LVGL 允许您使用键盘和/或编码器控制小部件，而无需触摸板。此演示演示如何在没有触摸板的情况下处理按钮、下拉列表、滚轮、滑块、开关和文本输入。
了解有关 LVGL 无触摸板使用的更多信息 [此处]（https://docs.lvgl.io/master/overview/indev.html#keypad-and-encoder）。

请参阅 [keypad_encoder]（https://github.com/lvgl/lvgl/tree/master/demos/keypad_encoder） 文件夹。

<img src=“https://github.com/lvgl/lvgl/tree/master/demos/keypad_encoder/screenshot1.png?raw=true” width=600px alt=“LVGL 嵌入式 GUI 库中的键盘和编码器导航”>

### 基准测试
用于测量 LVGL 性能或比较不同设置的演示。
请参阅 [benchmark]（https://github.com/lvgl/lvgl/tree/master/demos/benchmark） 文件夹。
<img src=“https://github.com/lvgl/lvgl/tree/master/demos/benchmark/screenshot1.png?raw=true” width=600px alt=“使用 LVGL 嵌入式 GUI 库的基准测试演示”>

### 压力
LVGL 的压力测试。它包含大量对象创建、删除、动画、样式使用等。如果在大量使用期间出现任何内存损坏或任何内存泄漏，则可以使用它。
请参阅 [stress]（https://github.com/lvgl/lvgl/tree/master/demos/stress） 文件夹。
<img src=“https://github.com/lvgl/lvgl/tree/master/demos/stress/screenshot1.png?raw=true” width=600px alt=“LVGL 的压力测试”>

## 贡献
有关贡献和编码风格指南，请参阅 LVGL 主存储库中的文件 docs/CONTRIBUTING.md：
  https://github.com/lvgl/lvgl