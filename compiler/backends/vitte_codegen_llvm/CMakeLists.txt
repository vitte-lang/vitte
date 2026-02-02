cmake_minimum_required(VERSION 3.22)
project(vitte_codegen_llvm LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)

add_library(vitte_codegen_llvm STATIC
    src/context.cpp
    src/lower/module.cpp
    src/lower/function.cpp
    src/lower/expr.cpp
    src/emit/emit.cpp
    src/passes/passes.cpp
)

target_include_directories(vitte_codegen_llvm PUBLIC include)
target_link_libraries(vitte_codegen_llvm PUBLIC LLVM)
target_compile_features(vitte_codegen_llvm PUBLIC cxx_std_20)
