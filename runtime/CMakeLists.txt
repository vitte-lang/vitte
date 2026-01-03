cmake_minimum_required(VERSION 3.16)
project(vitte-runtime C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/abi)

# Source files
set(RUNTIME_SOURCES
    src/vitte_types.c
    src/vitte_vm.c
    src/builtin.c
    src/main.c
    gc/garbage_collector.c
    abi/calling_convention.c
)

# Create runtime executable
add_executable(vitte-runtime ${RUNTIME_SOURCES})

# Link readline library if available
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(READLINE readline)
    if(READLINE_FOUND)
        target_include_directories(vitte-runtime PRIVATE ${READLINE_INCLUDE_DIRS})
        target_link_libraries(vitte-runtime ${READLINE_LIBRARIES})
    else()
        target_link_libraries(vitte-runtime readline)
    endif()
else()
    target_link_libraries(vitte-runtime readline)
endif()

# Link math library
target_link_libraries(vitte-runtime m)

# Set output directory
set_target_properties(vitte-runtime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build"
)

# Installation
install(TARGETS vitte-runtime DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/vitte)

# Testing
enable_testing()

add_test(
    NAME vitte-runtime-tests
    COMMAND vitte-runtime --test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom targets
add_custom_target(runtime-clean
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake_clean.cmake
)
