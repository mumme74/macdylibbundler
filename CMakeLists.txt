cmake_minimum_required(VERSION 3.15)
project(dylibbundler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_TESTS "Build tests" OFF)

string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

add_compile_options(-pipe -Wall -Wextra -Wpedantic)

add_library(json STATIC "")
add_library(dylib STATIC "")
add_executable(dylibbundler "")

add_subdirectory(src)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_custom_command(
    TARGET dylibbundler
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scripts
        ${CMAKE_CURRENT_BINARY_DIR}/scripts
)
