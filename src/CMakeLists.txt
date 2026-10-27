cmake_minimum_required(VERSION 3.16)

# Keep the CMake entry point on the same staged native/default executable as
# the repository Makefile. The Makefile owns the stage0 bootstrap build,
# generated native pipeline, and final link order.
find_program(VITTE_MAKE_EXECUTABLE NAMES gmake make)
if(NOT VITTE_MAKE_EXECUTABLE)
    message(FATAL_ERROR "Vitte requires make or gmake to build its compiler")
endif()

add_custom_target(vitte ALL
    COMMAND "${VITTE_MAKE_EXECUTABLE}"
        -C "${CMAKE_SOURCE_DIR}"
        "BUILD=${CMAKE_BINARY_DIR}"
        "CC=${CMAKE_C_COMPILER}"
        all
    BYPRODUCTS "${CMAKE_BINARY_DIR}/bin/vitte"
    COMMENT "Building the Vitte compiler with the native frontend pipeline"
    VERBATIM
)
