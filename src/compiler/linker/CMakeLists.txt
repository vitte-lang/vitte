cmake_minimum_required(VERSION 3.16)
project(vitte_linker LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(vitte_linker
  linker.cpp
  objects.cpp
  archive.cpp
)

target_include_directories(vitte_linker PUBLIC .)