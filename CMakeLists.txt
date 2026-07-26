cmake_minimum_required(VERSION 3.16)
project(kbd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KEYPAD_LAYOUT_VID "Build for the VID 14-key membrane keyboard" ON)

add_executable(kbd
  src/main.cpp
  src/mcp23017.cpp
)

target_include_directories(kbd PRIVATE include)

if (KEYPAD_LAYOUT_VID)
  target_compile_definitions(kbd PRIVATE KEYPAD_LAYOUT_VID=1)
  message(STATUS "Keypad layout: VID 14-key")
else()
  target_compile_definitions(kbd PRIVATE KEYPAD_LAYOUT_VID=0)
  message(STATUS "Keypad layout: legacy 4x4")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(kbd
 PRIVATE -Wall -Wextra -Wpedantic)
endif()
