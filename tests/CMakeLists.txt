include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# prevent overide parent linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()


# json lib
add_executable(jsontest test_Json.cpp)
target_link_libraries(jsontest gtest_main json)
target_include_directories(jsontest PUBLIC ${CMAKE_SOURCE_DIR}/src/jsonlib)
add_test(NAME jsontest COMMAND $<TARGET_FILE:jsontest>
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# argparser lib
add_executable(argparsertest test_ArgParser.cpp)
target_link_libraries(argparsertest gtest_main argparser)
target_include_directories(
  argparsertest PUBLIC
    ${CMAKE_SOURCE_DIR}/src/argparser
    ${CMAKE_SOURCE_DIR}/src/dylib
)
add_test(NAME argparsertest COMMAND $<TARGET_FILE:argparsertest>
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# common lib
add_executable(commontest test_Common.cpp)
target_link_libraries(commontest gtest_main gmock_main common)
target_include_directories(
  commontest PUBLIC
    ${CMAKE_SOURCE_DIR}/src/common
    ${GTEST_DIR}/googlemock/include
)
add_test(NAME commontest COMMAND $<TARGET_FILE:commontest>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# macho lib
add_executable(machotest test_MachO.cpp)
target_link_libraries(machotest gtest_main gmock_main common macho)
target_include_directories(
  machotest PUBLIC
    ${CMAKE_SOURCE_DIR}/src/macholib
    ${GTEST_DIR}/googlemock/include
)
add_test(NAME machotest COMMAND $<TARGET_FILE:machotest>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(
  TARGET machotest PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_SOURCE_DIR}/tests/testdata
      ${CMAKE_BINARY_DIR}/tests/testdata
)

# dylib
add_executable(dylibtest test_Dylib.cpp)
target_link_libraries(dylibtest gtest_main gmock_main dylib)
target_include_directories(
  dylibtest PUBLIC
    ${CMAKE_SOURCE_DIR}/src/dylib
    ${GTEST_DIR}/googlemock/include
)
add_test(NAME dylibtest COMMAND $<TARGET_FILE:dylibtest>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})