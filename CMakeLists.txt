cmake_minimum_required(VERSION 3.16)

project(vittec_compiler VERSION 0.1.0 LANGUAGES C)

# C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(VITTEC_ENABLE_RUST_API "Build and link Rust FFI (vitte_rust_api)" OFF)

# Compiler library sources
set(VITTEC_COMPILER_SOURCES
    src/compiler.c
    src/ast.c
    src/parser.c
    src/lexer.c
    src/front/vittec_lexer.c
    src/types.c
    src/hir.c
    src/ir.c
    src/sema.c
    src/symbol_table.c
    src/diagnostic.c
    src/backend_c.c
    src/backend.c
    src/frontend.c
    src/codegen.c
    src/lowering.c
    src/optimizer.c
    src/target.c
    src/driver.c
    src/back/emit_c.c
    src/diag/diagnostic.c
    src/diag/emitter.c
    src/diag/source_map.c
    src/support/arena.c
    src/support/fs.c
    src/support/log.c
    src/support/str.c
    src/support/vec.c
    src/vitte_stub.c
)

# Rust-dependent sources (must be in the target sources list to satisfy link symbols).
if(VITTEC_ENABLE_RUST_API)
    list(APPEND VITTEC_COMPILER_SOURCES src/front/muf_rust.c)
endif()

# Create compiler library
add_library(vittec_compiler STATIC ${VITTEC_COMPILER_SOURCES})

target_include_directories(vittec_compiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

if(DEFINED VITTE_INCLUDE_MAX_PATHS AND VITTE_INCLUDE_MAX_PATHS AND VITTE_MAX_INCLUDE_DIRS)
    target_include_directories(vittec_compiler PRIVATE ${VITTE_MAX_INCLUDE_DIRS})
endif()

if(VITTEC_ENABLE_RUST_API)
    target_compile_definitions(vittec_compiler PUBLIC VITTEC_ENABLE_RUST_API=1)

    find_program(CARGO_EXECUTABLE cargo REQUIRED)
    set(VITTE_RUST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../rust)
    set(VITTE_RUST_MANIFEST ${VITTE_RUST_DIR}/Cargo.toml)

    # NOTE: Cargo output location depends on target triple; we rely on default host build here.
    set(VITTE_RUST_PROFILE release)
    set(VITTE_RUST_LIBDIR ${VITTE_RUST_DIR}/target/${VITTE_RUST_PROFILE})
    if(WIN32)
        set(VITTE_RUST_STATICLIB ${VITTE_RUST_LIBDIR}/vitte_rust_api.lib)
    else()
        set(VITTE_RUST_STATICLIB ${VITTE_RUST_LIBDIR}/libvitte_rust_api.a)
    endif()

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
        WORKING_DIRECTORY ${VITTE_RUST_DIR}
        DEPENDS ${VITTE_RUST_WORKSPACE_SOURCES}
        COMMENT "Building Rust staticlib: vitte_rust_api"
        VERBATIM
        )

        add_custom_target(vitte_rust_api_build DEPENDS ${VITTE_RUST_STATICLIB})
    endif()

    if(NOT TARGET vitte_rust_api)
        add_library(vitte_rust_api STATIC IMPORTED GLOBAL)
        set_target_properties(vitte_rust_api PROPERTIES IMPORTED_LOCATION ${VITTE_RUST_STATICLIB})
        add_dependencies(vitte_rust_api vitte_rust_api_build)
    endif()

    target_include_directories(vittec_compiler PUBLIC ${VITTE_RUST_DIR}/include-gen)
    target_link_libraries(vittec_compiler PUBLIC vitte_rust_api)
    if(WIN32)
        target_link_libraries(vittec_compiler PUBLIC ntdll)
    endif()

    if(APPLE)
      target_link_libraries(vittec_compiler PUBLIC "-framework CoreFoundation")
    endif()
endif()

# Compiler flags
if(MSVC)
    target_compile_options(vittec_compiler PRIVATE /W4 /WX)
