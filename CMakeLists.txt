cmake_minimum_required(VERSION 3.20)

project(vitte_bench LANGUAGES C)

# --------------------------
# Options (opt-in extensions)
# --------------------------
option(VITTE_BENCH_EXTRA "Enable extra (opt-in) benchmark cases in the registry" OFF)
option(VITTE_BENCH_EXPERIMENTAL "Enable experimental (opt-in) benchmark cases" OFF)

# If you have an asm dispatch layer providing these symbols, you can benchmark them:
#   - vitte_memcpy_fast
#   - vitte_fnv1a64_fast
option(VITTE_BENCH_USE_ASM_MEMCPY "Use vitte_memcpy_fast() in bm_memcpy" OFF)
option(VITTE_BENCH_USE_ASM_HASH "Use vitte_fnv1a64_fast() in bm_hash" OFF)

# Optional: if the core project builds a target named `vitte_core`, you can link it.
option(VITTE_BENCH_LINK_VITTE_CORE "Link benchc against a parent target vitte_core (if present)" OFF)

# --------------------------
# Build settings
# --------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Nice-to-have warnings (keep non-fatal)
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --------------------------
# bench library (core)
# --------------------------
add_library(benchlib STATIC
  src/bench/bench_time.c
  src/bench/bench_stats.c
  src/bench/bench_registry.c
)

target_include_directories(benchlib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Define opt-in macros consumed by the registry and benchmarks.
if(VITTE_BENCH_EXTRA)
  target_compile_definitions(benchlib PUBLIC VITTE_BENCH_EXTRA=1)
endif()
if(VITTE_BENCH_EXPERIMENTAL)
  target_compile_definitions(benchlib PUBLIC VITTE_BENCH_EXPERIMENTAL=1)
endif()
if(VITTE_BENCH_USE_ASM_MEMCPY)
  target_compile_definitions(benchlib PUBLIC VITTE_BENCH_USE_ASM_MEMCPY=1)
endif()
if(VITTE_BENCH_USE_ASM_HASH)
  target_compile_definitions(benchlib PUBLIC VITTE_BENCH_USE_ASM_HASH=1)
endif()

# --------------------------
# bench executable
# --------------------------
add_executable(benchc
  src/bench/bench_main.c
  src/micro/bm_add.c
  src/micro/bm_hash.c
  src/micro/bm_memcpy.c
  src/macro/bm_json_parse.c
)

target_link_libraries(benchc PRIVATE benchlib)

target_include_directories(benchc PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Some platforms used to require -lrt for clock_gettime; keep it conditional.
if(UNIX AND NOT APPLE)
  find_library(RT_LIB rt)
  if(RT_LIB)
    target_link_libraries(benchc PRIVATE ${RT_LIB})
  endif()
endif()

# Optional link to parent core if your top-level build provides it.
if(VITTE_BENCH_LINK_VITTE_CORE)
  if(TARGET vitte_core)
    target_link_libraries(benchc PRIVATE vitte_core)
  else()
    message(WARNING "VITTE_BENCH_LINK_VITTE_CORE=ON but target vitte_core not found")
  endif()
endif()

# --------------------------
# Convenience targets
# --------------------------
add_custom_target(run_bench_list
  COMMAND benchc --list-full
  DEPENDS benchc
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(run_bench_all
  COMMAND benchc --all
  DEPENDS benchc
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
