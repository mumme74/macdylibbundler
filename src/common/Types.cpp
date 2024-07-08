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
  auto ret = std::find_if(begin(), end(), [&](path p)->bool {
    auto name = p.string();
    if (name.size() >= endsWith.size()){
      auto str = name.substr(name.size()-endsWith.size());
      return str == endsWith;
    }
    return false;
  });
  return ret;
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
  std::string str{this->string()};
  while (str.back() == '.')
    str = str.substr(0, str.size()-1);
  if (str.size() && str.back() != preferred_separator)
    str += preferred_separator;
  return Path(str);
}

extended_path
extended_path::end_wo_sep() const
{
  std::string str{this->string()};
  while (str.back() == '.')
    str = str.substr(0, str.size()-1);
  if (str.size() && str.back() == preferred_separator)
    str = str.substr(0, str.size()-1);
  return Path(str);
}

extended_path
extended_path::end_name() const
{
  iterator last = end();
  for (auto it = begin(); it != end(); ++it) {
    last = it;
  }
  if (last != end()) {
    return extended_path{last->string()};
  }
  return extended_path{};
}

} // namespace std::filesystem

// ---------------------------------------------------------

_bigendian::_bigendian(uint16_t vlu) {
  if constexpr(hostIsLittleEndian) {
    u16vlu = reverseEndian<uint16_t>(vlu);
  } else {
    u16vlu = vlu;
  }
}

_bigendian::_bigendian(uint32_t vlu)  {
  if constexpr(hostIsLittleEndian) {
    u32vlu = reverseEndian<uint32_t>(vlu);
  } else{
    u32vlu = vlu;
  }
}

_bigendian::_bigendian(uint64_t vlu)  {
  if constexpr(hostIsLittleEndian) {
    u64vlu = reverseEndian<uint64_t>(vlu);
  } else{
    u64vlu = vlu;
  }
}

uint16_t
_bigendian::u16native() {
  if constexpr(hostIsLittleEndian) {
    return reverseEndian(u16vlu);
  } else {
    return (uint16_t)u16arr[0];
  }
}

uint32_t
_bigendian::u32native() {
  if constexpr(hostIsLittleEndian) {
    return reverseEndian(u32vlu);
  } else {
    return (uint32_t)u32arr[0];
  }
}

uint64_t
_bigendian::u64native() {
  if constexpr(hostIsLittleEndian) {
    return reverseEndian(u64vlu);
  } else {
    return (uint64_t)u64arr[0];
  }
}
