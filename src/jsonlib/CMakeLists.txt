# this lib should not depend on anything in any other lib
# except libc++
target_sources(
  json
  PRIVATE
    Json.cpp
  PUBLIC
    Json.h
)