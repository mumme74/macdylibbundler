/*
The MIT License (MIT)

Copyright (c) 2014 Marianne Gagnon

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


#ifndef _utils_h_
#define _utils_h_

#include <string>
#include <vector>
#include <system_error>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <array>
#include "Types.h"

class Library;

void setWritable(PathRef file);
void copyFile(PathRef from, PathRef to);

/// executes a command in the native shell and returns output in string
std::string system_get_output(std::string_view cmd);

/// like 'system', runs a command on the system shell, but also prints the command to stdout.
int systemp(std::string_view cmd);
Path getUserInputDirForFile(PathRef file);

/// try to create a folder
void createFolder(PathRef folder);

/// sign `file` with an ad-hoc code signature: required for ARM (Apple Silicon) binaries
void adhocCodeSign(PathRef file);

/// checks if file is executable
bool isExecutable(PathRef path);



#endif // _utils_h_
