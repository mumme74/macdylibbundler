
target_sources(
  argparser
  PRIVATE
    ArgParser.cpp
  PUBLIC
    ArgParser.h
)
target_include_directories(
  argparser PUBLIC ${CMAKE_SOURCE_DIR}/src/common
)
target_link_libraries(argparser PRIVATE common)