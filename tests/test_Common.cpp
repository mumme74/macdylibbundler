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

#include <iostream>
#include "Common.h"

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

