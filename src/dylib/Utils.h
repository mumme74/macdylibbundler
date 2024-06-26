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

class Library;

bool fileExists(const std::string& filename);

void copyFile(const std::string& from, const std::string& to);

/// executes a command in the native shell and returns output in string
std::string system_get_output(const std::string& cmd);

/// like 'system', runs a command on the system shell, but also prints the command to stdout.
int systemp(const std::string& cmd);
void changeInstallName(const std::string& binary_file, const std::string& old_name, const std::string& new_name);
std::string getUserInputDirForFile(const std::string& filename);

/// sign `file` with an ad-hoc code signature: required for ARM (Apple Silicon) binaries
void adhocCodeSign(const std::string& file);

/// checks if file is executable
bool isExecutable(std::filesystem::path path);


/// @brief  convert an uint from native to bigendian and reverse again
typedef union _bigendian  {
private:
    static constexpr uint32_t uint32_ = 0x01020304;
    static constexpr uint8_t magic_ = (const uint8_t&)uint32_;
    static constexpr bool little = magic_ == 0x04;
    static constexpr bool big = magic_ == 0x01;
    static_assert(little || big, "Cannot determine endianness!");
    static void conv(uint8_t* uTo, uint8_t* uFrom, size_t sz);
public:
    static constexpr bool middle = magic_ == 0x02;
    _bigendian(uint16_t vlu);
    _bigendian(uint32_t vlu);
    _bigendian(uint64_t vlu);

    /// @brief bytes in bigendian order
    uint8_t u16arr[2];
    uint8_t u32arr[4];
    uint8_t u64arr[8];
    /// @brief go back to native again
    uint16_t u16native();
    uint32_t u32native();
    uint64_t u64native();

} bigendian_t;

#endif // _utils_h_
