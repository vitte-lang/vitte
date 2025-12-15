cmake_minimum_required(VERSION 3.20)
project(vitte_bench_c LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(benchc
  src/bench/bench_main.c
  src/bench/bench_registry.c
  src/bench/bench_stats.c
  src/bench/bench_time.c

  src/micro/bm_add.c
  src/micro/bm_hash.c
  src/micro/bm_memcpy.c

  src/macro/bm_json_parse.c
)

target_include_directories(benchc PRIVATE src)

if (MSVC)
  target_compile_options(benchc PRIVATE /W4)
else()
  target_compile_options(benchc PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()
