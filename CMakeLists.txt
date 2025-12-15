cmake_minimum_required(VERSION 3.10)
project(Steel VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

include_directories(${PROJECT_SOURCE_DIR}/include)

# Sources
file(GLOB COMPILER_SOURCES "src/compiler/*.c")
file(GLOB RUNTIME_SOURCES "src/runtime/*.c")
file(GLOB STDLIB_SOURCES "src/stdlib/*.c")
file(GLOB DEBUG_SOURCES "src/debug/*.c")

# Main executable
add_executable(steel src/main.c ${COMPILER_SOURCES} ${RUNTIME_SOURCES} ${STDLIB_SOURCES} ${DEBUG_SOURCES})

# Tests
enable_testing()
add_executable(test_suite tests/unit/test_main.c tests/unit/test_lexer.c tests/unit/test_vm.c)
add_test(NAME UnitTests COMMAND test_suite)

# Installation
install(TARGETS steel DESTINATION bin)
