# Helper to consume Vitte SDK from CMake
# Usage:
#   set(VITTE_SDK "/path/to/vitte/sdk")
#   include(${VITTE_SDK}/sysroot/share/vitte/cmake/vitte_sdk.cmake)

if(NOT DEFINED VITTE_SDK)
  message(FATAL_ERROR "VITTE_SDK not set")
endif()

set(VITTE_SDK_INCLUDE_DIR "${VITTE_SDK}/sysroot/include")
set(VITTE_SDK_LIB_DIR "${VITTE_SDK}/sysroot/lib/vitte")

add_library(vitte_runtime UNKNOWN IMPORTED)
set_target_properties(vitte_runtime PROPERTIES
  IMPORTED_LOCATION "${VITTE_SDK_LIB_DIR}/libvitte_runtime.a"
  INTERFACE_INCLUDE_DIRECTORIES "${VITTE_SDK_INCLUDE_DIR}"
)
