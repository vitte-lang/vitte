cmake_minimum_required(VERSION 3.10)
project(vitte_bootstrap_stage0 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2 -g")

# Source files
set(SOURCES
    main.c
    vitte_lexer.c
    vitte_ast.c
    vitte_compiler.c
)

set(HEADERS
    vitte_lexer.h
    vitte_ast.h
    vitte_compiler.h
)

# Create executable
add_executable(vittec-stage0 ${SOURCES})

# Include directories
target_include_directories(vittec-stage0 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set output directory
set_target_properties(vittec-stage0 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
