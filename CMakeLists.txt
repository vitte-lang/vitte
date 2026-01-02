cmake_minimum_required(VERSION 3.16)

project(vitte_compiler LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(VITTE_ENABLE_WARNINGS "Enable common warning flags" ON)

# -----------------------------------------------------------------------------
# Library: vitte_compiler (all compiler sources under compiler/src/compiler/lib)
# -----------------------------------------------------------------------------

set(VITTE_COMPILER_LIB_SOURCES
  compiler/src/compiler/lib/diag/diag.c
  compiler/src/compiler/lib/diag/diag_catalog.c
  compiler/src/compiler/lib/diag/diag_codes.c
  compiler/src/compiler/lib/diag/diag_levels.c
  compiler/src/compiler/lib/diag/diag_span.c
  compiler/src/compiler/lib/diag/render/diag_render_ansi.c
  compiler/src/compiler/lib/diag/render/diag_render_jsonl.c
  compiler/src/compiler/lib/diag/sinks/diag_sink_mem.c
  compiler/src/compiler/lib/diag/sinks/diag_sink_stdio.c
  compiler/src/compiler/lib/diag/normalize/diag_normalize.c
)

add_library(vitte_compiler STATIC ${VITTE_COMPILER_LIB_SOURCES})

target_include_directories(vitte_compiler
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler/src/compiler/include"
)

if(VITTE_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(vitte_compiler PRIVATE /W4)
  else()
    target_compile_options(vitte_compiler PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

# -----------------------------------------------------------------------------
# Executables (stub frontends)
# -----------------------------------------------------------------------------

set(VITTE_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/compiler/src/compiler/bin")

add_executable(vittec "${VITTE_BIN_DIR}/vittec_main.c")
target_link_libraries(vittec PRIVATE vitte_compiler)

add_executable(vittefmt "${VITTE_BIN_DIR}/vittefmt_main.c")
target_link_libraries(vittefmt PRIVATE vitte_compiler)

add_executable(vittepkg "${VITTE_BIN_DIR}/pkgtool_main.c")
target_link_libraries(vittepkg PRIVATE vitte_compiler)

add_executable(vitte_lsp "${VITTE_BIN_DIR}/lsp_main.c")
target_link_libraries(vitte_lsp PRIVATE vitte_compiler)

# -----------------------------------------------------------------------------
# Convenience output
# -----------------------------------------------------------------------------

message(STATUS "vitte_compiler sources: ${VITTE_COMPILER_LIB_SOURCES}")
message(STATUS "Binaries: vittec, vittefmt, vittepkg, vitte_lsp (stub implementations)")
