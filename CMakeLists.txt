cmake_minimum_required(VERSION 3.5.1)
project(tunproxy C)

include_directories("src")
set(CMAKE_C_STANDARD 99)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(SOFTWARE_MAJOR "1")
set(SOFTWARE_MINOR "1")
set(SOFTWARE_BUILD "0")
add_definitions(
        -DSOFTWARE_MAJOR=${SOFTWARE_MAJOR}
        -DSOFTWARE_MINOR=${SOFTWARE_MINOR}
        -DSOFTWARE_BUILD=${SOFTWARE_BUILD}
)

add_definitions(-Wall)

file(GLOB_RECURSE SRC_FILES "src/*.c" "src/*.h")
add_executable(tunproxy ${SRC_FILES})
