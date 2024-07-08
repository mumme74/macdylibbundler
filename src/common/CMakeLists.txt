# this lib should not depend on anything in any other lib
# except libc++

target_sources(
  common
  PRIVATE
    Common.cpp
    Types.cpp
  PUBLIC
    Common.h
    Types.h
    WinPort.h
)