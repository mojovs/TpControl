#此文件从模板自动生成! 请勿更改!
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.30)

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
project(TpControl C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#Uncomment for hardware floating point
#add_compile_definitions(ARM_MATH_CM4;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
#add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
#add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)

#Uncomment for software floating point
#add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=cortex-m3 -mthumb -mthumb-interwork)
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
    message(STATUS "Maximum optimization for size, debug info included")
    add_compile_options(-Os -g)
endif ()

include_directories(Core/Inc Drivers/STM32F1xx_HAL_Driver/Inc Drivers/STM32F1xx_HAL_Driver/Inc/Legacy Middlewares/Third_Party/FreeRTOS/Source/include Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 Drivers/CMSIS/Device/ST/STM32F1xx/Include Drivers/CMSIS/Include
        User
        User/Gui
        User/System/Tasks
        User/System/Stdio
        User/System/Resource
        User/System
        User/Hardeware
        User/Hardeware/Delay
        User/Hardeware/Fs
        User/Hardeware/Uart
        User/Hardeware/tim
        User/Hardeware/adc
        User/Hardeware/Lcd
        User/Hardeware/Spi
        User/Hardeware/Flash
        User/Hardeware/Buzz
        User/Hardeware/Temperature
        User/Hardeware/BLE
        User/Hardeware/Humidity
        User/Hardeware/RTC
        User/Hardeware/Control
        img
        img/icon_12x12
        img/icon_24x24
        img/icon_24x48
)

add_definitions(-DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB)

set(WRITE_IMAGE_GROUP "12x12" CACHE STRING "Image write batch: none, 12x12, 24x24, 48x24")
set_property(CACHE WRITE_IMAGE_GROUP PROPERTY STRINGS none 12x12 24x24 24x48)

set(IMAGE_SOURCES)
if ("${WRITE_IMAGE_GROUP}" STREQUAL "12x12")
    add_compile_definitions(WRITE_IMAGE_ICON_12X12=1 WRITE_IMAGE_ICON_24X24=0 WRITE_IMAGE_ICON_48x24=0)
    file(GLOB IMAGE_SOURCES "img/icon_12x12/*.c")
elseif ("${WRITE_IMAGE_GROUP}" STREQUAL "24x24")
    add_compile_definitions(WRITE_IMAGE_ICON_12X12=0 WRITE_IMAGE_ICON_24X24=1 WRITE_IMAGE_ICON_48x24=0)
    file(GLOB IMAGE_SOURCES "img/icon_24x24/*.c")
elseif ("${WRITE_IMAGE_GROUP}" STREQUAL "48x24")
    add_compile_definitions(WRITE_IMAGE_ICON_12X12=0 WRITE_IMAGE_ICON_24X24=0 WRITE_IMAGE_ICON_48x24=1)
    file(GLOB IMAGE_SOURCES "img/icon_24x48/*.c")
else ()
    add_compile_definitions(WRITE_IMAGE_ICON_12X12=0 WRITE_IMAGE_ICON_24X24=0 WRITE_IMAGE_ICON_48x24=0)
endif ()

option(WRITE_FONT_GB12 "Write img/font12.c to W25Q80 /font/GB12.bin" OFF)
option(WRITE_FONT_GB16 "Write img/font16.c to W25Q80 /font/GB16.bin" OFF)
option(WRITE_CJHR31 "Write CJHR31 lookup table to W25Q80 /sys/rh_table.bin" OFF)
option(WRITE_ASCII "Write ascii lookup table to W25Q80 " OFF)
option(FLASH_WRITE "Include font write-once data (ascii_1608, sz32, lfs_write functions)" OFF)

set(FONT_SOURCES)
set(CJHR31_SOURCES)
if (FLASH_WRITE)
    add_compile_definitions(FLASH_WRITE=1)
    list(APPEND FONT_SOURCES "User/Hardeware/Lcd/Font_Write.c")
else ()
    add_compile_definitions(FLASH_WRITE=0)
endif ()
if (WRITE_FONT_GB12)
    add_compile_definitions(WRITE_FONT_GB12=1)
else ()
    add_compile_definitions(WRITE_FONT_GB12=0)
endif ()

if (WRITE_FONT_GB16)
    add_compile_definitions(WRITE_FONT_GB16=1)
else ()
    add_compile_definitions(WRITE_FONT_GB16=0)
endif ()

if (WRITE_CJHR31)
    add_compile_definitions(WRITE_CJHR31=1)
    list(APPEND CJHR31_SOURCES "User/Hardeware/Humidity/cjhr31_table_write.c")
else ()
    add_compile_definitions(WRITE_CJHR31=0)
endif ()

file(GLOB_RECURSE SOURCES "Core/*.*" "Middlewares/*.*" "Drivers/*.*"
        "User/*.*"
        "User/Gui/*.*"
        "User/System/*.*"
        "User/System/Tasks/*.*"
        "User/System/Stdio/*.*"
        "User/Hardeware/Fs/*.*"
        "User/Hardeware/*.*"
        "User/Hardeware/Uart/*.*"
        "User/Hardeware/tim/*.*"
        "User/Hardeware/adc/*.*"
        "User/Hardeware/Lcd/*.*"
        "User/Hardeware/Spi/*.*"
        "User/Hardeware/Delay/*.*"
        "User/Hardeware/Flash/*.*"
        "User/Hardeware/Buzz/*.*"
        "User/Hardeware/Temperature/*.*"
        "User/Hardeware/BLE/*.*"
        "User/Hardeware/Humidity/*.*"
        "User/Hardeware/RTC/*.*"
        "User/Hardeware/Control/*.*"
)
list(REMOVE_ITEM SOURCES
        "${CMAKE_SOURCE_DIR}/User/System/Resource/resource_uart.c"
        "${CMAKE_SOURCE_DIR}/User/System/Resource/ymodem.c"
        "${CMAKE_SOURCE_DIR}/User/Hardeware/Uart/retarget.c"
        "${CMAKE_SOURCE_DIR}/User/Hardeware/Lcd/Font_Write.c"
        "${CMAKE_SOURCE_DIR}/User/Hardeware/Humidity/cjhr31_table_write.c"
        "User/System/Resource/resource_uart.c"
        "User/System/Resource/ymodem.c"
        "User/Hardeware/Uart/retarget.c"
        "User/Hardeware/Lcd/Font_Write.c"
        "User/Hardeware/Humidity/cjhr31_table_write.c"
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32F103C8TX_FLASH.ld )

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m3 -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})
 add_link_options(-specs=nano.specs -specs=nosys.specs)

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${IMAGE_SOURCES} ${FONT_SOURCES} ${CJHR31_SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
