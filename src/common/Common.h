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
std::string stripPrefix(const std::string& in);
/// Strip the last '/'
std::string stripLastSlash(const std::string& in);
/// Trim trailing whitespace
std::string rtrim(const std::string &in);

void tokenize(const std::string& str, const char* delimiters,
              std::vector<std::string>*);

// print msg to stderr and exit
void exitMsg(const std::string& msg,
                 std::error_code err = std::error_code(),
                 int exitCode = 1);

#endif // COMMON_H
