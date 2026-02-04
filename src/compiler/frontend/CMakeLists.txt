# ============================================================
# compiler/frontend/CMakeLists.txt — Vitte Compiler
# Frontend build configuration
# ============================================================

cmake_minimum_required(VERSION 3.20)

# ------------------------------------------------------------
# Target
# ------------------------------------------------------------

add_library(vitte_frontend STATIC
    ast.cpp
    diagnostics.cpp
    lexer.cpp
    parser.cpp
)

add_library(vitte::frontend ALIAS vitte_frontend)

# ------------------------------------------------------------
# Include paths
# ------------------------------------------------------------

target_include_directories(vitte_frontend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ------------------------------------------------------------
# Language / standard
# ------------------------------------------------------------

target_compile_features(vitte_frontend
    PUBLIC
        cxx_std_20
)

# ------------------------------------------------------------
# Warnings (strict but sane)
# ------------------------------------------------------------

if (MSVC)
    target_compile_options(vitte_frontend PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
else()
    target_compile_options(vitte_frontend PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wunused
    )
endif()

# ------------------------------------------------------------
# Compile definitions
# ------------------------------------------------------------

target_compile_definitions(vitte_frontend
    PRIVATE
        VITTE_FRONTEND
)

# ------------------------------------------------------------
# Sanitizers (optional)
# ------------------------------------------------------------

option(VITTE_ENABLE_SANITIZERS "Enable sanitizers for frontend" OFF)

if (VITTE_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(vitte_frontend PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(vitte_frontend PRIVATE
        -fsanitize=address,undefined
    )
endif()

# ------------------------------------------------------------
# LTO (optional)
# ------------------------------------------------------------

option(VITTE_ENABLE_LTO "Enable LTO for frontend" OFF)

if (VITTE_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if (ipo_supported)
        set_target_properties(vitte_frontend PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE
        )
    endif()
endif()

# ------------------------------------------------------------
# Tests hook (if enabled at root)
# ------------------------------------------------------------

if (VITTE_ENABLE_TESTS)
    add_subdirectory(tests)
endif()