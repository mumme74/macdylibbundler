add_subdirectory(common)
add_subdirectory(jsonlib)
add_subdirectory(dylib)
add_subdirectory(argparser)

target_sources(dylibbundler PRIVATE
  main.cpp
)
target_link_libraries(
  dylibbundler
  PUBLIC
    dylib
    argparser
)
target_include_directories(
  dylibbundler
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src/dylib
    ${CMAKE_SOURCE_DIR}/src/argparser
)
