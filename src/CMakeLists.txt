add_subdirectory(common)
add_subdirectory(jsonlib)
add_subdirectory(macholib)
add_subdirectory(dylib)
add_subdirectory(argparser)

# the main dylibbunler executable
target_sources(dylibbundler PRIVATE
  main.cpp
)

# macho lib dependencies
target_link_libraries(
  macho
  PUBLIC
    common
)
target_include_directories(
  macho
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src/common
)

# dylibbundler dependencies
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

# the secondary object tool
target_sources(object_tool PRIVATE
  object_tool.cpp
)
target_link_libraries(object_tool
  PUBLIC common argparser macho
)
target_include_directories(
  object_tool
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common
  PUBLIC ${CMAKE_SOURCE_DIR}/src/argparser
  PUBLIC ${CMAKE_SOURCE_DIR}/src/macholib
)
