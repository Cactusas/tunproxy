cmake_minimum_required(VERSION 3.21)
project(tunproxy C)

include_directories("src")
set(CMAKE_C_STANDARD 99)

file(GLOB_RECURSE SRC_FILES "src/*.c" "src/*.h")
add_executable(tunproxy ${SRC_FILES})
