cmake_minimum_required(VERSION 3.20)
project(vitte-bench C)

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

# ============================================================================
# Framework source files (bench/)
# ============================================================================

set(BENCH_FRAMEWORK_SOURCES
  src/bench/bench_main.c
  src/bench/bench_registry.c
  src/bench/bench_stats.c
  src/bench/bench_time.c
  src/bench/benchmark_init.c
  src/bench/runner.c
  src/bench/options.c
  src/bench/output.c
  src/bench/timing.c
  src/bench/sample.c
  src/bench/json_parser.c
  src/bench/alloc.c
  src/bench/strutil.c
  src/bench/format.c
  src/bench/csv.c
  src/bench/log.c
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
  src/micro/bm_hash.c
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

# ============================================================================
# Main executable: vittec-bench
# ============================================================================

add_executable(vittec-bench ${ALL_SOURCES} ${BENCH_FRAMEWORK_HEADERS})

# Link math library if available
if(NOT WIN32)
  target_link_libraries(vittec-bench m)
endif()

# Platform-specific linking
if(APPLE)
  # macOS: link CoreFoundation for timing
  target_link_libraries(vittec-bench "-framework CoreFoundation")
elseif(UNIX AND NOT APPLE)
  # Linux: link rt for clock_gettime
  target_link_libraries(vittec-bench rt)
endif()

# ============================================================================
# Build properties
# ============================================================================

set_target_properties(vittec-bench PROPERTIES
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
message(STATUS "  Executable: vittec-bench")
