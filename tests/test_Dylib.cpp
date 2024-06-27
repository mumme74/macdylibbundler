/*
The MIT License (MIT)

Copyright (c) 2023 Fredrik Johansson

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
#include <gtest/gtest.h>

#include <algorithm>
#include "Types.h"
namespace fs = std::filesystem;

const bool isLittle = ((uint8_t*)0x01020304)[0] == 0x01;

TEST(bigendian_t, test16) {
  bigendian_t u16{(uint16_t) 0x184};
  EXPECT_EQ(u16.u16native(), 0x0184);
  EXPECT_EQ(u16.u16arr[1], 0x84);
  EXPECT_EQ(u16.u16arr[0], 0x01);
};

TEST(bigendian_t, test32) {
  bigendian_t u32{(uint32_t) 0x01020304};
  EXPECT_EQ(u32.u32native(), 0x01020304);
  EXPECT_EQ(u32.u16arr[0], 0x04);
  EXPECT_EQ(u32.u16arr[1], 0x03);

  EXPECT_EQ(u32.u32arr[0], 0x04);
  EXPECT_EQ(u32.u32arr[1], 0x03);
  EXPECT_EQ(u32.u32arr[2], 0x02);
  EXPECT_EQ(u32.u32arr[3], 0x01);
};

TEST(bigendian_t, test64) {
  bigendian_t u64{(uint32_t) 0x0102030405060708};
  EXPECT_EQ(u64.u64native(), 0x0102030405060708);
  EXPECT_EQ(u64.u64arr[0], 0x08);
  EXPECT_EQ(u64.u64arr[1], 0x07);
  EXPECT_EQ(u64.u64arr[2], 0x06);
  EXPECT_EQ(u64.u64arr[3], 0x05);
  EXPECT_EQ(u64.u64arr[4], 0x04);
  EXPECT_EQ(u64.u64arr[5], 0x03);
  EXPECT_EQ(u64.u64arr[6], 0x02);
  EXPECT_EQ(u64.u64arr[7], 0x01);
};

// --------------------------------------------------------

TEST(extended_path, until) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.until(fizz).c_str(), "foo/bar/baz");
  EXPECT_EQ(p1.from("fizz").c_str(), "foo/bar/baz");
  EXPECT_EQ(p1.from("foo").c_str(), "");
  EXPECT_EQ(p1.from("nonexistent").c_str(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("bar").c_str(), "foo");
  EXPECT_EQ(p1.from("buzz").c_str(), "foo/bar/baz/fizz");
};

TEST(extended_path, upto) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.from(fizz).c_str(), "foo/bar/baz/fizz");
  EXPECT_EQ(p1.from("fizz").c_str(), "foo/bar/baz/fizz");
  EXPECT_EQ(p1.from("foo").c_str(), "foo");
  EXPECT_EQ(p1.from("nonexistent").c_str(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("bar").c_str(), "foo/bar");
  EXPECT_EQ(p1.from("buzz").c_str(), "foo/bar/baz/fizz/buzz");
};

TEST(extended_path, from) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.from(fizz).c_str(), "fizz/buzz");
  EXPECT_EQ(p1.from("fizz").c_str(), "fizz/buzz");
  EXPECT_EQ(p1.from("foo").c_str(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("nonexistent").c_str(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("bar").c_str(), "bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("buzz").c_str(), "buzz");
};

TEST(extended_path, after) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.from(fizz).c_str(), "buzz");
  EXPECT_EQ(p1.from("fizz").c_str(), "buzz");
  EXPECT_EQ(p1.from("foo").c_str(), "bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("nonexistent").c_str(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("bar").c_str(), "baz/fizz/buzz");
  EXPECT_EQ(p1.from("buzz").c_str(), "");
};
