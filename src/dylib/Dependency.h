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
#include <filesystem>
#include "Json.h"
#include "Types.h"

class Dependency
{
public:
    Dependency(PathRef path, PathRef dependent_file);

    void print() const;

    PathRef getCanonical() const { return m_canonical_file; }
    PathRef getOriginal() const { return m_original_file; }
    Path getInstallPath() const;
    Path getInnerPath() const;
    std::string getFrameworkName() const;
    bool isFramework() const { return m_framework; }

    const std::vector<Path>& getSymlinks() const {
        return m_symlinks;
    }
    PathRef getPrefix() const{ return m_prefix; }

    void copyMyself() const;
    void fixFileThatDependsOnMe(PathRef file) const;

    // Compares the given dependency with this one. If both refer to the same file,
    // it returns true and merges both entries into one.
    bool mergeIfSameAs(Dependency& dep2);

    Json::VluType toJson() const;

private:

    void addSymlink(PathRef link);

    // initialize function to be able to search many times
    bool findPrefix(PathRef path, PathRef dependent_file);

    // origin
    Path m_original_file; // the file as it is in bin/lib
    Path m_canonical_file; // as original_file but with symlinks resolved
    Path m_prefix;
    std::vector<Path> m_symlinks;
    bool m_framework;

    // if some libs are missing prefixes, this will be set to true
    // more stuff will then be necessary to do
    bool m_missing_prefixes = false;
};


#endif
