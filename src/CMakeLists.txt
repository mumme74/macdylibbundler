
target_sources(
  json
  PRIVATE
    Json.cpp
  PUBLIC
    Json.h
)

target_sources(
  dylib
  PRIVATE
    Dependency.cpp
    DylibBundler.cpp
    Settings.cpp
    Utils.cpp
  PUBLIC
    DylibBundler.h
    Settings.h
    Dependency.h
    Utils.h
)
target_link_libraries(dylib PUBLIC json)

target_sources(dylibbundler PRIVATE
  main.cpp
)
target_link_libraries(dylibbundler PUBLIC dylib)
