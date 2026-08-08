cmake_minimum_required(VERSION 3.16)

project(vitte_bootstrap C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE VITTE_BOOTSTRAP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(vitte-bootstrap ${VITTE_BOOTSTRAP_SOURCES})

target_compile_options(vitte-bootstrap PRIVATE
    -Wall
    -Wextra
    -Werror
    -pedantic
)
