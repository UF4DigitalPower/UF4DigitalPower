#此文件从模板自动生成! 请勿更改!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.28.1)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(UF4DigitalPower C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#Enable hardware floating point (Cortex-M4F)
add_compile_definitions(ARM_MATH_CM4)
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

add_compile_options(-mcpu=cortex-m4 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

set(UF4COM_ROOT "${CMAKE_SOURCE_DIR}/UF4COM")

include_directories(
        Core/Inc
        USB_Device/App
        USB_Device/Target
        Drivers/STM32G4xx_HAL_Driver/Inc
        Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
        Middlewares/ST/STM32_USB_Device_Library/Core/Inc
        Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
        Drivers/CMSIS/Device/ST/STM32G4xx/Include
        Drivers/CMSIS/Include
        ${UF4COM_ROOT}/uf4com/protocol/Inc
        ${UF4COM_ROOT}/uf4com/register/Inc
)

add_definitions(-DDEBUG -DUSE_HAL_DRIVER -DSTM32G474xx)

file(GLOB_RECURSE SOURCES "USB_Device/*.*" "Core/*.*" "Middlewares/*.*" "Drivers/*.*")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/Core/Src/startup_stm32g474xx.s")
list(APPEND SOURCES
        "${UF4COM_ROOT}/uf4com/protocol/Src/uf4com.c"
        "${UF4COM_ROOT}/uf4com/protocol/Src/uf4com_cmd.c"
        "${UF4COM_ROOT}/uf4com/protocol/Src/uf4com_crc.c"
        "${UF4COM_ROOT}/uf4com/protocol/Src/uf4com_parser.c"
        "${UF4COM_ROOT}/uf4com/register/Src/uf4com_id_table.c"
)


set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32G474CBTX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m4 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

set(UF4_BANNER_TXT "${CMAKE_BINARY_DIR}/uf4_banner.txt")
set(UF4_BANNER_SCRIPT "${CMAKE_BINARY_DIR}/uf4_generate_banner.cmake")
file(WRITE "${UF4_BANNER_SCRIPT}" [=[
string(TIMESTAMP UF4_BUILD_TIME "%Y-%m-%d %H:%M:%S")
file(WRITE "${UF4_BANNER_TXT}" "

  ██╗   ██╗███████╗██╗  ██╗██████╗ ██╗ ██████╗ ██╗████████╗ █████╗ ██╗     ██████╗  ██████╗ ██╗    ██╗███████╗██████╗
  ██║   ██║██╔════╝██║  ██║██╔══██╗██║██╔════╝ ██║╚══██╔══╝██╔══██╗██║     ██╔══██╗██╔═══██╗██║    ██║██╔════╝██╔══██╗
  ██║   ██║█████╗  ███████║██║  ██║██║██║  ███╗██║   ██║   ███████║██║     ██████╔╝██║   ██║██║ █╗ ██║█████╗  ██████╔╝
  ██║   ██║██╔══╝  ╚════██║██║  ██║██║██║   ██║██║   ██║   ██╔══██║██║     ██╔═══╝ ██║   ██║██║███╗██║██╔══╝  ██╔══██╗
  ╚██████╔╝██║          ██║██████╔╝██║╚██████╔╝██║   ██║   ██║  ██║███████╗██║     ╚██████╔╝╚███╔███╔╝███████╗██║  ██║
   ╚═════╝ ╚═╝          ╚═╝╚═════╝ ╚═╝ ╚═════╝ ╚═╝   ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝      ╚═════╝  ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝

  ██████╗ ██╗   ██╗██╗██╗     ██████╗     ███████╗██╗   ██╗ ██████╗ ██████╗███████╗███████╗███████╗
  ██╔══██╗██║   ██║██║██║     ██╔══██╗    ██╔════╝██║   ██║██╔════╝██╔════╝██╔════╝██╔════╝██╔════╝
  ██████╔╝██║   ██║██║██║     ██║  ██║    ███████╗██║   ██║██║     ██║     █████╗  ███████╗███████╗
  ██╔══██╗██║   ██║██║██║     ██║  ██║    ╚════██║██║   ██║██║     ██║     ██╔══╝  ╚════██║╚════██║
  ██████╔╝╚██████╔╝██║███████╗██████╔╝    ███████║╚██████╔╝╚██████╗╚██████╗███████╗███████║███████║
  ╚═════╝  ╚═════╝ ╚═╝╚══════╝╚═════╝     ╚══════╝ ╚═════╝  ╚═════╝ ╚═════╝╚══════╝╚══════╝╚══════╝

  ┌──────────────────────────────────────────────────────────────────────────────┐
  │ Project      : ${UF4_PROJECT_NAME}
  │ Build Type   : ${UF4_BUILD_TYPE}
  │ Build Time   : ${UF4_BUILD_TIME}
  │ Host Platform: ${UF4_HOST_SYSTEM_NAME} ${UF4_HOST_SYSTEM_PROCESSOR}
  │ Target System: ${UF4_TARGET_SYSTEM_NAME}
  │ MCU          : STM32G474CBTx / STM32G474xx
  │ Memory       : FLASH 128 KB, RAM 128 KB, CCMRAM 32 KB
  │ Compiler     : ${UF4_C_COMPILER_ID} ${UF4_C_COMPILER_VERSION}
  │ Toolchain    : ${UF4_C_COMPILER}
  │ Output       : ${UF4_OUTPUT_ELF}
  └──────────────────────────────────────────────────────────────────────────────┘

  UF4DigitalPower firmware generated successfully.

")
]=])

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMAND ${CMAKE_COMMAND}
                -DUF4_BANNER_TXT=${UF4_BANNER_TXT}
                -DUF4_PROJECT_NAME=${PROJECT_NAME}
                -DUF4_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DUF4_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME}
                -DUF4_HOST_SYSTEM_PROCESSOR=${CMAKE_HOST_SYSTEM_PROCESSOR}
                -DUF4_TARGET_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
                -DUF4_C_COMPILER_ID=${CMAKE_C_COMPILER_ID}
                -DUF4_C_COMPILER_VERSION=${CMAKE_C_COMPILER_VERSION}
                -DUF4_C_COMPILER=${CMAKE_C_COMPILER}
                -DUF4_OUTPUT_ELF=$<TARGET_FILE:${PROJECT_NAME}.elf>
                -P "${UF4_BANNER_SCRIPT}"
        COMMAND ${CMAKE_COMMAND} -E cat "${UF4_BANNER_TXT}"
        COMMENT "Generating firmware files"
        VERBATIM
)
