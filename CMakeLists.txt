cmake_minimum_required(VERSION 3.20)

project(vitte VERSION 0.1.0 LANGUAGES C)

option(VITTE_BUILD_BENCH "Build vitte benchmarks (benchc)" ON)
option(VITTE_BUILD_COMPILER "Build vitte C compiler (vittec)" ON)
option(VITTE_ENABLE_RUST_API "Build and link Rust FFI (vitte_rust_api)" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CTest)

add_library(vitte_lib STATIC
  src/vitte/ast.c
  src/vitte/lexer.c
  src/vitte/parser_phrase.c
  src/vitte/lint_phrase.c
  src/vitte/desugar_phrase.c
  src/vitte/codegen.c
  src/vitte/diag.c
)

target_include_directories(vitte_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(MSVC)
  target_compile_options(vitte_lib PRIVATE /W4)
else()
  target_compile_options(vitte_lib PRIVATE -Wall -Wextra -Werror)
endif()

if(VITTE_BUILD_BENCH)
  add_subdirectory(bench)
endif()

if(VITTE_BUILD_COMPILER)
  set(VITTEC_ENABLE_RUST_API ${VITTE_ENABLE_RUST_API} CACHE BOOL "Enable Rust FFI in vittec" FORCE)
  add_subdirectory(compiler)
endif()

if(BUILD_TESTING)
  add_executable(test_vitte_lexer tests/unit/vitte/t_lexer.c)
  target_link_libraries(test_vitte_lexer PRIVATE vitte_lib)
  add_test(NAME test_vitte_lexer COMMAND test_vitte_lexer)

  add_executable(test_vitte_parser_phrase tests/unit/vitte/t_parser_phrase.c)
  target_link_libraries(test_vitte_parser_phrase PRIVATE vitte_lib)
  add_test(NAME test_vitte_parser_phrase COMMAND test_vitte_parser_phrase)

  add_executable(test_vitte_desugar_phrase tests/unit/vitte/t_desugar_phrase.c)
  target_link_libraries(test_vitte_desugar_phrase PRIVATE vitte_lib)
  add_test(NAME test_vitte_desugar_phrase COMMAND test_vitte_desugar_phrase)

  add_executable(test_vitte_codegen tests/unit/vitte/t_codegen.c)
  target_link_libraries(test_vitte_codegen PRIVATE vitte_lib)
  add_test(NAME test_vitte_codegen COMMAND test_vitte_codegen)

  add_executable(test_vitte_diag_golden tests/unit/vitte/t_diag_golden.c)
  target_link_libraries(test_vitte_diag_golden PRIVATE vitte_lib)
  target_compile_definitions(test_vitte_diag_golden PRIVATE VITTE_TEST_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}\")
  add_test(NAME test_vitte_diag_golden COMMAND test_vitte_diag_golden)

  add_custom_target(vitte_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    USES_TERMINAL
  )
endif()
