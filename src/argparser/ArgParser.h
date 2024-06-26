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

#ifndef SRC_ARGPARSER_ARGPARSER_H_
#define SRC_ARGPARSER_ARGPARSER_H_

#include <vector>
#include <string>
#include <initializer_list>
#include <functional>

class ArgItem {
 public:
  typedef const char* CSTR;
  enum Options { VoidArg, VluTrue, VluFalse, OptVluString, ReqVluString };

  ArgItem(
    CSTR shSwitch, CSTR longSwitch, CSTR description,
    std::function<void()> callback);

  ArgItem(
    CSTR shSwitch, CSTR longSwitch, CSTR description,
    std::function<void(std::string value)> callback,
    Options option = OptVluString);

  ArgItem(
    CSTR shSwitch, CSTR longSwitch, CSTR description,
    std::function<void(bool value)> callback,
    Options option = VluTrue);

  bool match(CSTR current) const;
  int run(CSTR current, CSTR next) const;
  void help(CSTR indent = "") const;

  CSTR shSwitch() const { return m_short; }
  CSTR longSwitch() const { return m_long; }
  CSTR description() const { return m_description; }
  Options option() const { return m_option; }

 private:
  CSTR m_short;
  CSTR m_long;
  CSTR m_description;
  std::function<void()> m_callbackVoid;
  std::function<void(std::string)> m_callbackStr;
  std::function<void(bool)> m_callbackBool;
  Options m_option;
};

class ArgParser {
  std::vector<ArgItem> m_items;
  const char* m_programName, *m_programPath;
 public:
  ArgParser(std::initializer_list<ArgItem>);
  void parse(int argc, const char* argv[]);
  void help(const char* indent = "") const;
  const char* programName() const { return m_programName; }
};

#endif  // SRC_ARGPARSER_ARGPARSER_H_
