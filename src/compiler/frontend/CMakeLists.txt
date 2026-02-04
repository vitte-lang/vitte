cmake_minimum_required(VERSION 3.16)
project(vitte_frontend LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(vitte_frontend
  lexer.cpp
  parser.cpp
  ast.cpp
  diagnostics.cpp
)

target_include_directories(vitte_frontend PUBLIC .)