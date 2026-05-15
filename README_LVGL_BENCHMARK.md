# LVGL Benchmark (Internal RAM first)

This project runs `lv_demo_benchmark()` on boot.

LVGL working memory is now kept in internal RAM first:

- LVGL heap is allocated in internal RAM
- LVGL draw buffers are small partial buffers in internal RAM

The display framebuffer still lives in external SDRAM because a full 480×640 RGB565 framebuffer cannot fit in on-chip RAM.

## Memory layout used

- `0xC0000000`: LTDC front framebuffer in external SDRAM
- `0xC0096000`: LTDC back framebuffer in external SDRAM
- LVGL heap: internal RAM, `128 KB`
- LVGL draw buffers: internal RAM, `2 x 20 lines`

## What was tuned for smoothness

- Internal-RAM draw buffers
- All flush regions are rendered to the SDRAM back framebuffer
- LTDC address is swapped only on vertical blanking (reload event)
- Reduced LVGL heap size to fit on-chip memory
- Benchmark max-speed mode enabled
- LVGL task handler uses adaptive delay in main loop

## Build

```powershell
cmake -S E:\PROJECT_C\STM32H743 -B E:\PROJECT_C\STM32H743\cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build E:\PROJECT_C\STM32H743\cmake-build-debug -j
```

