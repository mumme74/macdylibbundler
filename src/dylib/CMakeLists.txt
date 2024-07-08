target_sources(
  dylib
  PRIVATE
    Dependency.cpp
    DylibBundler.cpp
    Settings.cpp
    Utils.cpp
    ScriptRunner.cpp
    Tools.cpp
  PUBLIC
    DylibBundler.h
    Settings.h
    Dependency.h
    Utils.h
    ScriptRunner.h
    Tools.h
)
target_link_libraries(dylib PUBLIC common json macho)
target_include_directories(
  dylib PUBLIC
  ${CMAKE_SOURCE_DIR}/src/common
  ${CMAKE_SOURCE_DIR}/src/jsonlib
  ${CMAKE_SOURCE_DIR}/src/macholib
)