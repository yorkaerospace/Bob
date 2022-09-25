cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(Bob C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(sources src/main.c
            src/usbcmd.c
            src/usbcmd.h
            src/sensors.c
            src/sensors.h
            src/core1.c
            src/core1.h)

add_executable(Bob
  src/main.c
  src/usbcmd.c
  src/sensors.c
  src/core1.c
)

add_subdirectory(lib)

target_link_libraries(Bob QMI8658C HP203B QMC5883L pico_stdlib pico_multicore hardware_i2c)

pico_enable_stdio_usb(Bob 1)

pico_add_extra_outputs(Bob)

