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

#ifndef _TYPES_H_
#define _TYPES_H_

#include <filesystem>

namespace std::filesystem {

class extended_path : public path {
public:
  extended_path() = default;
  extended_path(const extended_path& _path) = default;
  extended_path(extended_path&& _path) :
    path(std::move(_path))
  {}
  extended_path(std::string&& _path):
    path(std::move(_path))
  {}
  extended_path(const std::string& _path):
    path(_path)
  {}
  ~extended_path() = default;

  extended_path& operator=(const extended_path &other)
  {
    path::operator=(other);
    return *this;
  }
  extended_path& operator=(const path &other)
  {
    path::operator=(other);
    return *this;
  }
  extended_path& operator=(extended_path&& rhs) noexcept
  {
    path::operator=(std::move(rhs));
    return *this;
  }
  extended_path& operator=(path&& rhs) noexcept
  {
    path::operator=(std::move(rhs));
    return *this;
  }
  extended_path& operator=(std::string&& __source)
  {
    path::operator=(std::move(__source));
    return *this;
  }
  extended_path& assign(std::string&& __source)
  {
    path::assign(std::move(__source));
    return *this;
  }
  extended_path& operator/=(const extended_path& path)
  {
    path::operator/=(path);
    return *this;
  }
  extended_path& operator/=(const path& path)
  {
    path::operator/=(path);
    return *this;
  }
  extended_path& operator/=(const std::string& str)
  {
    path::operator/=(str);
    return *this;
  }
  extended_path& operator/=(std::string_view str)
  {
    path::operator/=(str);
    return *this;
  }
  extended_path& operator+=(const path& __x)
  {
    path::operator+=(__x);
    return *this;
  }
  extended_path& operator+=(const extended_path& __x)
  {
    path::operator+=(__x);
    return *this;
  }
  extended_path& operator+=(const std::string& __x)
  {
    path::operator+=(__x);
    return *this;
  }
  extended_path& operator+=(const value_type* __x)
  {
    path::operator+=(__x);
    return *this;
  }
  extended_path& operator+=(value_type __x)
  {
    path::operator+=(__x);
    return *this;
  }
  /// Append one path to another
  friend extended_path operator/(const extended_path& __lhs, const path& __rhs)
  {
    extended_path epath{__lhs};
    epath /= __rhs;
    return epath;
  }
  friend extended_path operator+(const extended_path& __lhs, const path& __rhs)
  {
    extended_path epath{__lhs};
    epath += __rhs;
    return epath;
  }

  extended_path filename() const
  {
    auto ret = path::filename();
    return *static_cast<extended_path*>(&ret);
  }


  /// return path from start until dir, but not including until
  extended_path before(std::filesystem::path::iterator dir) const;
  /// return path from start until endsWith, but not including dir
  extended_path before(std::string_view endsWith) const;

  /// return path from start until dir, including upto
  extended_path upto(std::filesystem::path::iterator dir) const;
  /// return path from start until dir, including upto dir
  extended_path upto(std::string_view endsWith) const;

  /// return path from dir to end of path
  extended_path from(std::filesystem::path::iterator dir) const;
  /// return path from dir to end of path, dirname endsWith
  extended_path from(std::string_view endsWith) const;

  /// return path after dir to end of path
  extended_path after(std::filesystem::path::iterator dir) const;
  /// return path after dir to end of path, dirname endsWith
  extended_path after(std::string_view endsWith) const;

  /// remove the first dir in a relative path
  extended_path strip_prefix() const;

  /// return this with an end slash if not present
  extended_path end_sep() const;

  /// return this without a end slash if present
  extended_path end_wo_sep() const;

  /// return the last entry
  extended_path end_name() const;

private:
  iterator findDirEntry(std::string_view endsWith) const;
};
} // namespace std::filesystem


template<typename T>
T reverseEndian(T in)
{
  T buf[sizeof(T)];
  for (size_t i = 0, j = sizeof(T) -1; i < sizeof(T); ++i, j--)
    buf[j] = ((uint8_t*)&in)[i];
  return (T)buf[0];
}

static constexpr uint32_t _magic_uint32_ = 0x01020304;
static constexpr uint8_t _magic_ = (const uint8_t&)_magic_uint32_;
static constexpr bool hostIsLittleEndian = _magic_ == 0x04;
static constexpr bool hostIsBigEndian = _magic_ == 0x01;
static_assert(hostIsLittleEndian || hostIsLittleEndian,
             "Cannot determine endianness!");

using PathRef = const std::filesystem::extended_path&;
using Path = std::filesystem::extended_path;

/// @brief  convert an uint from native to bigendian and reverse again
typedef union _bigendian  {
public:
    _bigendian(uint16_t vlu);
    _bigendian(uint32_t vlu);
    _bigendian(uint64_t vlu);
    /// @brief go back to native again
    uint16_t u16native();
    uint32_t u32native();
    uint64_t u64native();

    /// @brief bytes in bigendian order
    union {
      uint8_t u16arr[2];
      uint16_t u16vlu;
    };
    union {
      uint8_t u32arr[4];
      uint32_t u32vlu;
    };
    union {
      uint8_t u64arr[8];
      uint64_t u64vlu;
    };

} bigendian_t;

#endif // _TYPES_H_
