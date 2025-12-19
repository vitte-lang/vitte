cmake_minimum_required(VERSION 3.20)

project(vitte VERSION 0.1.0 LANGUAGES C)

option(VITTE_BUILD_BENCH "Build vitte benchmarks (benchc)" ON)
option(VITTE_BUILD_COMPILER "Build vitte C compiler (vittec)" ON)
option(VITTE_ENABLE_RUST_API "Build and link Rust FFI (vitte_rust_api)" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CTest)

if(VITTE_BUILD_BENCH)
  add_subdirectory(bench)
endif()

if(VITTE_BUILD_COMPILER)
  set(VITTEC_ENABLE_RUST_API ${VITTE_ENABLE_RUST_API} CACHE BOOL "Enable Rust FFI in vittec" FORCE)
  add_subdirectory(compiler)
endif()

if(BUILD_TESTING)
  add_custom_target(vitte_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    USES_TERMINAL
  )
endif()
