
target_sources(
  asyncfifo
  PRIVATE
    asyncfifo.cpp
  PUBLIC
    asyncfifo.hpp
)
#target_include_directories(
#  asyncfifo PUBLIC ${CMAKE_SOURCE_DIR}/src/common
#)
#target_link_libraries(asyncfifo PRIVATE common)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()