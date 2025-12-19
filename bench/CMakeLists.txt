cmake_minimum_required(VERSION 3.20)
project(vitte_bench C)

# ============================================================================
# Configuration
# ============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Platform detection
if(APPLE)
  add_compile_definitions(BENCH_PLATFORM_MACOS)
elseif(UNIX)
  add_compile_definitions(BENCH_PLATFORM_LINUX)
elseif(WIN32)
  add_compile_definitions(BENCH_PLATFORM_WINDOWS)
endif()

# Compiler detection
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(NOT MSVC)
    add_compile_options(-march=native)
  endif()
elseif(MSVC)
  add_compile_options(/W4)
endif()

# ============================================================================
# Include directories
# ============================================================================

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

option(VITTE_ENABLE_ASM_FASTPATHS "Build and link the Vitte .S fastpaths" OFF)
option(VITTE_ENABLE_RUST_API "Build and link Rust FFI (vitte_rust_api)" OFF)

# ============================================================================
# Framework source files (bench/)
# ============================================================================

set(BENCH_FRAMEWORK_SOURCES
  src/bench/runner.c
  src/bench/register_builtin.c
  src/bench/options.c
  src/bench/output.c
  src/bench/registry.c
  src/bench/benchmark_init.c
  src/bench/bench_time.c
  src/bench/bench_stats.c
  src/bench/bench.c
  src/bench/csv.c
  src/bench/format.c
  src/bench/log.c
  src/bench/json_parser.c
  src/bench/timing.c
)

set(BENCH_FRAMEWORK_HEADERS
  src/bench/bench.h
  src/bench/benchmark_init.h
  src/bench/types.h
  src/bench/config.h
  src/bench/platform.h
  src/bench/common.h
  src/bench/runner.h
  src/bench/options.h
  src/bench/output.h
  src/bench/timing.h
  src/bench/sample.h
  src/bench/json_parser.h
  src/bench/alloc.h
  src/bench/strutil.h
  src/bench/format.h
  src/bench/csv.h
  src/bench/log.h
)

# ============================================================================
# Micro-benchmark source files
# ============================================================================

set(BENCH_MICRO_SOURCES
  src/micro/bm_hash.c
  src/micro/bm_add.c
  src/micro/bm_array_access.c
  src/micro/bm_bitops.c
  src/micro/bm_branch_prediction.c
  src/micro/bm_cache_line_effects.c
  src/micro/bm_conditional_move.c
  src/micro/bm_data_dependency.c
  src/micro/bm_division.c
  src/micro/bm_floating_point.c
  src/micro/bm_function_call.c
  src/micro/bm_loop_unroll.c
  src/micro/bm_memcpy.c
  src/micro/bm_recursion.c
  src/micro/bm_string_search.c
)

# ============================================================================
# Macro-benchmark source files
# ============================================================================

set(BENCH_MACRO_SOURCES
  src/macro/bm_cache_effects.c
  src/macro/bm_compression.c
  src/macro/bm_file_io_sim.c
  src/macro/bm_json_parse.c
  src/macro/bm_math_compute.c
  src/macro/bm_regex_match.c
  src/macro/bm_sort_algorithms.c
  src/macro/bm_string_ops.c
)

# ============================================================================
# Combine all sources
# ============================================================================

set(ALL_SOURCES
  ${BENCH_FRAMEWORK_SOURCES}
  ${BENCH_MICRO_SOURCES}
  ${BENCH_MACRO_SOURCES}
)

