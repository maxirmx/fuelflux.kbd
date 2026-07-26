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
  src/key_press_tracker.cpp
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
  target_compile_options(kbd PRIVATE
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

include(CTest)

if (BUILD_TESTING)
  add_executable(key_press_tracker_test
    tests/key_press_tracker_test.cpp
    src/key_press_tracker.cpp
  )
  target_include_directories(key_press_tracker_test PRIVATE include)

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(key_press_tracker_test
      PRIVATE -Wall -Wextra -Wpedantic
    )
  endif()

  add_test(NAME key_press_tracker COMMAND key_press_tracker_test)

  add_test(NAME kbd_help_default_port COMMAND kbd --help)
  set_tests_properties(kbd_help_default_port PROPERTIES
    PASS_REGULAR_EXPRESSION "--port    B"
  )

  add_test(NAME kbd_port_a_option COMMAND kbd --port A --help)
  add_test(NAME kbd_port_b_option COMMAND kbd --port B --help)
endif()
