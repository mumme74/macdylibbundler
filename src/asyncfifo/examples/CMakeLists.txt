
add_executable(asyncfifo_examples "")

target_sources(
  asyncfifo_examples
  PRIVATE
    testfifoAsync.cpp
)
target_include_directories(
  asyncfifo_examples PUBLIC ${CMAKE_SOURCE_DIR}/src/asyncfifo
)
target_link_libraries(asyncfifo_examples PRIVATE asyncfifo)

if(USE_SCRIPTS)
    add_custom_command(
        TARGET asyncfifo_examples PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/asyncfifo/examples
            ${CMAKE_BINARY_DIR}/src/asyncfifo/examples
    )
endif()