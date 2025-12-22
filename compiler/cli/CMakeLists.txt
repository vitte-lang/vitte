# Vitte Compiler - CLI Build Configuration
cmake_minimum_required(VERSION 3.16)
project(vitte-cli VERSION 1.0.0 LANGUAGES C)

# ============================================================================
# Compiler Settings
# ============================================================================

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler-specific flags
if(MSVC)
    add_compile_options(/W4 /WX)  # Warnings as errors on MSVC
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)  # GCC/Clang
endif()

# ============================================================================
# Source Files
# ============================================================================

set(CLI_HEADERS
    cli.h
    cli_commands.h
    config.h
)

set(CLI_SOURCES
    cli.c
    cli_commands.c
    config.c
    main.c
)

# ============================================================================
# Library Target
# ============================================================================

add_library(vitte-cli-lib STATIC
    cli.c
    cli_commands.c
    config.c
)

target_include_directories(vitte-cli-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(vittec
    ${CLI_SOURCES}
)

target_include_directories(vittec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link the library to executable
target_link_libraries(vittec PRIVATE vitte-cli-lib)

# ============================================================================
# Platform-Specific Settings
# ============================================================================

if(WIN32)
    # Windows-specific settings
    target_compile_definitions(vittec PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS vittec
    RUNTIME DESTINATION bin
)

install(FILES ${CLI_HEADERS}
    DESTINATION include/vitte/cli
)

# ============================================================================
# Testing
# ============================================================================

enable_testing()

# Test executable for CLI
add_executable(test-cli
    cli.c
    cli_commands.c
    config.c
    tests/test_cli.c
)

target_include_directories(test-cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME CLI_Tests COMMAND test-cli)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "Vitte CLI Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER}")
