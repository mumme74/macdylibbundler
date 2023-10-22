include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# prevent overede parent linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(jsontest test_Json.cpp)
target_link_libraries(jsontest gtest_main json)
target_include_directories(jsontest PUBLIC ${CMAKE_SOURCE_DIR}/src/jsonlib)
add_test(NAME jsontest COMMAND $<TARGET_FILE:jsontest>
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
