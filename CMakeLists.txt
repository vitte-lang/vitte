# C:\Users\vince\Documents\GitHub\vitte\CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# ------------------------------------------------------------------------------
# Vitte – C toolchain / compiler (C99) + optional Rust workspace build glue
#
# Conventions:
# - C sources are expected under: compiler/ (and optionally runtime/)
# - One canonical executable: vittec
# - Tests (optional): tests/
# - Bench (optional): bench/
#
# You can override paths:
#   -DVITTE_COMPILER_DIR=compiler
#   -DVITTE_RUNTIME_DIR=runtime
# ------------------------------------------------------------------------------

project(vitte
  VERSION 0.1.0
  LANGUAGES C
)

# --- options ------------------------------------------------------------------
option(VITTE_BUILD_TESTS "Build Vitte tests" ON)
option(VITTE_BUILD_BENCH "Build Vitte benchmarks" OFF)
option(VITTE_STRICT_WERROR "Treat warnings as errors" OFF)
option(VITTE_ENABLE_SANITIZERS "Enable ASan/UBSan on supported compilers" OFF)

set(VITTE_COMPILER_DIR "compiler" CACHE PATH "Path to Vitte compiler sources")
set(VITTE_RUNTIME_DIR  "runtime"  CACHE PATH "Path to Vitte runtime sources (optional)")

# --- C standard ----------------------------------------------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# --- default build type --------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# --- helpers -------------------------------------------------------------------
function(vitte_apply_common_c_flags target_name)
  target_compile_features(${target_name} PRIVATE c_std_99)

  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)
    if(VITTE_STRICT_WERROR)
      target_compile_options(${target_name} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall -Wextra -Wpedantic
      -Wshadow
      -Wconversion
      -Wstrict-prototypes
      -Wmissing-prototypes
      -Wno-unused-parameter
    )
    if(VITTE_STRICT_WERROR)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()

  if(VITTE_ENABLE_SANITIZERS AND NOT MSVC)
    # Best effort: clang/gcc
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  endif()
endfunction()

# --- sources discovery ----------------------------------------------------------
# Prefer explicit lists later; for now use glob with CONFIGURE_DEPENDS.
# Expected layout examples:
#   compiler/src/*.c
#   compiler/include/*.h
#   runtime/src/*.c (optional)
file(GLOB_RECURSE VITTE_COMPILER_C CONFIGURE_DEPENDS
  "${VITTE_COMPILER_DIR}/src/*.c"
  "${VITTE_COMPILER_DIR}/*.c"
)
file(GLOB_RECURSE VITTE_COMPILER_HEADERS CONFIGURE_DEPENDS
  "${VITTE_COMPILER_DIR}/include/*.h"
  "${VITTE_COMPILER_DIR}/src/*.h"
  "${VITTE_COMPILER_DIR}/*.h"
)

file(GLOB_RECURSE VITTE_RUNTIME_C CONFIGURE_DEPENDS
  "${VITTE_RUNTIME_DIR}/src/*.c"
  "${VITTE_RUNTIME_DIR}/*.c"
)

# --- include dirs --------------------------------------------------------------
set(VITTE_INCLUDE_DIRS
  "${VITTE_COMPILER_DIR}/include"
  "${VITTE_COMPILER_DIR}/src"
)

if(EXISTS "${VITTE_RUNTIME_DIR}/include")
  list(APPEND VITTE_INCLUDE_DIRS "${VITTE_RUNTIME_DIR}/include")
endif()
if(EXISTS "${VITTE_RUNTIME_DIR}/src")
  list(APPEND VITTE_INCLUDE_DIRS "${VITTE_RUNTIME_DIR}/src")
endif()

# --- core library (optional) ---------------------------------------------------
# Build compiler sources into a static library to reuse across tools/tests.
add_library(vitte_core STATIC
  ${VITTE_COMPILER_C}
  ${VITTE_COMPILER_HEADERS}
)

target_include_directories(vitte_core PUBLIC ${VITTE_INCLUDE_DIRS})
vitte_apply_common_c_flags(vitte_core)

# If a runtime exists, link it in
if(VITTE_RUNTIME_C)
  add_library(vitte_runtime STATIC ${VITTE_RUNTIME_C})
  target_include_directories(vitte_runtime PUBLIC ${VITTE_INCLUDE_DIRS})
  vitte_apply_common_c_flags(vitte_runtime)
  target_link_libraries(vitte_core PUBLIC vitte_runtime)
