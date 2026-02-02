cmake_minimum_required(VERSION 3.22)
project(vitte_llvm LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)

add_library(vitte_llvm STATIC
    src/context.cpp
    src/lower/from_ssa.cpp
    src/opt/passes.cpp
    src/emit/object.cpp
    src/link/lld.cpp
)

target_include_directories(vitte_llvm PUBLIC include)
target_link_libraries(vitte_llvm PUBLIC LLVM)
target_compile_features(vitte_llvm PUBLIC cxx_std_20)
