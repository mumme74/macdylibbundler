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
#include "ArgParser.h"


TEST(ArgItemStringTest, constructStrCallback) {
  std::string s;
  ArgItem itm("a","long","test short a", [&](std::string str){ s=str; });
  EXPECT_STREQ(itm.description(), "test short a");
  EXPECT_STREQ(itm.shSwitch(), "a");
  EXPECT_STREQ(itm.longSwitch(), "long");
  EXPECT_EQ(itm.option(), ArgItem::OptVluString);
  EXPECT_TRUE(itm.match("-a"));
  EXPECT_TRUE(itm.match("--long"));
  EXPECT_FALSE(itm.match("-b"));
  EXPECT_FALSE(itm.match("--longer"));
  EXPECT_EQ(itm.run("-a", nullptr), 1);
  EXPECT_EQ(s.empty(), true);
  EXPECT_EQ(itm.run("-a", "aValue"), 2);
  EXPECT_EQ(s, "aValue");
  itm.run("-a","nr2");
  EXPECT_EQ(s,"nr2");
};

TEST(ArgItemStringTest, constructStrCallbackReq) {
  std::string s;
  ArgItem itm("a","long","test short a", [&](std::string vlu){s=vlu;}, ArgItem::ReqVluString);
  EXPECT_STREQ(itm.description(), "test short a");
  EXPECT_STREQ(itm.shSwitch(), "a");
  EXPECT_STREQ(itm.longSwitch(), "long");
  EXPECT_EQ(itm.option(), ArgItem::ReqVluString);
};

TEST(ArgItemStringTest, helpOptionalVlu) {
  ArgItem itm("a","long","test short a", [&](std::string vlu){(void)vlu;});
  testing::internal::CaptureStdout();
  itm.help(" ");
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            " -a[=<vlu>], --long[=<vlu>]\n \ttest short a\n");
};

TEST(ArgItemStringTest, helpRequiredVlu) {
  ArgItem itm("a","long","test short a", [&](std::string vlu){(void)vlu;}, ArgItem::ReqVluString);
  testing::internal::CaptureStdout();
  itm.help(" ");
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            " -a=<vlu>, --long=<vlu>\n \ttest short a\n");
};

// ----------------------------------------------------------------

TEST(ArgItemBoolTest, constructBoolCallback) {
  bool v = false;
  ArgItem itm("a","long","test short a", [&](bool vlu){v=vlu;});
  EXPECT_STREQ(itm.description(), "test short a");
  EXPECT_STREQ(itm.shSwitch(), "a");
  EXPECT_STREQ(itm.longSwitch(), "long");
  EXPECT_EQ(itm.option(), ArgItem::VluTrue);
  EXPECT_TRUE(itm.match("-a"));
  EXPECT_TRUE(itm.match("--long"));
  EXPECT_FALSE(itm.match("-b"));
  EXPECT_FALSE(itm.match("--longer"));
  EXPECT_FALSE(v);
  itm.run("-a",nullptr);
  EXPECT_TRUE(v);
};

TEST(ArgItemBoolTest, constructBoolCallbackFalse) {
  bool v = false;
  ArgItem itm("a","long","test short a", [&](bool vlu){v=vlu;}, ArgItem::VluFalse);
  EXPECT_STREQ(itm.description(), "test short a");
  EXPECT_STREQ(itm.shSwitch(), "a");
  EXPECT_STREQ(itm.longSwitch(), "long");
  EXPECT_EQ(itm.option(), ArgItem::VluFalse);
};

TEST(ArgItemBoolTest, helpTrue) {
  ArgItem itm("a","long","test short a", [&](bool vlu){(void)vlu;});
  testing::internal::CaptureStdout();
  itm.help(" ");
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            " -a, --long\n \ttest short a\n");
};

TEST(ArgItemBoolTest, helpFalse) {
  ArgItem itm("a","long","test short a", [&](bool vlu){(void)vlu;}, ArgItem::VluFalse);
  testing::internal::CaptureStdout();
  itm.help(" ");
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            " -a, --long\n \ttest short a\n");
};

// -----------------------------------------------------------

TEST(ArgItemVoidTest, constructVoidCallback) {
  bool called = false;
  ArgItem itm("a","long","test short a", [&](){called=true;});
  EXPECT_STREQ(itm.description(), "test short a");
  EXPECT_STREQ(itm.shSwitch(), "a");
  EXPECT_STREQ(itm.longSwitch(), "long");
  EXPECT_EQ(itm.option(), ArgItem::VoidArg);
  EXPECT_TRUE(itm.match("-a"));
  EXPECT_TRUE(itm.match("--long"));
  EXPECT_FALSE(itm.match("-b"));
  EXPECT_FALSE(itm.match("--longer"));
  EXPECT_FALSE(called);
  itm.run("-a",nullptr);
  EXPECT_TRUE(called);
};

// ------------------------------------------------------------

class ArgParserTest : public testing::Test {
protected:
  int argc = 6;
  const char* argv[6];

  bool aVlu = false;
  std::string bVlu, longVlu = "nej", cVlu;

  void SetUp() override {
    argv[0] = "program";
    argv[1] = "-a";
    argv[2] = "--long";
    argv[3] = "-b";
    argv[4] = "reqVlu";
    argv[5] = "-c=vlu";
  }
};

TEST_F(ArgParserTest, parse) {
  ArgParser args {
    {"a",nullptr,"a desc",[&](bool vlu){aVlu=vlu;}},
    {"b",nullptr,"b desc", [&](std::string vlu){bVlu=vlu;}, ArgItem::ReqVluString},
    {nullptr,"long","long desc",[&](std::string vlu){longVlu=vlu;}},
    {"c",nullptr,"c desc",[&](std::string v){cVlu=v;}}
  };
  args.parse(argc, argv);
  EXPECT_TRUE(aVlu);
  EXPECT_EQ(bVlu, "reqVlu");
  EXPECT_EQ(longVlu, "");
};

TEST_F(ArgParserTest, parseUnrecognized) {
  ArgParser args {
    {"no","nothing","desc",[](bool v){(void)v;}}
  };
  EXPECT_EXIT(args.parse(argc,argv), testing::ExitedWithCode(1), "Unrecognized arg: -a");
};

TEST_F(ArgParserTest, help) {
  ArgParser args {
    {"a",nullptr,"a desc",[&](bool vlu){aVlu=vlu;}},
    {"b",nullptr,"b desc", [&](std::string vlu){bVlu=vlu;}, ArgItem::ReqVluString},
    {nullptr,"long","long desc",[&](std::string vlu){longVlu=vlu;}},
    {"c",nullptr,"c desc",[&](std::string v){cVlu=v;}}
  };
  testing::internal::CaptureStdout();
  args.help(" ");
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
  " -a\n \ta desc\n"
  " -b=<vlu>\n \tb desc\n"
  " --long[=<vlu>]\n \tlong desc\n"
  " -c[=<vlu>]\n \tc desc\n");
};
