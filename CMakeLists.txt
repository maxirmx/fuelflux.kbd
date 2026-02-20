cmake_minimum_required(VERSION 3.16)
project(kbd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kbd
  src/main.cpp
  src/mcp23017.cpp
)

target_include_directories(kbd PRIVATE include)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(kbd
 PRIVATE -Wall -Wextra -Wpedantic)
endif()
