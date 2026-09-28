cmake_minimum_required(VERSION 3.16)

project(vitte_compiler C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE VITTE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(vitte ${VITTE_SOURCES})

target_compile_options(vitte PRIVATE
    -Wall
    -Wextra
    -Werror
    -pedantic
)