else()
target_compile_options(vittec_compiler PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(vittec_compiler PRIVATE _CRT_SECURE_NO_WARNINGS)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_options(vittec_compiler PRIVATE -g -O0)
    endif()
endif()

# Unit tests
enable_testing()

# Test: Lexer
add_executable(test_lexer tests/unit/test_lexer.c)
target_link_libraries(test_lexer PRIVATE vittec_compiler)
target_include_directories(test_lexer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_lexer COMMAND test_lexer)

# Test: AST
add_executable(test_ast tests/unit/test_ast.c)
target_link_libraries(test_ast PRIVATE vittec_compiler)
target_include_directories(test_ast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_ast COMMAND test_ast)

# Test: Types
add_executable(test_types tests/unit/test_types.c)
target_link_libraries(test_types PRIVATE vittec_compiler)
target_include_directories(test_types PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_types COMMAND test_types)

# Test: Symbol Table
add_executable(test_symbol_table tests/unit/test_symbol_table.c)
target_link_libraries(test_symbol_table PRIVATE vittec_compiler)
target_include_directories(test_symbol_table PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_symbol_table COMMAND test_symbol_table)

# Test: Diagnostics & Source Map
add_executable(test_diag tests/unit/test_diag.c)
target_link_libraries(test_diag PRIVATE vittec_compiler)
target_include_directories(test_diag PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_diag COMMAND test_diag)

if(VITTEC_ENABLE_RUST_API)
  add_executable(test_muf_rust tests/unit/test_muf_rust.c)
  target_link_libraries(test_muf_rust PRIVATE vittec_compiler)
  target_include_directories(test_muf_rust PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME test_muf_rust COMMAND test_muf_rust)

  add_executable(test_rust_abi_smoke tests/unit/test_rust_abi_smoke.c)
  target_link_libraries(test_rust_abi_smoke PRIVATE vittec_compiler)
  target_include_directories(test_rust_abi_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME test_rust_abi_smoke COMMAND test_rust_abi_smoke)
endif()

# Main executable
add_executable(vittec src/main.c src/vittec.c src/version.c)
target_link_libraries(vittec PRIVATE vittec_compiler)
target_include_directories(vittec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

if(DEFINED VITTE_INCLUDE_MAX_PATHS AND VITTE_INCLUDE_MAX_PATHS AND VITTE_MAX_INCLUDE_DIRS)
    target_include_directories(vittec PRIVATE ${VITTE_MAX_INCLUDE_DIRS})
endif()

set(VITTEC_TOKEN_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../tests/integration/cli_tokens/sample_program.vitte)
set(VITTEC_TOKEN_TEST_EXPECTED ${CMAKE_CURRENT_SOURCE_DIR}/../tests/integration/cli_tokens/expected_tokens.txt)

set(VITTEC_TOKEN_TEST_RUNNER "")
if(WIN32)
    find_program(PYTHON_EXECUTABLE NAMES python3 python REQUIRED)
    set(VITTEC_TOKEN_TEST_RUNNER ${PYTHON_EXECUTABLE})
    set(VITTEC_TOKEN_TEST_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../tests/integration/cli_tokens/run_cli_tokens.py)
else()
    set(VITTEC_TOKEN_TEST_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../tests/integration/cli_tokens/run_cli_tokens.sh)
endif()

if(VITTEC_TOKEN_TEST_RUNNER)
    add_test(
        NAME integration_cli_tokens
        COMMAND ${VITTEC_TOKEN_TEST_RUNNER}
                ${VITTEC_TOKEN_TEST_SCRIPT}
                $<TARGET_FILE:vittec>
                ${VITTEC_TOKEN_TEST_SRC}
                ${VITTEC_TOKEN_TEST_EXPECTED}
    )
else()
    add_test(
        NAME integration_cli_tokens
        COMMAND ${VITTEC_TOKEN_TEST_SCRIPT}
                $<TARGET_FILE:vittec>
                ${VITTEC_TOKEN_TEST_SRC}
                ${VITTEC_TOKEN_TEST_EXPECTED}
    )
endif()

# Installation
install(TARGETS vittec RUNTIME DESTINATION bin)
install(TARGETS vittec_compiler ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    USES_TERMINAL
  )
endif()
