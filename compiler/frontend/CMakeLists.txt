# Vitte Compiler - Frontend Build Configuration
cmake_minimum_required(VERSION 3.16)
project(vitte-frontend VERSION 1.0.0 LANGUAGES C)

# ============================================================================
# Compiler Settings
# ============================================================================

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler-specific flags
if(MSVC)
    add_compile_options(/W4)  # Warnings
else()
    add_compile_options(-Wall -Wextra -pedantic)  # GCC/Clang
endif()

# ============================================================================
# Source Files
# ============================================================================

set(FRONTEND_HEADERS
    lexer.h
    parser.h
    semantic.h
)

set(FRONTEND_SOURCES
    lexer.c
    parser.c
    semantic.c
)

# ============================================================================
# Library Target
# ============================================================================

add_library(vitte-frontend STATIC
    ${FRONTEND_SOURCES}
)

target_include_directories(vitte-frontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# Test Executable
# ============================================================================

add_executable(test-frontend
    test_frontend.c
    ${FRONTEND_SOURCES}
)

target_include_directories(test-frontend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS vitte-frontend
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${FRONTEND_HEADERS}
    DESTINATION include/vitte/frontend
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "Vitte Frontend Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER}")
