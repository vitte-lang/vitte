# ============================================================
# vitte_llvm — LLVM backend for Vitte compiler
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(vitte_llvm
    VERSION 0.1.0
    LANGUAGES C CXX
)

# ------------------------------------------------------------
# Global options
# ------------------------------------------------------------

option(VITTE_LLVM_ENABLE_LTO        "Enable LTO for vitte_llvm"        OFF)
option(VITTE_LLVM_ENABLE_SANITIZERS "Enable sanitizers"               OFF)
option(VITTE_LLVM_BUILD_TESTS       "Build vitte_llvm tests"          OFF)
option(VITTE_LLVM_STATIC_LLVM       "Link LLVM statically if possible" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# LLVM discovery
# ------------------------------------------------------------

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# LLVM helpers
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include(HandleLLVMOptions)

# Map LLVM components to libs
llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
    Analysis
    TransformUtils
    ScalarOpts
    Passes
    Target
    MC
    MCParser
    MCDisassembler
    BitWriter
    BitReader
)

# ------------------------------------------------------------
# Source layout
# ------------------------------------------------------------

set(VITTE_LLVM_SRC
    src/backend.cpp
    src/context.cpp
    src/codegen.cpp
    src/lowering.cpp
    src/abi.cpp
    src/target.cpp
    src/passes.cpp
)

set(VITTE_LLVM_HEADERS
    include/vitte_llvm/backend.hpp
    include/vitte_llvm/context.hpp
    include/vitte_llvm/codegen.hpp
    include/vitte_llvm/lowering.hpp
    include/vitte_llvm/abi.hpp
    include/vitte_llvm/target.hpp
    include/vitte_llvm/passes.hpp
)

# ------------------------------------------------------------
# Library target
# ------------------------------------------------------------

add_library(vitte_llvm STATIC
    ${VITTE_LLVM_SRC}
    ${VITTE_LLVM_HEADERS}
)

add_library(vitte::llvm ALIAS vitte_llvm)

target_include_directories(vitte_llvm
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(vitte_llvm
    PRIVATE
        ${LLVM_DEFINITIONS}
)

target_link_libraries(vitte_llvm
    PUBLIC
        ${LLVM_LIBS}
)

# ------------------------------------------------------------
# Compile flags
# ------------------------------------------------------------

if (MSVC)
    target_compile_options(vitte_llvm PRIVATE
        /EHsc
        /W4
    )
else()
    target_compile_options(vitte_llvm PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

# ------------------------------------------------------------
# LTO
# ------------------------------------------------------------

if (VITTE_LLVM_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if (ipo_supported)
        set_property(TARGET vitte_llvm PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
        message(STATUS "LTO enabled for vitte_llvm")
    else()
        message(WARNING "LTO not supported: ${ipo_error}")
    endif()
endif()

# ------------------------------------------------------------
# Sanitizers
# ------------------------------------------------------------

if (VITTE_LLVM_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(vitte_llvm PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(vitte_llvm PRIVATE
        -fsanitize=address,undefined
    )
endif()

# ------------------------------------------------------------
# Static LLVM (optional)
# ------------------------------------------------------------

if (VITTE_LLVM_STATIC_LLVM)
    message(STATUS "Attempting static LLVM linkage")
    target_link_options(vitte_llvm PRIVATE -static)
endif()

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

if (VITTE_LLVM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ------------------------------------------------------------
# Install
# ------------------------------------------------------------

install(TARGETS vitte_llvm
    EXPORT vitte_llvm_targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

install(EXPORT vitte_llvm_targets
    FILE vitte_llvmTargets.cmake
    NAMESPACE vitte::
    DESTINATION lib/cmake/vitte_llvm
)

# ------------------------------------------------------------
# Summary
# ------------------------------------------------------------

message(STATUS "vitte_llvm configuration:")
message(STATUS "  LLVM version:        ${LLVM_PACKAGE_VERSION}")
message(STATUS "  Build tests:         ${VITTE_LLVM_BUILD_TESTS}")
message(STATUS "  LTO:                 ${VITTE_LLVM_ENABLE_LTO}")
message(STATUS "  Sanitizers:          ${VITTE_LLVM_ENABLE_SANITIZERS}")
message(STATUS "  Static LLVM:         ${VITTE_LLVM_STATIC_LLVM}")
