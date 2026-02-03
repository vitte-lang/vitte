# ============================================================
# vitte_codegen_llvm — CMake configuration
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(vitte_codegen_llvm
    LANGUAGES CXX
    VERSION 0.1.0
)

# ------------------------------------------------------------
# Options
# ------------------------------------------------------------

option(VITTE_LLVM_BUILD_TESTS "Build LLVM backend tests" ON)

# ------------------------------------------------------------
# C++ standard
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# LLVM configuration
# ------------------------------------------------------------

# LLVM must be discoverable via llvm-config or CMake config
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# LLVM utilities
llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
    passes
    analysis
    target
    asmparser
    asmprinter
    codegen
    transformutils
)

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------

set(VITTE_LLVM_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(VITTE_LLVM_SRC
    src/emit/emit.cpp
    src/lower/expr.cpp
    src/lower/function.cpp
    src/lower/module.cpp
    src/passes/passes.cpp
)

set(VITTE_LLVM_HEADERS
    include/vitte/codegen/llvm/context.hpp
    include/vitte/codegen/llvm/emit.hpp
    include/vitte/codegen/llvm/passes.hpp
)

# ------------------------------------------------------------
# Library target
# ------------------------------------------------------------

add_library(vitte_codegen_llvm STATIC
    ${VITTE_LLVM_SRC}
    ${VITTE_LLVM_HEADERS}
)

target_include_directories(vitte_codegen_llvm
    PUBLIC
        ${VITTE_LLVM_INCLUDE_DIR}
)

target_link_libraries(vitte_codegen_llvm
    PUBLIC
        ${LLVM_LIBS}
)

target_compile_definitions(vitte_codegen_llvm
    PRIVATE
        VITTE_LLVM_BACKEND
)

# Silence some LLVM warnings on MSVC
if (MSVC)
    target_compile_options(vitte_codegen_llvm PRIVATE /EHsc)
endif()

# ------------------------------------------------------------
# Smoke test
# ------------------------------------------------------------

if (VITTE_LLVM_BUILD_TESTS)

    enable_testing()

    add_executable(vitte_codegen_llvm_smoke
        tests/smoke.cpp
    )

    target_link_libraries(vitte_codegen_llvm_smoke
        PRIVATE
            vitte_codegen_llvm
            ${LLVM_LIBS}
    )

    target_include_directories(vitte_codegen_llvm_smoke
        PRIVATE
            ${VITTE_LLVM_INCLUDE_DIR}
    )

    add_test(
        NAME vitte_codegen_llvm_smoke
        COMMAND vitte_codegen_llvm_smoke
    )

endif()

# ------------------------------------------------------------
# Installation (optionnelle)
# ------------------------------------------------------------

install(
    TARGETS vitte_codegen_llvm
    ARCHIVE DESTINATION lib
)

install(
    DIRECTORY include/
    DESTINATION include
)

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

message(STATUS "vitte_codegen_llvm configured")
message(STATUS "  LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "  Tests enabled: ${VITTE_LLVM_BUILD_TESTS}")
message(STATUS "  Library target: vitte_codegen_llvm")
message(STATUS "  Include dir: ${VITTE_LLVM_INCLUDE_DIR}")
message(STATUS "  Source files: ${VITTE_LLVM_SRC}")