# LVGL Benchmark (SDRAM Double Buffer)

This project runs `lv_demo_benchmark()` on boot.

This variant uses two full-screen framebuffers in external SDRAM and swaps LTDC address only at vertical blanking.

LVGL heap remains in internal RAM.

## Memory layout used

- `0xC0000000`: LTDC front framebuffer in external SDRAM
- `0xC0096000`: LTDC back framebuffer in external SDRAM
- LVGL heap: internal RAM, `128 KB`
- LVGL draw buffers: bound directly to the two full SDRAM framebuffers

## What was tuned for smoothness

- Two full external SDRAM framebuffers
- Full-screen redraw each frame (`full_refresh = 1`)
- LTDC address swap only on vertical blanking (reload event)
- Reduced LVGL heap size to fit on-chip memory
- Benchmark max-speed mode enabled
- LVGL task handler uses adaptive delay in main loop

## Build

```powershell
cmake -S E:\PROJECT_C\STM32H743 -B E:\PROJECT_C\STM32H743\cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build E:\PROJECT_C\STM32H743\cmake-build-debug -j
```

