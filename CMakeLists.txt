cmake_minimum_required(VERSION 3.16)

project(vittec_compiler VERSION 0.1.0 LANGUAGES C)

set(VITTEC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/compiler)

# C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(VITTEC_ENABLE_RUST_API "Build and link Rust FFI (vitte_rust_api)" OFF)
option(VITTEC_ENABLE_ENTERPRISE_LIB "Build enterprise_lib (portable fallbacks on non-x86)" OFF)
option(VITTEC_ENABLE_FRONT_EXPERIMENTAL "Build experimental front lexer/parser" OFF)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    set(VITTEC_ENTERPRISE_LIB_ARCH_X86 TRUE)
else()
    set(VITTEC_ENTERPRISE_LIB_ARCH_X86 FALSE)
endif()

# Compiler library sources
set(VITTEC_COMPILER_SOURCES
    ${VITTEC_ROOT}/src/compiler.c
    ${VITTEC_ROOT}/src/asm_optimizer.c
    ${VITTEC_ROOT}/src/ast.c
    ${VITTEC_ROOT}/src/parser.c
    ${VITTEC_ROOT}/src/lexer.c
    ${VITTEC_ROOT}/src/front/vittec_lexer.c
    ${VITTEC_ROOT}/src/types.c
    ${VITTEC_ROOT}/src/hir.c
    ${VITTEC_ROOT}/src/ir.c
    ${VITTEC_ROOT}/src/sema.c
    ${VITTEC_ROOT}/src/symbol_table.c
    ${VITTEC_ROOT}/src/diagnostic.c
    ${VITTEC_ROOT}/src/backend_c.c
    ${VITTEC_ROOT}/src/backend.c
    ${VITTEC_ROOT}/src/frontend.c
    ${VITTEC_ROOT}/src/codegen.c
    ${VITTEC_ROOT}/src/inline_asm.c
    ${VITTEC_ROOT}/src/lowering.c
    ${VITTEC_ROOT}/src/native_codegen.c
    ${VITTEC_ROOT}/src/optimizer.c
    ${VITTEC_ROOT}/src/target.c
    ${VITTEC_ROOT}/src/driver.c
    ${VITTEC_ROOT}/src/back/emit_c.c
    ${VITTEC_ROOT}/src/diag/diagnostic.c
    ${VITTEC_ROOT}/src/diag/emitter.c
    ${VITTEC_ROOT}/src/diag/emitter_human.c
    ${VITTEC_ROOT}/src/diag/emitter_json.c
    ${VITTEC_ROOT}/src/diag/source_map.c
    ${VITTEC_ROOT}/src/support/arena.c
    ${VITTEC_ROOT}/src/support/fs.c
    ${VITTEC_ROOT}/src/support/log.c
    ${VITTEC_ROOT}/src/support/str.c
    ${VITTEC_ROOT}/src/support/vec.c
    ${VITTEC_ROOT}/src/vitte_stub.c
)

# Rust-dependent sources (must be in the target sources list to satisfy link symbols).
if(VITTEC_ENABLE_RUST_API)
    list(APPEND VITTEC_COMPILER_SOURCES ${VITTEC_ROOT}/src/front/muf_rust.c)
endif()

if(VITTEC_ENABLE_ENTERPRISE_LIB)
    list(APPEND VITTEC_COMPILER_SOURCES ${VITTEC_ROOT}/src/enterprise_lib.c)
endif()

if(VITTEC_ENABLE_FRONT_EXPERIMENTAL)
    list(APPEND VITTEC_COMPILER_SOURCES
        ${VITTEC_ROOT}/src/front/lexer.c
        ${VITTEC_ROOT}/src/front/parser.c
    )
endif()

# Create compiler library
add_library(vittec_compiler STATIC ${VITTEC_COMPILER_SOURCES})

