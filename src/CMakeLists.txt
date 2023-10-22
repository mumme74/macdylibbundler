
add_subdirectory(jsonlib)
add_subdirectory(dylib)

target_sources(dylibbundler PRIVATE
  main.cpp
)
target_link_libraries(
  dylibbundler
  PUBLIC
    dylib
)
target_include_directories(
  dylibbundler
  PUBLIC
  ${CMAKE_SOURCE_DIR}/src/dylib
)
