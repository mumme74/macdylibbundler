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
#include <gmock/gmock.h>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include "Common.h"
#include "Types.h"

using ::testing::MatchesRegex;

TEST(StripPrefixTest, testPaths) {
  EXPECT_EQ(stripPrefix("thisisafile"), "thisisafile");
  EXPECT_EQ(stripPrefix("dir/file"), "file");
  EXPECT_EQ(stripPrefix("/dir/file"), "file");
  EXPECT_EQ(stripPrefix("root/dir/file"), "file");
  EXPECT_EQ(stripPrefix("/root/dir/file"), "file");
};

// --------------------------------------------------------

TEST(RTrimTest, testString) {
  EXPECT_EQ(rtrim("str"), "str");
  EXPECT_EQ(rtrim("str "), "str");
  EXPECT_EQ(rtrim("  str  "), "  str");
  EXPECT_EQ(rtrim("/str / "), "/str /");
};

// --------------------------------------------------------

TEST(StripLastSlash, testPaths) {
  EXPECT_EQ(stripLastSlash("file  /"), "file  ");
  EXPECT_EQ(stripLastSlash("file/"), "file");
  EXPECT_EQ(stripLastSlash("root/file"), "root");
  EXPECT_EQ(stripLastSlash("root/file/"), "root/file");
  EXPECT_EQ(stripLastSlash("/root/file"), "/root");
  EXPECT_EQ(stripLastSlash("/root/file/"), "/root/file");
  EXPECT_EQ(stripLastSlash("/root/folder/file"), "/root/folder");
  EXPECT_EQ(stripLastSlash("/root/folder/file/"), "/root/folder/file");
};

// ---------------------------------------------------------

TEST(TokenizeTest, splitPath) {
  auto v1 = tokenize("dir/file:next/dir/:third", ":");
  ASSERT_EQ(v1.size(), 3);
  EXPECT_EQ(v1[0], "dir/file");
  EXPECT_EQ(v1[1], "next/dir/");
  EXPECT_EQ(v1[2], "third");
}

TEST(TokenizeTest, splitManyChNeedle) {
  auto v1 = tokenize("dir/file**--**next/dir/**--**third", "**--**");
  ASSERT_EQ(v1.size(), 3);
  EXPECT_EQ(v1[0], "dir/file");
  EXPECT_EQ(v1[1], "next/dir/");
  EXPECT_EQ(v1[2], "third");
};

// ---------------------------------------------------------

TEST(ExitMsg, default) {
  EXPECT_EXIT(exitMsg("msg"), testing::ExitedWithCode(1), "msg");
  auto err = std::make_error_code(std::errc::bad_message);
  EXPECT_EXIT(exitMsg("msg2", err), testing::ExitedWithCode(err.value()), "msg2 Bad message");
};

// -----------------------------------------------------------

namespace fs = std::filesystem;

const uint32_t _endianTest = 0x01020304;
const uint8_t *_endianTestArr = (uint8_t*)&_endianTest;
const bool isLittle = _endianTestArr[0] == 0x01;

TEST(bigendian_t, test16) {
  bigendian_t u16{(uint16_t) 0x184};
  EXPECT_EQ(u16.u16native(), 0x0184);
  EXPECT_EQ(u16.u16arr[1], 0x84);
  EXPECT_EQ(u16.u16arr[0], 0x01);
};

TEST(bigendian_t, test32) {
  bigendian_t u32{(uint32_t) 0x04030201};
  EXPECT_EQ(u32.u32native(), 0x04030201);
  EXPECT_EQ(u32.u16arr[0], 0x04);
  EXPECT_EQ(u32.u16arr[1], 0x03);

  EXPECT_EQ(u32.u32arr[0], 0x04);
  EXPECT_EQ(u32.u32arr[1], 0x03);
  EXPECT_EQ(u32.u32arr[2], 0x02);
  EXPECT_EQ(u32.u32arr[3], 0x01);
};

TEST(bigendian_t, test64) {
  bigendian_t u64{(uint64_t) 0x0807060504030201};
  EXPECT_EQ(u64.u64native(), 0x0807060504030201);
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
  EXPECT_EQ(p1.before(fizz).string(), "foo/bar/baz");
  EXPECT_EQ(p1.before("fizz").string(), "foo/bar/baz");
  EXPECT_EQ(p1.before("foo").string(), "");
  EXPECT_EQ(p1.before("nonexistent").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.before("bar").string(), "foo");
  EXPECT_EQ(p1.before("buzz").string(), "foo/bar/baz/fizz");
  EXPECT_EQ(p1.before("ba").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.before("az").string(), "foo/bar");
};

