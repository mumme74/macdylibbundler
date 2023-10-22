/*
The MIT License (MIT)

Copyright (c) 2014 Marianne Gagnon
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

// this lib should not depend on anything in any other lib
// except libc++
#include <algorithm>
#include <iostream>
#include "Common.h"


std::vector<std::string>
tokenize(const std::string& str, const std::string& delimiters)
{
  std::vector<std::string> tokens;

  // skip delimiters at beginning.
  auto lastPos = str.find_first_not_of( delimiters , 0);

  // find first "non-delimiter".
  auto pos = str.find_first_of(delimiters, lastPos);

  while (str.npos != pos || str.npos != lastPos)
  {
      // found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));

      // skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(delimiters, pos);

      // find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
  }
  return tokens;
}

std::string stripPrefix(const std::string& in)
{
  auto idx = in.rfind("/");
  if (idx != in.npos)
      return in.substr(idx+1);
  return in;
}

std::string stripLastSlash(const std::string& in)
{
  return rtrim(in).substr(0, in.rfind("/"));
}

std::string rtrim(const std::string &in) {
  std::string s = in;
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
  return s;
}


void exitMsg(const std::string& msg,
                 std::error_code err /* = std::error_code()*/)
{
  std::cerr << msg;
  int code = 1;
  if (err) {
    std::cerr << " " << err.message();
    code = err.value();
  }
  std::cerr << std::endl;
  exit(code);
}