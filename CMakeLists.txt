cmake_minimum_required(VERSION 3.16)
project(kbd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KEYPAD_LAYOUT_VID "Build for the VID 14-key membrane keyboard" ON)
set(KEYPAD_LONG_PRESS_MS 1000 CACHE STRING
    "Milliseconds a key must be held to count as a long press")

if (NOT KEYPAD_LONG_PRESS_MS MATCHES "^[1-9][0-9]*$")
  message(FATAL_ERROR "KEYPAD_LONG_PRESS_MS must be a positive integer")
endif()

if (KEYPAD_LONG_PRESS_MS GREATER 2147483647)
  message(FATAL_ERROR "KEYPAD_LONG_PRESS_MS must not exceed 2147483647")
endif()

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

target_compile_definitions(kbd PRIVATE
  KEYPAD_LONG_PRESS_MS=${KEYPAD_LONG_PRESS_MS}
)
message(STATUS "Long press threshold: ${KEYPAD_LONG_PRESS_MS} ms")

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(kbd
 PRIVATE -Wall -Wextra -Wpedantic)
endif()
