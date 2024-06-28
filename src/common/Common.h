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

#ifndef COMMON_H
#define COMMON_H

// this lib should not depend on anything in any other lib
// except libc++
#include <string>
#include <system_error>
#include <vector>


/// Strip the first dir
/// ie ./name -> name,
///    \@rpath/dir/name -> dir/name
std::string stripPrefix(std::string_view in);
/// Strip the last '/'
std::string stripLastSlash(std::string_view in);
/// Trim trailing whitespace
std::string rtrim(std::string_view in);
/// split str on delimiters into vector<string>
std::vector<std::string> tokenize(
  std::string_view str,
  std::string_view delimiters);

// print msg to stderr and exit
void exitMsg(std::string_view msg,
             std::error_code err = std::error_code());

#endif // COMMON_H
