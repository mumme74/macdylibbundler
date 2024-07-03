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
#include "Tools.h"
#include "Common.h"

using namespace Tools;


// --------------------------------------------------

Base::Base(std::string_view name, bool verbose)
  : m_verbose{verbose}
  , m_name{name.data()}
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

void
Base::testingSystemFn(std::function<int (const char*)> systemFn)
{
  m_systemFn = systemFn;
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
    ss << m_name << " -add_rpath \""<< rpath << "\" "
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
    ss << m_name << " -delete_rpath \""<< rpath << "\" "
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
    ss << m_name << " -change \""
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
    ss << m_name << " -id \"" << id << "\" "
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
    ss << m_name << " -rpath \""<< from << "\" "
       << "\"" << to << "\" \"" << bin << "\"";

    if(systemPrint(ss.str()) != 0) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << bin << " when changing rpath.\n";
        exitMsg(ss.str());
    }
}