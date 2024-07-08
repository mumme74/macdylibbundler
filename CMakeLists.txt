cmake_minimum_required(VERSION 3.15)
project(dylibbundler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_TESTS "Build tests" OFF)
option(USE_SCRIPTS "Run scripts after complete, neccesary Qt bundles for example" ON)

string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

add_compile_options(-pipe -Wall -Wextra -Wpedantic)

add_library(common STATIC "")
add_library(json STATIC "")
add_library(macho STATIC "")
add_library(dylib STATIC "")
add_library(argparser STATIC "")
add_executable(dylibbundler "")
add_executable(object_tool "")

add_subdirectory(src)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(USE_SCRIPTS)
    target_compile_definitions(dylib PUBLIC USE_SCRIPTS=1)
    target_compile_definitions(dylibbundler PUBLIC USE_SCRIPTS=1)
    add_custom_command(
        TARGET dylibbundler PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/scripts
            ${CMAKE_BINARY_DIR}/scripts
    )
    add_custom_target(scripts)
    add_custom_command(
        TARGET scripts PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/scripts
            ${CMAKE_BINARY_DIR}/scripts
    )
endif()

