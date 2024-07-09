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


#include <algorithm>
#include <filesystem>
#include "Types.h"
#include "Tools.h"


using ::testing::MatchesRegex;

namespace fs = std::filesystem;

// -----------------------------------------------------------------

struct SystemFnMock {
  std::function<int (const char*)> create() {
    auto fn = [&](const char* cmd)->int {
      calls.emplace_back(cmd);
      return retVlu;
    };
    return fn;
  }
  std::vector<std::string> calls;
  int retVlu = 0;
};

TEST(Tools_InstallName, verbose) {
  testing::internal::CaptureStdout();
  Tools::InstallName test("test", true);
  SystemFnMock mock;
  test.testingSystemFn(mock.create());
  EXPECT_EQ(mock.calls.size(), 0);
  test.id(Path("newId"), Path("toBin"));
  EXPECT_EQ(mock.calls[0], "test -id \"\"newId\"\" \"\"toBin\"\"");
  EXPECT_EQ(
    testing::internal::GetCapturedStdout(),
    "    test -id \"\"newId\"\" \"\"toBin\"\"\n");
}

TEST(Tools_InstallName, nonVerbose) {
  testing::internal::CaptureStdout();
  Tools::InstallName test("test", false);
  SystemFnMock mock;
  test.testingSystemFn(mock.create());

  EXPECT_EQ(mock.calls.size(), 0);
  test.id(Path("newId"), Path("toBin"));
  EXPECT_THAT(mock.calls[0],
    MatchesRegex("test -id \"+newId\"+ \"+toBin\"+.*"));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(Tools_InstallNameDeath, failResponse) {
  Tools::InstallName test("test", false);
  SystemFnMock mock;
  mock.retVlu = 1;
  test.testingSystemFn(mock.create());
  EXPECT_DEATH(test.id(Path("newId"), Path("toBin")),"Error: .* id");
}

TEST(Tools_InstallName, add_rpath) {
  testing::internal::CaptureStdout();
  Tools::InstallName test("test", false);
  SystemFnMock mock;
  test.testingSystemFn(mock.create());

  EXPECT_EQ(mock.calls.size(), 0);

  test.add_rpath(Path("new"), Path("toBin"));
  EXPECT_EQ(mock.calls[0], "test -add_rpath \"\"new\"\" \"\"toBin\"\"");
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(Tools_InstallName, delete_rpath) {
  testing::internal::CaptureStdout();
  Tools::InstallName test("test", false);
  SystemFnMock mock;
  test.testingSystemFn(mock.create());

  EXPECT_EQ(mock.calls.size(), 0);

  test.delete_rpath(Path("rpath"), Path("toBin"));
  EXPECT_EQ(mock.calls[0], "test -delete_rpath \"\"rpath\"\" \"\"toBin\"\"");
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(Tools_InstallName, change) {
  testing::internal::CaptureStdout();
  Tools::InstallName test("test", false);
  SystemFnMock mock;
  test.testingSystemFn(mock.create());

  EXPECT_EQ(mock.calls.size(), 0);

  test.change(Path("old"), Path("new"), Path("toBin"));
  EXPECT_EQ(mock.calls[0], "test -change \"\"old\"\" \"\"new\"\" \"\"toBin\"\"");
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(Tools_InstallName, id) {
  testing::internal::CaptureStdout();
  Tools::InstallName test("test", false);
  SystemFnMock mock;
  test.testingSystemFn(mock.create());

  EXPECT_EQ(mock.calls.size(), 0);

  test.id(Path("new"), Path("toBin"));
  EXPECT_EQ(mock.calls[0], "test -id \"\"new\"\" \"\"toBin\"\"");
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(Tools_InstallName, Defaults) {
  Tools::InstallName tool;
  EXPECT_EQ(tool.cmd(), "");
  EXPECT_EQ(tool.verbose(), false);
}

TEST(Tools_InstallName, CustomDefaults) {
  Tools::InstallName::initDefaults("test_cmd", true);
  Tools::InstallName tool;
  EXPECT_EQ(tool.cmd(), "test_cmd");
  EXPECT_EQ(tool.verbose(), true);
}

// -----------------------------------------------------

struct POpenFnMock {
  ~POpenFnMock() {
    if (fp)
      fclose(fp);
  }
  std::function<FILE* (const char*, const char*)> createOpen() {
    fp = fopen(filename.c_str(), "r");
    auto fn = [&](const char* cmd, const char* mode)->FILE* {
      calls.emplace_back(std::pair<std::string, std::string>{cmd,mode});
      if (!fp)
        std::cerr << "Failed to open mockfile "
                  << errno << " " << strerror(errno) << "\n";
      return fp;
    };
    return fn;
  }
  std::function<int (FILE*)> createClose() {
    auto fn = [&](FILE* fileP){
      EXPECT_EQ(fileno(fileP), fileno(fp));
      fclose(fp);
      fp = nullptr;
      return retVlu;
    };
    return fn;
  }
  fs::path filename = Path(__FILE__).parent_path()
                    / "testdata/testdata_common_otooltest.txt";
  std::vector<std::pair<std::string, std::string>> calls;
  FILE* fp = nullptr;
  int retVlu = 0;
};

TEST(Tools_OTool, errorWarning) {
  testing::internal::CaptureStderr();
  POpenFnMock mock;
  Tools::OTool tool("testOtool", false);
  tool.testingPopenFn(mock.createOpen(), mock.createClose());
  tool.scanBinary(Path("somefile.dylib"));
  EXPECT_EQ(mock.calls.size(), 0);
  EXPECT_THAT(
    testing::internal::GetCapturedStderr(),
    MatchesRegex(".* WARNING .*"));
}

TEST(Tools_OTool, nonVerbose) {
  testing::internal::CaptureStdout();
  POpenFnMock mock;
  Tools::OTool tool("testOtool", false);
  tool.testingPopenFn(mock.createOpen(), mock.createClose());
  tool.scanBinary(Path(mock.filename));
  EXPECT_EQ(mock.calls.size(), 1);
  EXPECT_EQ(mock.calls[0].first,
    std::string("testOtool -l \"\"") + mock.filename.string() + "\"\"");
  EXPECT_EQ(mock.calls[0].second, "r");
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

TEST(Tools_OTool, verbose) {
  testing::internal::CaptureStdout();
  POpenFnMock mock;
  Tools::OTool tool("testOtool", true);
  tool.testingPopenFn(mock.createOpen(), mock.createClose());
  tool.scanBinary(Path(mock.filename));
  EXPECT_EQ(mock.calls.size(), 1);
  EXPECT_EQ(mock.calls[0].first,
    std::string("testOtool -l \"\"") + mock.filename.string() + "\"\"");
  EXPECT_EQ(mock.calls[0].second, "r");
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
    std::string("    testOtool -l \"\"") + mock.filename.string() + "\"\"\n");
}

TEST(Tools_OTool, paths) {
  POpenFnMock mock;
  Tools::OTool tool("testOtool", false);
  tool.testingPopenFn(mock.createOpen(), mock.createClose());
  tool.scanBinary(Path(mock.filename));
  EXPECT_EQ(mock.calls.size(), 1);
  EXPECT_EQ(tool.dependencies.size(), 16);
  EXPECT_EQ(tool.rpaths.size(), 1);
  EXPECT_EQ(tool.rpaths[0].string(), "/x86_64/usr/qt6/lib");
  EXPECT_EQ(tool.dependencies[0].string(), "@rpath/QtXml.framework/Versions/A/QtXml");
  EXPECT_EQ(tool.dependencies[9].string(), "@rpath/QtCore.framework/Versions/A/QtCore");
  EXPECT_EQ(tool.dependencies[15].string(), "/usr/lib/libSystem.B.dylib");
}

