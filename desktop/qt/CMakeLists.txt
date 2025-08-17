cmake_minimum_required(VERSION 3.16)
project(desktop-qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try Qt6 then Qt5
find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
  set(QT_PACKAGE Qt6)
else()
  find_package(Qt5 COMPONENTS Widgets REQUIRED)
  set(QT_PACKAGE Qt5)
endif()

add_executable(desktop-qt
  src/main.cpp
)

if(QT_PACKAGE STREQUAL "Qt6")
  target_link_libraries(desktop-qt PRIVATE Qt6::Widgets)
else()
  target_link_libraries(desktop-qt PRIVATE Qt5::Widgets)
endif()
