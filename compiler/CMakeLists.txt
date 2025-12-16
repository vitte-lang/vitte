cmake_minimum_required(VERSION 3.16)

project(vittec_compiler VERSION 0.1.0 LANGUAGES C)

# C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler library sources
set(VITTEC_COMPILER_SOURCES
    src/compiler.c
    src/ast.c
    src/parser.c
    src/lexer.c
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
)

# Create compiler library
add_library(vittec_compiler STATIC ${VITTEC_COMPILER_SOURCES})

target_include_directories(vittec_compiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler flags
if(MSVC)
    target_compile_options(vittec_compiler PRIVATE /W4 /WX)
else()
    target_compile_options(vittec_compiler PRIVATE -Wall -Wextra -Werror)
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

# Main executable
add_executable(vittec src/main.c src/vittec.c src/version.c)
target_link_libraries(vittec PRIVATE vittec_compiler)
target_include_directories(vittec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Installation
install(TARGETS vittec RUNTIME DESTINATION bin)
install(TARGETS vittec_compiler ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