if(VITTE_ENABLE_ASM_FASTPATHS)
  enable_language(ASM)

  set(VITTE_ASM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../src/asm)

  set(VITTE_ASM_C_SOURCES
    ${VITTE_ASM_ROOT}/src/vitte/asm_dispatch.c
    ${VITTE_ASM_ROOT}/src/vitte/asm_verify.c
    ${VITTE_ASM_ROOT}/src/vitte/cpu.c
  )

  set(VITTE_ASM_X86_64_SOURCES
    ${VITTE_ASM_ROOT}/src/asm/x86_64/hash/vitte_fnv1a64.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/memcpy/vitte_memcpy_baseline.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/memcpy/vitte_memcpy_sse2.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/memcpy/vitte_memcpy_avx2.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/memset/vitte_memset_baseline.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/memset/vitte_memset_sse2.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/memset/vitte_memset_avx2.S
    ${VITTE_ASM_ROOT}/src/asm/x86_64/utf8/vitte_utf8_validate_stub.S
  )

  set(VITTE_ASM_AARCH64_SOURCES
    ${VITTE_ASM_ROOT}/src/asm/aarch64/hash/vitte_fnv1a64.S
    ${VITTE_ASM_ROOT}/src/asm/aarch64/memcpy/vitte_memcpy_baseline.S
    ${VITTE_ASM_ROOT}/src/asm/aarch64/memcpy/vitte_memcpy_neon.S
    ${VITTE_ASM_ROOT}/src/asm/aarch64/memset/vitte_memset_baseline.S
    ${VITTE_ASM_ROOT}/src/asm/aarch64/memset/vitte_memset_neon.S
    ${VITTE_ASM_ROOT}/src/asm/aarch64/utf8/vitte_utf8_validate_stub.S
  )

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(VITTE_ASM_ARCH_SOURCES ${VITTE_ASM_X86_64_SOURCES})
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(VITTE_ASM_ARCH_SOURCES ${VITTE_ASM_AARCH64_SOURCES})
  else()
    message(FATAL_ERROR "ASM fastpaths enabled but unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  endif()

  add_library(vitte_asm_fastpaths STATIC
    ${VITTE_ASM_C_SOURCES}
    ${VITTE_ASM_ARCH_SOURCES}
  )

  target_include_directories(vitte_asm_fastpaths
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${VITTE_ASM_ROOT}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
      ${VITTE_ASM_ROOT}/src/asm
  )

  if(APPLE)
    target_compile_definitions(vitte_asm_fastpaths PRIVATE VITTE_ASM_MACHO=1)
  elseif(NOT MSVC)
    target_compile_definitions(vitte_asm_fastpaths PRIVATE VITTE_ASM_ELF=1)
  endif()

  list(APPEND ALL_SOURCES src/bench/bench_asm_shim.c)
endif()

# ============================================================================
# Main executable: vittec-bench
# ============================================================================

add_executable(benchc ${ALL_SOURCES} ${BENCH_FRAMEWORK_HEADERS})
target_compile_definitions(benchc PRIVATE BENCH_RUNNER_STANDALONE BENCH_PLATFORM_NO_IMPL)

# Link math library if available
if(NOT WIN32)
  target_link_libraries(benchc PRIVATE m)
endif()

if(VITTE_ENABLE_RUST_API)
  target_compile_definitions(benchc PRIVATE VITTE_ENABLE_RUST_API=1)

  set(VITTE_RUST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../rust)

  if(NOT TARGET vitte_rust_api)
    find_program(CARGO_EXECUTABLE cargo REQUIRED)
    set(VITTE_RUST_MANIFEST ${VITTE_RUST_DIR}/Cargo.toml)

    # NOTE: Cargo output location depends on target triple; we rely on default host build here.
    set(VITTE_RUST_PROFILE release)
    set(VITTE_RUST_LIBDIR ${VITTE_RUST_DIR}/target/${VITTE_RUST_PROFILE})
    set(VITTE_RUST_STATICLIB ${VITTE_RUST_LIBDIR}/libvitte_rust_api.a)

    if(NOT TARGET vitte_rust_api_build)
      file(GLOB_RECURSE VITTE_RUST_WORKSPACE_SOURCES
        CONFIGURE_DEPENDS
        ${VITTE_RUST_DIR}/Cargo.toml
        ${VITTE_RUST_DIR}/Cargo.lock
        ${VITTE_RUST_DIR}/crates/*/Cargo.toml
        ${VITTE_RUST_DIR}/crates/*/src/*.rs
      )

      add_custom_command(
        OUTPUT ${VITTE_RUST_STATICLIB}
        COMMAND ${CARGO_EXECUTABLE} build -p vitte_rust_api --profile ${VITTE_RUST_PROFILE} --manifest-path ${VITTE_RUST_MANIFEST}
        BYPRODUCTS ${VITTE_RUST_STATICLIB}
        WORKING_DIRECTORY ${VITTE_RUST_DIR}
        DEPENDS ${VITTE_RUST_WORKSPACE_SOURCES}
        COMMENT "Building Rust staticlib: vitte_rust_api"
        VERBATIM
      )

      add_custom_target(vitte_rust_api_build DEPENDS ${VITTE_RUST_STATICLIB})
    endif()

    add_library(vitte_rust_api STATIC IMPORTED GLOBAL)
    set_target_properties(vitte_rust_api PROPERTIES IMPORTED_LOCATION ${VITTE_RUST_STATICLIB})
    add_dependencies(vitte_rust_api vitte_rust_api_build)
  endif()

  target_include_directories(benchc PRIVATE ${VITTE_RUST_DIR}/include-gen)
  target_link_libraries(benchc PRIVATE vitte_rust_api)

  if(APPLE)
    target_link_libraries(benchc PRIVATE "-framework CoreFoundation")
  endif()
endif()

# Platform-specific linking
if(APPLE)
  # macOS: link CoreFoundation for timing
  target_link_libraries(benchc PRIVATE "-framework CoreFoundation")
elseif(UNIX AND NOT APPLE)
  # Linux: link rt for clock_gettime
  target_link_libraries(benchc PRIVATE rt)
endif()

if(VITTE_ENABLE_ASM_FASTPATHS)
  target_link_libraries(benchc PRIVATE vitte_asm_fastpaths)
  target_compile_definitions(benchc PRIVATE
    VITTE_BENCH_USE_ASM_MEMCPY=1
    VITTE_BENCH_USE_ASM_HASH=1
  )
endif()

# ============================================================================
# Build properties
# ============================================================================

set_target_properties(benchc PROPERTIES
  C_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

# ============================================================================
# Output
# ============================================================================

message(STATUS "vitte-bench: Configuring ${CMAKE_BUILD_TYPE} build")
message(STATUS "  Framework: 16 .c files + 16 .h headers")
message(STATUS "  Micro-benchmarks: 15 .c files")
message(STATUS "  Macro-benchmarks: 8 .c files")
message(STATUS "  Total: 56 source/header files, 23 benchmarks")
message(STATUS "  Executable: benchc")
