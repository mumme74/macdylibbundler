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
#include "MachO.h"

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
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf)-1, fp)) != 0) {
    buf[n] = 0;
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

std::string InstallName::defaultCmd;// = "install_name_tool";
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
  if (!m_cmd.empty()) {
    addRPathExternal(rpath, bin);
  } else {
    MachO::MachOLoader loader(bin);

    auto change = [&](MachO::mach_object& obj)->void {
      if (obj.addRPath(rpath))
        exitMsg(std::string("Could not add rpath on ") + bin.string());
    };

    auto write = [&]()->void {
      if (!loader.write(bin, true))
        exitMsg(std::string("Could not add rpath on ") + bin.string());
    };

    if (loader.isFat()) {
      for (auto& slice : loader.fatObject()->objects())
        change(slice);
      write();

    } else if (loader.isObject()) {
      change(*loader.object());
      write();
    } else {
      exitMsg(std::string("Failed to open ") + bin.string() +
              " not a mach-o object\n");
    }
  }
}

void
InstallName::addRPathExternal(PathRef rpath, PathRef bin) const
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
  if (!m_cmd.empty()) {
    deleteRpathExternal(rpath, bin);
  } else {
    MachO::MachOLoader loader(bin);

    auto change = [&](MachO::mach_object& obj)->void {
      if (obj.removeRPath(rpath))
        exitMsg(std::string("Could not delete_rpath on ") + bin.string());
    };

    auto write = [&]()->void {
      if (!loader.write(bin, true))
        exitMsg(std::string("Could not delete_rpath on ") + bin.string());
    };

    if (loader.isFat()) {
      for (auto& slice : loader.fatObject()->objects())
        change(slice);
      write();

    } else if (loader.isObject()) {
      change(*loader.object());
      write();
    } else {
      exitMsg(std::string("Failed to open ") + bin.string() +
              " not a mach-o object\n");
    }
  }
}


void
InstallName::deleteRpathExternal(PathRef rpath, PathRef bin) const
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
  if (!m_cmd.empty()) {
    changeExternal(oldPath, newPath, bin);

  } else {
    MachO::MachOLoader loader(bin);

    auto change = [&](MachO::mach_object& obj)->void {
      if (obj.changeDylibPaths(oldPath, newPath))
        exitMsg(std::string("Could not change lib path on ") + bin.string());
    };

    auto write = [&]()->void {
      if (!loader.write(bin, true))
        exitMsg(std::string("Could not change lib path on ") + bin.string());
    };

    if (loader.isFat()) {
      for (auto& slice : loader.fatObject()->objects())
        change(slice);
      write();

    } else if (loader.isObject()) {
      change(*loader.object());
      write();
    } else {
      exitMsg(std::string("Failed to open ") + bin.string() +
              " not a mach-o object\n");
    }
  }
}

void
InstallName::changeExternal(
  PathRef oldPath, PathRef newPath, PathRef bin
) const
{
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
  if (!m_cmd.empty()) {
    idExternal(id, bin);
  } else {
     if (!m_cmd.empty()) {
      idExternal(id, bin);
    } else {
      MachO::MachOLoader loader(bin);

      auto change = [&](MachO::mach_object& obj)->void {
        if (obj.changeId(id))
          exitMsg(std::string("Could not change id on ") + bin.string());
      };

      auto write = [&]()->void {
        if (!loader.write(bin, true))
          exitMsg(std::string("Could not change id on ") + bin.string());
      };

      if (loader.isFat()) {
        for (auto& slice : loader.fatObject()->objects())
          change(slice);
        write();

      } else if (loader.isObject()) {
        change(*loader.object());
        write();
      } else {
        exitMsg(std::string("Failed to open ") + bin.string() +
                " not a mach-o object\n");
      }
    }
  }
}

void
InstallName::idExternal(PathRef id, PathRef bin) const
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
  if (!m_cmd.empty()) {
    rpathExternal(from, to, bin);
  } else {
    MachO::MachOLoader loader(bin);

    auto change = [&](MachO::mach_object& obj)->void {
      if (obj.changeRPath(from, to))
        exitMsg(std::string("Could not change rpath on ") + bin.string());
    };

    auto write = [&]()->void {
      if (!loader.write(bin, true))
        exitMsg(std::string("Could not change rpath on ") + bin.string());
    };

    if (loader.isFat()) {
      for (auto& slice : loader.fatObject()->objects())
        change(slice);
      write();

    } else if (loader.isObject()) {
      change(*loader.object());
      write();
    } else {
      exitMsg(std::string("Failed to open ") + bin.string() +
              " not a mach-o object\n");
    }
  }
}

void
InstallName::rpathExternal(PathRef from, PathRef to, PathRef bin) const
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

