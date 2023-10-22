target_sources(
  dylib
  PRIVATE
    Dependency.cpp
    DylibBundler.cpp
    Settings.cpp
    Utils.cpp
    ScriptRunner.cpp
  PUBLIC
    DylibBundler.h
    Settings.h
    Dependency.h
    Utils.h
    ScriptRunner.h
)
target_link_libraries(dylib PUBLIC json)
target_include_directories(
  dylib PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/jsonlib
)