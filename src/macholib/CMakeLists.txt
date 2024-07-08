# a library used to parse and modify a mach object binary
# such as changing its import paths

target_sources(
  macho
  PRIVATE
    MachO.cpp
  PUBLIC
    MachO.h
)