std::string OTool::defaultCmd; // = "otool";
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

  if (!m_cmd.empty())
    return scanBinaryExternal(bin);

  // using build in macho lib
  auto getFromObj = [&](MachO::mach_object& obj)->void {
    std::vector<MachO::LoadCmds> filterIn {
      MachO::LC_LOAD_DYLIB,
      MachO::LC_REEXPORT_DYLIB,
      MachO::LC_LOAD_WEAK_DYLIB
    };

    for (const MachO::load_command_bytes* cmd : obj.filterCmds(filterIn)) {
      auto bytes = cmd->bytes.get();
      MachO::dylib_command dylib{*cmd, obj};
      dependencies.emplace_back(
        dylib.name().str(bytes));
    }

    for (const auto& path : obj.rpaths())
      rpaths.emplace_back(Path(path));
  };

  MachO::MachOLoader loader(bin);
  if (loader.isFat()) {
    for (auto& slice : loader.fatObject()->objects())
      getFromObj(slice);

  } else if (loader.isObject()) {
    getFromObj(*loader.object());

  } else {
    std::cerr << "Failed to open " << bin << " not a mach-o object\n";
    exit(2);
  }
  return true;
}

bool
OTool::scanBinaryExternal(PathRef bin)
{
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
        rpaths.emplace_back(Path(sm[1].str()));
        st = Search;
      }
    } break;
    case Dep: {
      std::smatch sm;
      if (std::regex_search(line, sm, reDep)) {
        dependencies.emplace_back(Path(sm[1].str()));
        st = Search;
      }
    } break;
    }
  }

  return true;
}

// ----------------------------------------------------

Codesign::Codesign(
  std::string_view cmd, bool verbose,
  std::string cmdLineOptions
) : Base(cmd, verbose)
  , m_cmdLineOptions{
      cmdLineOptions.size() ? cmdLineOptions : defaultCmdLineOptions}
{}

Codesign::Codesign()
  : Base(defaultCmd, defaultVerbosity)
{}

void
Codesign::initDefaults(
    std::string_view cmd, bool verbose,
    std::string cmdLineOptions)
{
  if (cmdLineOptions.size())
    defaultCmdLineOptions = cmdLineOptions;
  defaultCmd = cmd;
  defaultVerbosity = verbose;
}

std::string Codesign::defaultCmdLineOptions =
  "--force --deep --preserve-metadata=entitlements"
  ",requirements,flags,runtime --sign - ";
std::string Codesign::defaultCmd = "codesign";
bool Codesign::defaultVerbosity = false;

bool
Codesign::sign(PathRef bin) const
{
  if (m_verbose)
      std::cout << "Signing '" << bin << "'" << std::endl;

  std::stringstream signCmd;
  signCmd << m_cmd << " " << m_cmdLineOptions << " " << bin;
  auto signCommand = signCmd.str();

  if (systemPrint(signCommand) != 0 ) {
   /* // If the codesigning fails, it may be a bug in Apple's codesign utility.
    // A known workaround is to copy the file to another inode, then move it back
    // erasing the previous file. Then sign again.
    std::cerr << "  * Error : An error occurred while applying "
              << "ad-hoc signature to " << bin
              << ". Attempting workaround" << std::endl;

    //auto machine = system_get_output("machine");
    //bool isArm = machine.find("arm") != std::string::npos;
    bool isArm = false; // TODO Implement
    auto tempDirTemplate = std::string(std::getenv("TMPDIR") +
                            std::string("dylibbundler.XXXXXXXX"));
    std::string filename = bin.filename().string();
    char* tmpDirCstr = mkdtemp((char *)(tempDirTemplate.c_str()));
    if( tmpDirCstr == NULL )
    {
      std::cerr << "  * Error : Unable to create temp "
                << "directory for signing workaround"
                << std::endl;
      if( isArm )
      {
          exit(1);
      }
    }
    std::string tmpDir = std::string(tmpDirCstr);
    std::string tmpFile = tmpDir+"/"+filename;
    const auto runCommand = [&](const std::string& command, const std::string& errMsg)
    {
      if( systemPrint( command ) != 0 )
      {
          std::cerr << errMsg << std::endl;
          if( isArm )
          {
              exit(1);
          }
      }
    };
    std::stringstream err, cmd;
    cmd << "cp -p \"" << bin << "\" \"" << tmpFile << "\"";
    err << "  * Error : An error occurred copying " << bin
        << " to " << tmpDir;
    runCommand(cmd.str(), err.str());

    cmd << "mv -f \"" << tmpFile << "\" \"" << bin << "\"";
    err << "  * Error : An error occurred moving " << tmpFile
        << " to " << bin;
    runCommand(cmd.str(), err.str());

    cmd << "rm -rf \"" << tmpDir << "\"";
    systemPrint(cmd.str());
    err << "  * Error : An error occurred while applying "
        << "ad-hoc signature to " << bin;
    runCommand(signCommand, err.str());*/

    return true;
  }
  return false;
}


