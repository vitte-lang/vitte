cmake_minimum_required(VERSION 3.16)
project(vitte_driver LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(vitte_driver
  driver.cpp
  options.cpp
  pipeline.cpp
)

target_include_directories(vitte_driver PUBLIC .)