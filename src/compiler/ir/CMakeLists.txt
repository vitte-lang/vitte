cmake_minimum_required(VERSION 3.16)

project(vitte_ir LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# IR sources
# --------------------------------------------------

set(VITTE_IR_SOURCES
    hir.cpp
    hir.hpp
    validate.cpp
    validate.hpp

    mir.cpp
    mir.hpp

    ir_builder.cpp
    ir_builder.hpp
)

# --------------------------------------------------
# IR library
# --------------------------------------------------

add_library(vitte_ir STATIC
    ${VITTE_IR_SOURCES}
)

target_include_directories(vitte_ir
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# --------------------------------------------------
# Warnings
# --------------------------------------------------

if (MSVC)
    target_compile_options(vitte_ir PRIVATE /W4)
else()
    target_compile_options(vitte_ir PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# --------------------------------------------------
# Export
# --------------------------------------------------

set_target_properties(vitte_ir PROPERTIES
    OUTPUT_NAME "vitte_ir"
)
