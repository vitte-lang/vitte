cmake_minimum_required(VERSION 3.16)

project(vitte_backends LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Backend C++ sources
# --------------------------------------------------

set(VITTE_BACKEND_CPP_SOURCES
    cpp_backend.cpp

    # AST
    ast/cpp_type.hpp
    ast/cpp_expr.hpp
    ast/cpp_stmt.hpp
    ast/cpp_decl.hpp

    # Context
    context/cpp_context.cpp
    context/cpp_context.hpp

    # Lowering
    lower/lower_mir.cpp
    lower/lower_mir.hpp

    # Emit
    emit/emit.hpp
    emit/emit_expr.cpp
    emit/emit_stmt.cpp
    emit/emit_decl.cpp
    emit/emit_file.cpp

    # Runtime
    runtime/vitte_runtime.cpp
    runtime/vitte_runtime.hpp

    # Toolchain
    toolchain/clang.cpp
    toolchain/clang.hpp
)

# --------------------------------------------------
# Backend library
# --------------------------------------------------

add_library(vitte_backend_cpp STATIC
    ${VITTE_BACKEND_CPP_SOURCES}
)

target_include_directories(vitte_backend_cpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# --------------------------------------------------
# Warnings / options
# --------------------------------------------------

if (MSVC)
    target_compile_options(vitte_backend_cpp PRIVATE /W4)
else()
    target_compile_options(vitte_backend_cpp PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# --------------------------------------------------
# Platform specifics
# --------------------------------------------------

if (UNIX AND NOT APPLE)
    target_link_libraries(vitte_backend_cpp PRIVATE stdc++)
endif()

# --------------------------------------------------
# Export target
# --------------------------------------------------

set_target_properties(vitte_backend_cpp PROPERTIES
    OUTPUT_NAME "vitte_backend_cpp"
)
