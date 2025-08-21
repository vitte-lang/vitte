cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
project(vitte-blink-pico C CXX ASM)
pico_sdk_init()

add_executable(vitte-blink
  vitte_ffi_rp2040.c
)
target_link_libraries(vitte-blink pico_stdlib)
pico_enable_stdio_usb(vitte-blink 1)
pico_enable_stdio_uart(vitte-blink 0)
pico_add_extra_outputs(vitte-blink)
