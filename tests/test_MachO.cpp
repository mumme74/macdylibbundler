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
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include "Common.h"
#include "Types.h"
#include "MachO.h"



using ::testing::MatchesRegex;

namespace fs = std::filesystem;

// ----------------------------------------------------------------------

class MachOTest : public testing::Test
{
public:
  void SetUp() override {
    auto path = fs::path(__FILE__).parent_path() / "testdata" / "libicuio.73.dylib";
    file.open(path, std::ios::binary);
    ASSERT_FALSE(file.fail());
  }

  void TearDown() override {
    file.close();
  }

  std::ifstream file;
};

TEST_F(MachOTest, readHeader) {
  MachO::mach_object macho(file);
  EXPECT_FALSE(macho.failure());
  EXPECT_FALSE(file.bad());

  EXPECT_FALSE(macho.isBigEndian());
  auto type = macho.header64()->cputype();
  EXPECT_STREQ(MachO::CpuTypeStr(type), "MH_X86_64");
}

TEST_F(MachOTest, readLoadCmds) {
  MachO::mach_object macho(file);
  EXPECT_FALSE(file.bad());

  auto dylibs = macho.loadDylibPaths();
  EXPECT_EQ(dylibs.size(), 5);
  EXPECT_EQ(dylibs[0].string(), "@executable_path/../libs/libicuuc.73.dylib");
  EXPECT_EQ(dylibs[1].string(), "@executable_path/../libs/libicudata.73.dylib");
  EXPECT_EQ(dylibs[2].string(), "@executable_path/../libs/libicui18n.73.dylib");
  EXPECT_EQ(dylibs[3].string(), "/usr/lib/libSystem.B.dylib");
  EXPECT_EQ(dylibs[4].string(), "/usr/lib/libc++.1.dylib");

}

TEST_F(MachOTest, sections) {
  MachO::mach_object macho(file);
  EXPECT_FALSE(file.bad());
  auto segm = macho.dataSegments();
  EXPECT_EQ(segm.size(), 3);
  EXPECT_STREQ(segm.at(0)->segname(), "__TEXT");
  EXPECT_STREQ(segm.at(1)->segname(), "__DATA");
  EXPECT_STREQ(segm.at(2)->segname(), "__LINKEDIT");
}

TEST_F(MachOTest, readToEnd) {
  MachO::mach_object macho{file};
  EXPECT_FALSE(file.bad());
  auto pos = file.tellg();
  file.seekg(0, file.end);
  auto end = file.tellg();
  file.seekg(pos);
  EXPECT_FALSE(file.eof());
  file.get();
  EXPECT_TRUE(file.eof());
  EXPECT_EQ(pos, end);
}

// -----------------------------------------------------------

struct MachOWrite : ::testing::Test
{
  void SetUp() {
    auto tests = fs::path(__FILE__).parent_path();
    inPath = tests / "testdata/libicudata.73.dylib";
    outPath = tests / "__dump";
    infile.open(inPath, std::ios::binary);
    outfile.open(outPath, std::ios::binary);
  }

  void TearDown() {
    infile.close();
    outfile.close();
    chk.close();
    fs::remove(fs::path(__FILE__).parent_path()/"__dump");
  }

  bool noDiff() {
    FILE* f1 = fopen(inPath.c_str(),"r");
    FILE* f2 = fopen(outPath.c_str(),"r");
    const int N = 10000;
    char buf1[N];
    char buf2[N];
    bool res = false;

    do {
      size_t r1 = fread(buf1, 1, N, f1);
      size_t r2 = fread(buf2, 1, N, f2);

      if (r1 != r2 ||
          memcmp(buf1, buf2, r1))
      {
        goto out;  // Files are not equal
      }
    } while (!feof(f1) && !feof(f2));

    res = feof(f1) && feof(f2);

  out:
    fclose(f1);
    fclose(f2);
    return res;
  }

  std::ifstream infile;
  std::ofstream outfile;
  std::ifstream chk;
  fs::path inPath, outPath;
};

TEST_F(MachOWrite, write) {
  MachO::mach_object obj{infile};
  obj.write(outfile);
  EXPECT_FALSE(outfile.bad());
  EXPECT_TRUE(noDiff());
}

// ------------------------------------------------------------

TEST(MachoIOS, readTest) {
  std::ifstream file;
  file.open(fs::path(__FILE__).parent_path() / "testdata" / "AngryBirds2");
  MachO::mach_object macho{file};

  auto type = macho.header64()->cputype();
  EXPECT_STREQ(MachO::CpuTypeStr(type), "MH_ARM64");

  EXPECT_EQ(macho.is64bits(), true);
  auto segm = macho.dataSegments();
  EXPECT_EQ(segm.size(), 5);
}

// ------------------------------------------------------------

class FatMachO : public testing::Test
{
public:
  void SetUp() override {
    auto path = fs::path(__FILE__).parent_path() / "testdata" / "sublime_text";
    file.open(path, std::ios::binary);
    ASSERT_EQ(file.fail(), false);
  }

  void TearDown() override {
    file.close();
  }

  std::ifstream file;
};

TEST_F(FatMachO, readHeader) {
  MachO::mach_fat_object fat{file};
  EXPECT_FALSE(fat.failure());
  EXPECT_TRUE(fat.isBigEndian());
  const auto archs = fat.architectures();
  const auto& obj = fat.objects();
  EXPECT_EQ(archs.size(), 2);
  EXPECT_STREQ(MachO::CpuTypeStr(archs[0].cputype()), "MH_X86_64");
  EXPECT_FALSE(obj[0].isBigEndian());
  EXPECT_TRUE(obj[0].is64bits());
  EXPECT_TRUE(obj[0].hasBeenSigned());
  EXPECT_STREQ(MachO::CpuTypeStr(archs[1].cputype()), "MH_ARM64");
  EXPECT_FALSE(obj[1].isBigEndian());
  EXPECT_TRUE(obj[0].hasBeenSigned());
  EXPECT_TRUE(obj[1].is64bits());
}

// --------------------------------------------------------------

class MachOIntropect : public testing::Test
{
public:
  void SetUp()
  {
    auto path = fs::path(__FILE__).parent_path() / "testdata" / "classphoto";
    file.open(path, std::ios::binary);
    ASSERT_EQ(file.fail(), false);

    macho = std::make_unique<MachO::mach_object>(file);
    ASSERT_FALSE(macho->failure());
  }

  void TearDown()
  {
    file.close();
  }

  std::unique_ptr<MachO::mach_object> macho;
  std::ifstream file;
};

TEST_F(MachOIntropect, loadCmds) {
  MachO::introspect_object insp(macho.get());
  EXPECT_THAT(insp.loadCmds(),
    testing::ContainsRegex("cmd LC_LOAD_DYLIB"));
}
