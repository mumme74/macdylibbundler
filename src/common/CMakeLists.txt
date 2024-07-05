# this lib should not depend on anything in any other lib
# except libc++

target_sources(
  common
  PRIVATE
    Common.cpp
    Types.cpp
    Tools.cpp
    MachO.cpp
  PUBLIC
    Common.h
    Types.h
    Tools.h
    MachO.h
)