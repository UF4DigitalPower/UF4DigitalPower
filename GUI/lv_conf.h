/**
 * @file lv_conf.h
 * Configuration file for v8.4.0
 */

/*
 * Copy this file as `lv_conf.h`
 * 1. simply next to the `lvgl` folder
 * 2. or any other places and
 *    - define `LV_CONF_INCLUDE_SIMPLE`
 *    - add the path as include path
 */

/* clang-format off */
#if 1 /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>
#include <stdlib.h>

/*====================
   COLOR SETTINGS
 *====================*/

/*Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888)*/
#define LV_COLOR_DEPTH 16

/*交换 2 字节的 RGB565 颜色。如果显示器具有 8 位接口（例如 SPI），则很有用*/
#define LV_COLOR_16_SWAP 0

/*Enable features to draw on transparent background.
 *如果使用 opa 和 transform_* 样式属性，则需要它.
 *如果 UI 位于另一层之上，例如 OSD 菜单或视频播放器，也可以使用。*/
#define LV_COLOR_SCREEN_TRANSP 0

/* Adjust color mix functions rounding. GPUs might calculate color mix (blending) differently.
 * 0: round down, 64: round up from x.75, 128: round up from half, 192: round up from x.25, 254: round up */
#define LV_COLOR_MIX_ROUND_OFS 0

/*如果采用色度键控，则不会绘制具有此颜色的图像像素）*/
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)         /*pure green*/

/*=========================
   MEMORY SETTINGS
 *=========================*/

/*1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()`*/
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    /* SDRAM layout:
     * 0xC0000000~0xC01FFFFF: LTDC layer/framebuffer 1, reserved 2MB
     * 0xC0200000~0xC03FFFFF: LTDC layer/framebuffer 2, reserved 2MB
     * 0xC0400000~...       : LVGL draw buffers and heap
     * Two full-size LVGL draw buffers occupy 640*480*2*2 = 0x12C000 bytes.
     */
    #define LV_MEM_SIZE (0x1AD4000U)        /* 28131328 bytes */
    #define LV_MEM_ADR  0xC052C000U
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif

#else       /*LV_MEM_CUSTOM*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /*LV_MEM_CUSTOM*/

/*Number of the intermediate memory buffer used during rendering and other internal processing mechanisms.
 *You will see an error log message if there wasn't enough buffers. */
#define LV_MEM_BUF_MAX_NUM 32

/*Use the standard `memcpy` and `memset` instead of LVGL's own functions. (Might or might not be faster).*/
#define LV_MEMCPY_MEMSET_STD 1

/*====================
   HAL SETTINGS
 *====================*/

/*默认的显示刷新周期。LVG会在这段时间内重绘变化区域*/
#define LV_DISP_DEF_REFR_PERIOD 10      /*[ms]*/

/*输入设备读取周期，单位为毫秒*/
#define LV_INDEV_DEF_READ_PERIOD 10     /*[ms]*/

/*Use a custom tick source that tells the elapsed time in milliseconds.
 *It removes the need to manually update the tick with `lv_tick_inc()`)*/
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "stm32h7xx_hal.h"         /*Header for the system time function*/
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (HAL_GetTick())    /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

/*Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings.
 *(Not so important, you can adjust it to modify default sizes and spaces)*/
#define LV_DPI_DEF 130     /*[px/inch]*/

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*-------------
 * Drawing
 *-----------*/

/*Enable complex draw engine.
 *绘制阴影、渐变、圆角、圆形、圆弧、倾斜线、图像变换或任何蒙版时需要*/
#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX != 0

    /*Allow buffering some shadow calculation.
    *LV_SHADOW_CACHE_SIZE is the max. shadow size to buffer, where shadow size is `shadow_width + radius`
    *缓存的 RAM 成本为 LV_SHADOW_CACHE_SIZE^2*/
    #define LV_SHADOW_CACHE_SIZE 0

    /* Set number of maximally cached circle data.
    * The circumference of 1/4 circle are saved for anti-aliasing
    * radius * 4 bytes are used per circle (the most often used radiuses are saved)
    * 0: to disable caching */
    #define LV_CIRCLE_CACHE_SIZE 4
#endif /*LV_DRAW_COMPLEX*/

#define LV_LAYER_SIMPLE_BUF_SIZE          (64 * 1024)
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (8 * 1024)

#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_GRAD_CACHE_DEF_SIZE 0
#define LV_DITHER_GRADIENT 0
#if LV_DITHER_GRADIENT
    #define LV_DITHER_ERROR_DIFFUSION 0
#endif

#define LV_DISP_ROT_MAX_BUF (10*1024)

/*-------------
 * GPU
 *-----------*/

/* 暂时关闭 LVGL 内置 DMA2D GPU，避免它绕过当前显示 flush 判断。 */
#define LV_USE_GPU_ARM2D 0
#define LV_USE_GPU_STM32_DMA2D 0
#if LV_USE_GPU_STM32_DMA2D
    #define LV_GPU_DMA2D_CMSIS_INCLUDE "stm32h7xx.h"
#endif

#define LV_USE_GPU_RA6M3_G2D 0
#if LV_USE_GPU_RA6M3_G2D
    #define LV_GPU_RA6M3_G2D_INCLUDE "hal_data.h"
#endif

#define LV_USE_GPU_SWM341_DMA2D 0
#if LV_USE_GPU_SWM341_DMA2D
    #define LV_GPU_SWM341_DMA2D_INCLUDE "SWM341.h"
#endif

#define LV_USE_GPU_NXP_PXP 0
#if LV_USE_GPU_NXP_PXP
    #define LV_USE_GPU_NXP_PXP_AUTO_INIT 0
#endif

#define LV_USE_GPU_NXP_VG_LITE 0
#define LV_USE_GPU_SDL 0

#include "lv_conf.h"

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/