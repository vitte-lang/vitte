cmake_minimum_required(VERSION 3.20)

project(vitte LANGUAGES C)

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
# Vitte core library + tests
# --------------------------
add_library(vitte_core STATIC
  src/vitte/ast.c
  src/vitte/lexer.c
  src/vitte/parser_phrase.c
  src/vitte/desugar_phrase.c
  src/vitte/codegen.c
)

target_include_directories(vitte_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

add_executable(vitte_t_lexer tests/unit/vitte/t_lexer.c)
target_link_libraries(vitte_t_lexer PRIVATE vitte_core)

add_executable(vitte_t_desugar tests/unit/vitte/t_desugar_phrase.c)
target_link_libraries(vitte_t_desugar PRIVATE vitte_core)

add_executable(vitte_t_codegen tests/unit/vitte/t_codegen.c)
target_link_libraries(vitte_t_codegen PRIVATE vitte_core)

add_executable(vitte_t_parser tests/unit/vitte/t_parser_phrase.c)
target_link_libraries(vitte_t_parser PRIVATE vitte_core)

add_executable(vitte_t_emit tests/unit/vitte/t_emit_c_backend.c compiler/src/back/emit_c.c)
target_link_libraries(vitte_t_emit PRIVATE vitte_core)
target_include_directories(vitte_t_emit PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/compiler/include
)

add_test(NAME vitte_t_lexer COMMAND vitte_t_lexer)
add_test(NAME vitte_t_desugar COMMAND vitte_t_desugar)
add_test(NAME vitte_t_codegen COMMAND vitte_t_codegen)
add_test(NAME vitte_t_parser COMMAND vitte_t_parser)
add_test(NAME vitte_t_emit COMMAND vitte_t_emit)

add_custom_target(vitte_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS vitte_t_lexer vitte_t_desugar vitte_t_codegen vitte_t_parser vitte_t_emit
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# --------------------------
# bench library (core)
# --------------------------
add_library(benchlib STATIC
  bench/src/bench/bench_time.c
  bench/src/bench/bench_stats.c
  bench/src/bench/bench_registry.c
)

target_include_directories(benchlib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/bench/src
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
  bench/src/bench/bench_main.c
  bench/src/micro/bm_add.c
  bench/src/micro/bm_hash.c
  bench/src/micro/bm_memcpy.c
  bench/src/macro/bm_json_parse.c
)

target_link_libraries(benchc PRIVATE benchlib)

target_include_directories(benchc PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/bench/src
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
