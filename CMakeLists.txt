cmake_minimum_required(VERSION 3.16)
project(mcp23017_keypad_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mcp23017_keypad_demo
  src/main.cpp
  src/mcp23017.cpp
)

target_include_directories(mcp23017_keypad_demo PRIVATE include)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(mcp23017_keypad_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