TEST(extended_path, upto) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.upto(fizz).string(), "foo/bar/baz/fizz");
  EXPECT_EQ(p1.upto("fizz").string(), "foo/bar/baz/fizz");
  EXPECT_EQ(p1.upto("foo").string(), "foo");
  EXPECT_EQ(p1.upto("nonexistent").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.upto("bar").string(), "foo/bar");
  EXPECT_EQ(p1.upto("buzz").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.upto("ba").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.upto("az").string(), "foo/bar/baz");
};

TEST(extended_path, from) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.from(fizz).string(), "fizz/buzz");
  EXPECT_EQ(p1.from("fizz").string(), "fizz/buzz");
  EXPECT_EQ(p1.from("foo").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("nonexistent").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("bar").string(), "bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("buzz").string(), "buzz");
  EXPECT_EQ(p1.from("ba").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.from("az").string(), "baz/fizz/buzz");
};

TEST(extended_path, after) {
  Path p1{"foo/bar/baz/fizz/buzz"};
  auto fizz = std::find(p1.begin(), p1.end(), "fizz");
  EXPECT_EQ(p1.after(fizz).string(), "buzz");
  EXPECT_EQ(p1.after("fizz").string(), "buzz");
  EXPECT_EQ(p1.after("foo").string(), "bar/baz/fizz/buzz");
  EXPECT_EQ(p1.after("nonexistent").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.after("bar").string(), "baz/fizz/buzz");
  EXPECT_EQ(p1.after("buzz").string(), "");
  EXPECT_EQ(p1.after("ba").string(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p1.after("az").string(), "fizz/buzz");
};

TEST(extended_path, end_sep) {
  Path p1{"foo/bar/baz/fizz/buzz"},
       p2{"foo/bar/baz/fizz/buzz/"},
       p3{""},
       p4{"foo"},
       p5{"/root/."},
       p6{"/tmp/.."};
  EXPECT_EQ(p1.end_sep(), "foo/bar/baz/fizz/buzz/");
  EXPECT_EQ(p2.end_sep(), "foo/bar/baz/fizz/buzz/");
  EXPECT_EQ(p3.end_sep(), "");
  EXPECT_EQ(p4.end_sep(), "foo/");
  EXPECT_EQ(p5.end_sep(), "/root/");
  EXPECT_EQ(p6.end_sep(), "/tmp/");
};

TEST(extended_path, end_wo_sep) {
  Path p1{"foo/bar/baz/fizz/buzz"},
       p2{"foo/bar/baz/fizz/buzz/"},
       p3{""},
       p4{"foo"},
       p5{"/root/."},
       p6{"/tmp/.."};
  EXPECT_EQ(p1.end_wo_sep(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p2.end_wo_sep(), "foo/bar/baz/fizz/buzz");
  EXPECT_EQ(p3.end_wo_sep(), "");
  EXPECT_EQ(p4.end_wo_sep(), "foo");
  EXPECT_EQ(p5.end_wo_sep(), "/root");
  EXPECT_EQ(p6.end_wo_sep(), "/tmp");
};

TEST(extended_path, strip_prefix) {
  Path p1{"foo/bar/baz/fizz/buzz"},
       p2{"@rpath/bar/baz/fizz/buzz/"},
       p3{""},
       p4{"foo"},
       p5{"/root/."},
       p6{"./tmp/.."};
  EXPECT_EQ(p1.strip_prefix(), "bar/baz/fizz/buzz");
  EXPECT_EQ(p2.strip_prefix(), "bar/baz/fizz/buzz/");
  EXPECT_EQ(p3.strip_prefix(), "");
  EXPECT_EQ(p4.strip_prefix(), "");
  EXPECT_EQ(p5.strip_prefix(), "root/.");
  EXPECT_EQ(p6.strip_prefix(), "tmp/..");
};

TEST(extended_path, end_name) {
  Path p1{"foo/bar/baz/fizz/buzz"},
       p2{"@rpath/bar/baz/fizz/buzz/"},
       p3{""},
       p4{"foo"},
       p5{"/root/."},
       p6{"./tmp/.."};
  EXPECT_EQ(p1.end_name(), "buzz");
  EXPECT_EQ(p2.end_name(), "");
  EXPECT_EQ(p3.end_name(), "");
  EXPECT_EQ(p4.end_name(), "foo");
  EXPECT_EQ(p5.end_name(), ".");
  EXPECT_EQ(p6.end_name(), "..");
};

