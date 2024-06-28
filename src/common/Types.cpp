/*
The MIT License (MIT)

Copyright (c) 2024 Fredrik Johansson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

#include <algorithm>
#include "Types.h"

namespace std::filesystem {

extended_path::iterator
extended_path::findDirEntry(std::string_view endsWith) const
{
  return std::find_if(begin(), end(), [&](const std::string name){
    if (name.size() >= endsWith.size()){
      auto str = name.substr(name.size()-endsWith.size());
      return str == endsWith;
    }
    return false;
  });
}

extended_path
extended_path::before(extended_path::iterator dir) const
{
  extended_path ret;
  for (auto it = begin(); it != dir; ++it) {
    ret /= *it;
  }
  return ret;
}

extended_path
extended_path::before(std::string_view endsWith) const
{
  auto it = findDirEntry(endsWith);
  if (it == end()) return extended_path(*this);
  return before(it);
}

extended_path
extended_path::upto(extended_path::iterator dir) const
{
  dir++;
  return before(dir);
}

extended_path
extended_path::upto(std::string_view endsWith) const
{
  auto it = findDirEntry(endsWith);
  if (it == end()) return extended_path(*this);
  return upto(it);
}

extended_path
extended_path::from(extended_path::iterator dir) const
{
  extended_path ret;
  for (auto it = dir; it != end(); ++it) {
    ret /= *it;
  }
  return ret;
}

extended_path
extended_path::from(std::string_view endsWith) const
{
  auto it = findDirEntry(endsWith);
  if (it == end()) return extended_path(*this);
  return from(it);
}

extended_path
extended_path::after(extended_path::iterator dir) const
{
  dir++;
  return from(dir);
}

extended_path
extended_path::after(std::string_view endsWith) const
{
  auto it = findDirEntry(endsWith);
  if (it == end()) return extended_path(*this);
  return after(it);
}

extended_path
extended_path::strip_prefix() const
{
  extended_path ret;
  auto it = begin();
  for (it++; it != end(); ++it) {
    ret /= *it;
  }
  return ret;

}

extended_path
extended_path::end_sep() const
{
  std::string str{*this};
  while (str.back() == '.')
    str = str.substr(0, str.size()-1);
  if (str.size() && str.back() != preferred_separator)
    str += preferred_separator;
  return Path(str);
}

extended_path
extended_path::end_wo_sep() const
{
  std::string str{*this};
  while (str.back() == '.')
    str = str.substr(0, str.size()-1);
  if (str.size() && str.back() == preferred_separator)
    str = str.substr(0, str.size()-1);
  return Path(str);
}

extended_path
extended_path::end_name() const
{
  iterator last;
  for (auto it = begin(); it != end(); ++it) {
    last = it;
  }
  return static_cast<extended_path>(*last);
}

} // namespace std::filesystem

// ---------------------------------------------------------

_bigendian::_bigendian(uint16_t vlu) {
  if constexpr(little) {
    uint8_t *a = (uint8_t*)(&vlu);
    _bigendian::conv(u16arr, a, 2);
  } else {
    auto u = (uint16_t*)u16arr;
    u[0] = vlu;
  }
}

_bigendian::_bigendian(uint32_t vlu)  {
  if constexpr(little) {
    uint8_t *a = (uint8_t*)(&vlu);
    _bigendian::conv(u32arr, a, 4);
  } else{
    auto u = (uint32_t*)u32arr;
    u[0] = vlu;
  }
}

_bigendian::_bigendian(uint64_t vlu)  {
  if constexpr(little) {
    uint8_t *a = (uint8_t*)(&vlu);
    _bigendian::conv(u64arr, a, 8);
  } else{
    auto u = (uint64_t*)u64arr;
    u[0] = vlu;
  }
}

uint16_t
_bigendian::u16native() {
  if constexpr(little) {
    uint16_t vlu;
    uint8_t *a = (uint8_t*)(&vlu);
    _bigendian::conv(a, u16arr, 2);
    return vlu;
  } else {
    return (uint16_t)u16arr[0];
  }
}

uint32_t
_bigendian::u32native() {
  if constexpr(little) {
    uint32_t vlu;
    uint8_t *a = (uint8_t*)(&vlu);
    _bigendian::conv(a, u32arr, 4);
    return vlu;
  } else {
    return (uint32_t)u32arr[0];
  }
}

uint64_t
_bigendian::u64native() {
  if constexpr(little) {
    uint64_t vlu;
    uint8_t *a = (uint8_t*)(&vlu);
    _bigendian::conv(a, u64arr, 8);
    return vlu;
  } else {
    return (uint64_t)u64arr[0];
  }
}

//static
void _bigendian::conv(uint8_t* uTo, uint8_t* uFrom, size_t sz)
{
  for (size_t i = 0, j = sz-1; i < sz; ++i, --j)
    uTo[i] = uFrom[j];
}
