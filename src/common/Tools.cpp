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

#include <iostream>
#include <stdlib.h>
#include <filesystem>
#include <regex>
#if defined(_WIN32) && !defined(popen)
# define popen = _popen;
# define pclose = _pclose;
#endif
#include "Tools.h"
#include "Common.h"

using namespace Tools;
namespace fs = std::filesystem;


// --------------------------------------------------

Base::Base(std::string_view name, bool verbose)
  : m_verbose{verbose}
  , m_cmd{name.data()}
  , m_systemFn{system}
  , m_popenFn{popen}
  , m_pcloseFn{pclose}
{}

Base::~Base()
{}

int
Base::systemPrint(std::string_view cmd) const
{
    if (m_verbose)
        std::cout << "    " << cmd << std::endl;
    return m_systemFn(cmd.data());
}

std::stringstream
Base::runAndGetOutput(
  std::string_view cmd, int& exitCode
) const {
  std::stringstream ss;

  if (m_verbose)
      std::cout << "    " << cmd << std::endl;

  FILE* fp = m_popenFn(cmd.data(), "r");
  if (fp == nullptr){ //|| errno == ECHILD) {
    ss << "*Failed to run popen(..) using " << cmd << "\n";
    exitMsg(ss.str());
  }

  char buf[2048] = {0};
  size_t n, read =0;
  while ((n = fread(buf, 1, sizeof(buf)-1, fp)) != 0) {
    buf[n] = 0;
    read += n;
    ss << buf;
  }

  auto end = feof(fp);
  exitCode = m_pcloseFn(fp);

  if (end == 0) {
    std::cout << ss.str() << "\n";

    std::stringstream msg;
    msg << "*Failed to read all data from popen"
        << " using " << cmd << "\n";
    exitMsg(msg.str());
  }

  return ss;
}

void
Base::testingSystemFn(std::function<int (const char*)> systemFn)
{
  m_systemFn = systemFn;
}

void
Base::testingPopenFn(
  std::function<FILE* (const char*, const char*)> popenFn,
  std::function<int (FILE*)> pcloseFn
) {
  m_popenFn = popenFn;
  m_pcloseFn = pcloseFn;
}

// ---------------------------------------------------

InstallName::InstallName()
  : Base(InstallName::defaultCmd, InstallName::defaultVerbosity)
{}

std::string InstallName::defaultCmd = "install_name_tool";
bool InstallName::defaultVerbosity = false;

InstallName::InstallName(std::string_view cmd, bool verbose)
  : Base{cmd, verbose}
{}

void
InstallName::initDefaults(
  std::string_view cmd, bool verbose)
{
  InstallName::defaultCmd = cmd;
  InstallName::defaultVerbosity = verbose;
}

/// Add a new rpath
void
InstallName::add_rpath(PathRef rpath, PathRef bin) const
{
    std::stringstream ss;
    ss << m_cmd << " -add_rpath \""<< rpath << "\" "
       << "\"" << bin << "\"";

    if(systemPrint(ss.str()) != 0) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << bin << " when add_rpath.\n";
        exitMsg(ss.str());
    }
}

/// Delete specified rpath
void
InstallName::delete_rpath(PathRef rpath, PathRef bin) const
{
    std::stringstream ss;
    ss << m_cmd << " -delete_rpath \""<< rpath << "\" "
       << "\"" << bin << "\"";

    if(systemPrint(ss.str()) != 0) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << bin << " when delete_rpath.\n";
        exitMsg(ss.str());
    }
}


/// Change dependent shared library install name
void
InstallName::change(
  PathRef oldPath, PathRef newPath, PathRef bin
) const {
    std::stringstream ss;
    ss << m_cmd << " -change \""
       << oldPath << "\" \"" << newPath << "\" \""
       << bin << "\"";

    if(systemPrint(ss.str()) != 0) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << bin
           << " when change install name\n";
        exitMsg(ss.str());
    }
}

/// Change dynamic library id
void
InstallName::id(PathRef id, PathRef bin) const
{
    std::stringstream ss;
    ss << m_cmd << " -id \"" << id << "\" "
       << "\"" << bin << "\"";

    if(systemPrint(ss.str()) != 0) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << bin << " when change binary id\n";
        exitMsg(ss.str());
    }
}

/// Change rpath path name
void
InstallName::rpath(PathRef from, PathRef to, PathRef bin) const
{
    std::stringstream ss;
    ss << m_cmd << " -rpath \""<< from << "\" "
       << "\"" << to << "\" \"" << bin << "\"";

    if(systemPrint(ss.str()) != 0) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << bin << " when changing rpath.\n";
        exitMsg(ss.str());
    }
}

// -----------------------------------------------------

OTool::OTool()
  : Base(OTool::defaultCmd, OTool::defaultVerbosity)
{}

OTool::OTool(std::string_view cmd, bool verbose)
  : Base(cmd, verbose)
{}

std::string OTool::defaultCmd = "otool";
bool OTool::defaultVerbosity = false;

void
OTool::initDefaults(std::string_view cmd, bool verbose)
{
  OTool::defaultCmd = cmd;
  OTool::defaultVerbosity = verbose;
}

bool
OTool::scanBinary(PathRef bin)
{
  if (!fs::exists(bin))
  {
      std::cerr << "\n/!\\ WARNING : can't scan a "
                << "nonexistent file '" << bin << "'\n";
      return false;
  }

  std::stringstream cmd;
  cmd << m_cmd << " -l \"" << bin << "\"";

  int status;

  std::string c = cmd.str();
  auto resStream = runAndGetOutput(cmd.str(), status);

  std::regex reRath("^\\s+path\\s+(.*)\\s+\\(");
  std::regex reDep("^\\s+name\\s+(.*)\\s+\\(");
  std::regex reSearch("\\s+cmd\\s+(\\w*)");

  enum state { Search, RPath, Dep };
  state st = Search;
  std::string line;
  while (std::getline(resStream, line)) {
    switch (st) {
    case Search: {
      std::smatch sm;
      if (std::regex_search(line, sm, reSearch)) {
        if (sm[1] == "LC_RPATH") st = RPath;
        else if (sm[1] == "LC_LOAD_DYLIB" ||
                 sm[1] == "LC_REEXPORT_DYLIB"
        ) {
          st = Dep;
        }
      }
    } break;
    case RPath: {
      std::smatch sm;
      if (std::regex_search(line, sm, reRath)) {
        rpaths.emplace_back(sm[1]);
        st = Search;
      }
    } break;
    case Dep: {
      std::smatch sm;
      if (std::regex_search(line, sm, reDep)) {
        dependencies.emplace_back(sm[1]);
        st = Search;
      }
    } break;
    }
  }

  return true;
}


