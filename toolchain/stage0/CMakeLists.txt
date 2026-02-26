cmake_minimum_required(VERSION 3.20)

project(vittec0 LANGUAGES CXX)

option(VITTEC0_WARNINGS_AS_ERRORS "Treat vittec0 warnings as errors" ON)

add_executable(vittec0 src/main.cpp)
target_compile_features(vittec0 PRIVATE cxx_std_20)
set_target_properties(vittec0 PROPERTIES
  CXX_EXTENSIONS OFF
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(vittec0 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
  if(VITTEC0_WARNINGS_AS_ERRORS)
    target_compile_options(vittec0 PRIVATE -Werror)
  endif()
elseif(MSVC)
  target_compile_options(vittec0 PRIVATE /W4)
  if(VITTEC0_WARNINGS_AS_ERRORS)
    target_compile_options(vittec0 PRIVATE /WX)
  endif()
endif()

include(GNUInstallDirs)
install(TARGETS vittec0
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
