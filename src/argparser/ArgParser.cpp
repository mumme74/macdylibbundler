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

#include "ArgParser.h"
#include "Common.h"
#include <cstring>
#include <string>
#include <iostream>
#include <cassert>


ArgItem::ArgItem(
  CSTR shSwitch, CSTR longSwitch, CSTR description,
  std::function<void()> callback
) :
  m_short(shSwitch), m_long(longSwitch),
  m_description(description),
  m_callbackVoid(callback),
  m_callbackStr(nullptr),
  m_callbackBool(nullptr),
  m_option(VoidArg)
{
}

ArgItem::ArgItem(
  CSTR shSwitch, CSTR longSwitch, CSTR description,
  std::function<void(std::string)> callback, Options option
) :
  m_short(shSwitch), m_long(longSwitch),
  m_description(description),
  m_callbackVoid(nullptr),
  m_callbackStr(callback),
  m_callbackBool(nullptr),
  m_option(option)
{
  assert(option == OptVluString || option == ReqVluString);
}

ArgItem::ArgItem(
  CSTR shSwitch, CSTR longSwitch, CSTR description,
  std::function<void(bool)> callback, Options option
) :
  m_short(shSwitch), m_long(longSwitch),
  m_description(description),
  m_callbackVoid(nullptr),
  m_callbackStr(nullptr),
  m_callbackBool(callback),
  m_option(option)
{
  assert(option == VluFalse || option == VluTrue);
}

bool
ArgItem::match(CSTR current) const
{
  if (!current || *current == '\0') return false;
  if (*current == '-') {
    current++;
    const char* needle = m_short;
    if (*current=='-') {
      if (!m_long) return false;
      needle = m_long;
      current++;
    }
    if (!needle) return false;
    return std::strncmp(current, needle, 100)==0;
  }
  return false;
}

int
ArgItem::run(CSTR current, CSTR next) const
{
  if (match(current)) {
    switch (m_option) {
    case VoidArg:
      assert(m_callbackVoid);
      m_callbackVoid(); return 1;
    case VluTrue:
      assert(m_callbackBool);
      m_callbackBool(true); return 1;
    case VluFalse:
      assert(m_callbackBool);
      m_callbackBool(false); return 1;
    case OptVluString:
      assert(m_callbackStr);
      if (next == nullptr) {
        m_callbackStr(std::string()); return 1;
      }
      [[fallthrough]];
    default:
      if (next == nullptr || *next == '-')
        exitMsg(std::string(m_long ? m_long : m_short) +
                " requires a value");

      assert(m_callbackStr);
      m_callbackStr(next);
      return 2;
    }
  }
  return 0;
}

void
ArgItem::help(CSTR indent) const
{
  std::string vlu;
  if (m_option == ReqVluString)
    vlu += "=<vlu>";
  else if (m_option == OptVluString)
    vlu += "[=<vlu>]";

  std::cout << indent;
  if (m_short) {
    std::cout << "-" << m_short << vlu;
    if (m_long) std::cout << ", ";
  }
  if (m_long) std::cout << "--" << m_long << vlu;
  std::cout << "\n" << indent << "\t";
  std::cout << m_description << std::endl;
}

// ----------------------------------------------------------

ArgParser::ArgParser(std::initializer_list<ArgItem> items) :
  m_items(items),
  m_programName(nullptr),
  m_programPath(nullptr)
{
}


// parse --switch=vlu, split on '='
void splitOnEq(const char *argv, char first[], char second[], size_t SZ) {
  char *buf = first;
  for (const char *p = argv;
      *p != '\0' && p-SZ < argv; // max 100 chars
      ++p)
  {
    if (*p == '=')
      buf = second;
    else {
      *buf = *p;
      ++buf;
    }
  }
}

void
ArgParser::parse(int argc, const char* argv[])
{
  m_programPath = argv[0];
  m_programName = strrchr(argv[0], '/');
  if (m_programName) m_programName++;
  else m_programName = argv[0];

  for(int i = 1; i < argc; ++i) {
    bool found = false;
    for(const auto& itm : m_items) {
      const size_t SZ = 100;
      char first[SZ] = {}, second[SZ] = {};
      splitOnEq(argv[i], first, second, SZ);

      const char *next = *second != '\0' ? second :
                    i+1 < argc && *argv[i+1]!='-' ? argv[i+1] : nullptr;

      int tookArgs = itm.run(first, next);
      if (tookArgs > 0) {
        found = true;
        // if we have -a=vlu they are one itm
        // also -1 because it increments on next loop
        i += next==second ? 0 : tookArgs -1;
        break;
      }
    }
    if (!found)
      exitMsg(std::string("Unrecognized arg: ") + argv[i]);
  }
}

void
ArgParser::help(const char* indent) const
{
  for(const auto& itm : m_items) {
    itm.help(indent);
  }
}
