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


#ifndef _depend_h_
#define _depend_h_

#include <string>
#include <vector>
#include "Json.h"

class Dependency
{
    // origin
    std::string original_file; // the file as it is in bin/lib
    std::string canonical_file; // as original_file but with symlinks resolved
    std::string prefix;
    std::vector<std::string> symlinks;
    bool framework;

    // initialize function to be able to search many times
    bool findPrefix(std::string &path, const std::string& dependent_file);

public:
    Dependency(const std::string& path, const std::string& dependent_file);

    void print();

    std::string getOriginalFileName() const;
    std::string getOriginalPath() const { return prefix+getOriginalFileName(); }
    std::string getCanonicalFileName() const;
    std::string getCanonicalPath() const { return prefix+getCanonicalFileName(); }
    std::string getInstallPath();
    std::string getInnerPath();
    //std::string getFrameworkRoot() const;
    //std::string getFrameworkPath() const;
    std::string getFrameworkName() const;
    bool isFramework() const { return framework; }

    void addSymlink(const std::string& s);
    int getSymlinkAmount() const{ return symlinks.size(); }

    std::string getSymlink(const int i) const{ return symlinks[i]; }
    std::string getPrefix() const{ return prefix; }

    void copyYourself();
    void fixFileThatDependsOnMe(const std::string& file);

    // Compares the given dependency with this one. If both refer to the same file,
    // it returns true and merges both entries into one.
    bool mergeIfSameAs(Dependency& dep2);

    Json::VluType toJson() const;
};


#endif