target_include_directories(vittec_compiler PUBLIC
    $<BUILD_INTERFACE:${VITTEC_ROOT}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(VITTEC_ENABLE_ENTERPRISE_LIB)
    if(VITTEC_ENTERPRISE_LIB_ARCH_X86)
        target_compile_definitions(vittec_compiler PUBLIC ENTERPRISE_LIB_X86=1)
    else()
        target_compile_definitions(vittec_compiler PUBLIC ENTERPRISE_PORTABLE=1)
        message(STATUS "enterprise_lib: using portable implementation for ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

if(DEFINED VITTE_INCLUDE_MAX_PATHS AND VITTE_INCLUDE_MAX_PATHS AND VITTE_MAX_INCLUDE_DIRS)
    target_include_directories(vittec_compiler PRIVATE ${VITTE_MAX_INCLUDE_DIRS})
endif()

if(VITTEC_ENABLE_RUST_API)
    target_compile_definitions(vittec_compiler PUBLIC VITTEC_ENABLE_RUST_API=1)

    find_program(CARGO_EXECUTABLE cargo REQUIRED)
    set(VITTE_RUST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rust)
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
add_executable(test_lexer ${VITTEC_ROOT}/tests/unit/test_lexer.c)
target_link_libraries(test_lexer PRIVATE vittec_compiler)
target_include_directories(test_lexer PRIVATE ${VITTEC_ROOT}/include)
add_test(NAME test_lexer COMMAND test_lexer)

# Test: AST
add_executable(test_ast ${VITTEC_ROOT}/tests/unit/test_ast.c)
target_link_libraries(test_ast PRIVATE vittec_compiler)
target_include_directories(test_ast PRIVATE ${VITTEC_ROOT}/include)
add_test(NAME test_ast COMMAND test_ast)

# Test: Types
add_executable(test_types ${VITTEC_ROOT}/tests/unit/test_types.c)
target_link_libraries(test_types PRIVATE vittec_compiler)
target_include_directories(test_types PRIVATE ${VITTEC_ROOT}/include)
add_test(NAME test_types COMMAND test_types)

# Test: Symbol Table
add_executable(test_symbol_table ${VITTEC_ROOT}/tests/unit/test_symbol_table.c)
target_link_libraries(test_symbol_table PRIVATE vittec_compiler)
target_include_directories(test_symbol_table PRIVATE ${VITTEC_ROOT}/include)
add_test(NAME test_symbol_table COMMAND test_symbol_table)

# Test: Diagnostics & Source Map
add_executable(test_diag ${VITTEC_ROOT}/tests/unit/test_diag.c)
target_link_libraries(test_diag PRIVATE vittec_compiler)
target_include_directories(test_diag PRIVATE ${VITTEC_ROOT}/include)
add_test(NAME test_diag COMMAND test_diag)

if(VITTEC_ENABLE_RUST_API)
  add_executable(test_muf_rust ${VITTEC_ROOT}/tests/unit/test_muf_rust.c)
  target_link_libraries(test_muf_rust PRIVATE vittec_compiler)
  target_include_directories(test_muf_rust PRIVATE ${VITTEC_ROOT}/include)
  add_test(NAME test_muf_rust COMMAND test_muf_rust)

  add_executable(test_rust_abi_smoke ${VITTEC_ROOT}/tests/unit/test_rust_abi_smoke.c)
  target_link_libraries(test_rust_abi_smoke PRIVATE vittec_compiler)
  target_include_directories(test_rust_abi_smoke PRIVATE ${VITTEC_ROOT}/include)
  add_test(NAME test_rust_abi_smoke COMMAND test_rust_abi_smoke)
endif()

# Main executable
add_executable(vittec
    ${VITTEC_ROOT}/src/main.c
    ${VITTEC_ROOT}/src/vittec.c
    ${VITTEC_ROOT}/src/version.c
)
target_link_libraries(vittec PRIVATE vittec_compiler)
target_include_directories(vittec PRIVATE
    ${VITTEC_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(DEFINED VITTE_INCLUDE_MAX_PATHS AND VITTE_INCLUDE_MAX_PATHS AND VITTE_MAX_INCLUDE_DIRS)
    target_include_directories(vittec PRIVATE ${VITTE_MAX_INCLUDE_DIRS})
endif()

set(VITTEC_TOKEN_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/cli_tokens/sample_program.vitte)
set(VITTEC_TOKEN_TEST_EXPECTED ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/cli_tokens/expected_tokens.txt)

set(VITTEC_TOKEN_TEST_RUNNER "")
if(WIN32)
    find_program(PYTHON_EXECUTABLE NAMES python3 python REQUIRED)
    set(VITTEC_TOKEN_TEST_RUNNER ${PYTHON_EXECUTABLE})
    set(VITTEC_TOKEN_TEST_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/cli_tokens/run_cli_tokens.py)
else()
    set(VITTEC_TOKEN_TEST_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/cli_tokens/run_cli_tokens.sh)
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
install(DIRECTORY ${VITTEC_ROOT}/include/ DESTINATION include)