endif()

# --- vittec executable ----------------------------------------------------------
# If you have a dedicated main file, keep it in compiler/src/main.c or compiler/main.c
# Otherwise, set VITTE_MAIN explicitly.
set(VITTE_MAIN "" CACHE FILEPATH "Path to vittec main .c file (optional override)")
if(VITTE_MAIN)
  set(VITTE_VITTEC_MAIN "${VITTE_MAIN}")
else()
  if(EXISTS "${VITTE_COMPILER_DIR}/src/main.c")
    set(VITTE_VITTEC_MAIN "${VITTE_COMPILER_DIR}/src/main.c")
  elseif(EXISTS "${VITTE_COMPILER_DIR}/main.c")
    set(VITTE_VITTEC_MAIN "${VITTE_COMPILER_DIR}/main.c")
  else()
    # Fallback: try to find a file named vittec_main.c
    file(GLOB_RECURSE _VITTEC_MAIN_CAND CONFIGURE_DEPENDS
      "${VITTE_COMPILER_DIR}/src/*vittec*main*.c"
      "${VITTE_COMPILER_DIR}/*vittec*main*.c"
    )
    list(LENGTH _VITTEC_MAIN_CAND _len)
    if(_len GREATER 0)
      list(GET _VITTEC_MAIN_CAND 0 VITTE_VITTEC_MAIN)
    else()
      message(FATAL_ERROR
        "Cannot locate vittec main file. Provide -DVITTE_MAIN=path/to/main.c "
        "or add compiler/src/main.c.")
    endif()
  endif()
endif()

add_executable(vittec ${VITTE_VITTEC_MAIN})
target_link_libraries(vittec PRIVATE vitte_core)
target_include_directories(vittec PRIVATE ${VITTE_INCLUDE_DIRS})
vitte_apply_common_c_flags(vittec)

# On Windows, enable larger obj if needed (MSVC)
if(MSVC)
  target_compile_options(vittec PRIVATE /bigobj)
endif()

# --- install -------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS vittec
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install public headers if present
if(EXISTS "${VITTE_COMPILER_DIR}/include")
  install(DIRECTORY "${VITTE_COMPILER_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vitte
    FILES_MATCHING PATTERN "*.h"
  )
endif()

# --- tests ---------------------------------------------------------------------
if(VITTE_BUILD_TESTS)
  enable_testing()

  # Simple CTest discovery:
  # Put unit tests under tests/*.c (each can be built as an executable).
  file(GLOB_RECURSE VITTE_TEST_SOURCES CONFIGURE_DEPENDS "tests/*.c")
  foreach(test_src IN LISTS VITTE_TEST_SOURCES)
    get_filename_component(test_name "${test_src}" NAME_WE)
    add_executable("${test_name}" "${test_src}")
    target_link_libraries("${test_name}" PRIVATE vitte_core)
    target_include_directories("${test_name}" PRIVATE ${VITTE_INCLUDE_DIRS})
    vitte_apply_common_c_flags("${test_name}")
    add_test(NAME "${test_name}" COMMAND "${test_name}")
  endforeach()
endif()

# --- benchmarks ----------------------------------------------------------------
if(VITTE_BUILD_BENCH)
  file(GLOB_RECURSE VITTE_BENCH_SOURCES CONFIGURE_DEPENDS "bench/*.c" "bench/*/*.c")
  foreach(bench_src IN LISTS VITTE_BENCH_SOURCES)
    get_filename_component(bench_name "${bench_src}" NAME_WE)
    add_executable("${bench_name}" "${bench_src}")
    target_link_libraries("${bench_name}" PRIVATE vitte_core)
    target_include_directories("${bench_name}" PRIVATE ${VITTE_INCLUDE_DIRS})
    vitte_apply_common_c_flags("${bench_name}")
  endforeach()
endif()

# --- summary -------------------------------------------------------------------
message(STATUS "Vitte version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler dir: ${VITTE_COMPILER_DIR}")
message(STATUS "Runtime dir: ${VITTE_RUNTIME_DIR}")
message(STATUS "Compiler sources: ${VITTE_COMPILER_C}")
if(VITTE_RUNTIME_C)
  message(STATUS "Runtime sources: ${VITTE_RUNTIME_C}")
endif()
message(STATUS "Vitte main: ${VITTE_VITTEC_MAIN}")
