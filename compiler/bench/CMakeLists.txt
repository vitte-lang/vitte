cmake_minimum_required(VERSION 3.16)
project(vitte_bench_compat LANGUAGES C)

# Compatibility entrypoint:
# Some IDEs / scripts may be configured to use `compiler/bench` as the CMake
# source directory. The benchmarks live at the repository root in `bench/`.
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../bench" bench)
