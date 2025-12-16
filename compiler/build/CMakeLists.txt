cmake_minimum_required(VERSION 3.10)

# Create a target for the compiler library
add_library(vittec_compiler
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
    src/codegen.c
    src/lowering.c
    src/optimizer.c
    src/target.c
    src/driver.c
)

target_include_directories(vittec_compiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(vittec_compiler PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    PUBLIC_HEADER "include/compiler.h"
)

# Optional: Install targets
install(TARGETS vittec_compiler
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)
