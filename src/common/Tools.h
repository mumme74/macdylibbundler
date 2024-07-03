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

#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <sstream>
#include <functional>
#include "Types.h"

namespace Tools {
class Base
{
public:
  Base(std::string_view name, bool verbose);
  virtual ~Base();

  virtual std::string_view name() const { return m_name; }
  bool verbose() const { return m_verbose; }

  /// replace stdlib system fun with this
  /// for testing purpose
  void testingSystemFn(std::function<int (const char*)> systemFn);

protected:
  int systemPrint(std::string_view cmd) const;

  bool m_verbose;

  std::string m_name;

  std::function<int (const char*)> m_systemFn;
};

class InstallName : public Base
{
public:
  static void initDefaults(std::string_view cmd, bool verbose);
  InstallName();
  InstallName(std::string_view cmd, bool verbose);
  /// Add a new rpath
  void add_rpath(PathRef rpath, PathRef bin) const;
  /// Delete specified rpath
  void delete_rpath(PathRef rpath, PathRef bin) const;
  /// Change dependent shared library install name
  void change(PathRef oldPath, PathRef newPath, PathRef bin) const;
  /// Change dynamic library id
  void id(PathRef id, PathRef bin) const;
  /// Change rpath path name
  void rpath(PathRef from, PathRef to, PathRef bin) const;
private:
  static std::string defaultCmd;
  static bool defaultVerbosity;
};

} // namespace Tools

#endif
