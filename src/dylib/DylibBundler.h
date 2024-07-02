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

#ifndef _crawler_
#define _crawler_

#include <string>
#include <vector>
#include "Types.h"
#include "Dependency.h"

class DylibBundler {
public:
    DylibBundler();
    ~DylibBundler();
    static DylibBundler* instance();

    /// @brief  collect dependencies for file
    /// @param file The file to check
    void collectDependencies(PathRef file, bool isExecutable);
    /// @brief  collect all un collected subDependancies
    void collectSubDependencies();
    /// @brief createDestDir/appbundle and process all files
    void moveAndFixBinaries();
    /// checks if path is a relative path i app binary
    static bool isRpath(PathRef path);
    /// @brief Search and find real paths for relative paths from binary
    /// @param rpath_file The file that has the rpath to look for
    /// @param dependent_file The dependency we search for
    /// @return true if found
    std::string searchFilenameInRPaths(
      PathRef rpath_file, PathRef dependent_file);
    /// @brief true if any dependency is a framework dependency
    bool hasFrameworkDep();

    /// @brief Dump all dependencies to json
    /// @param srcFile Only dump for this sourcefile if set
    /// @return Json::Object unique_ptr with the dump
    Json::VluType toJson(std::string_view srcFile = "") const;

    /// @brief Called from scrips. Meant to be called from script
    ///   fix libpath and rpaths in binary and codesign(if enabled) on files
    /// @param files The files to fix
    Json::VluType fixPathsInBinAndCodesign(const Json::Array* files);

private:
    enum DepState {
      Nothing              = 0x00,
      Collected            = 0x01,
      Copied               = 0x02,
      LibPathsChanged      = 0x04,
      RPathsChanged        = 0x08,
      Codesigned           = 0x10,
      Done                 = 0x20
    };
    void changeLibPathsOnFile(PathRef file);
    void collectRpaths(PathRef file);
    void fixRPathsOnFile(PathRef original_file, PathRef file_to_fix);
    void addDependency(PathRef path, PathRef filename);
    void collectDep(PathRef file, std::vector<std::string> &lines);
    void createDestDir() const;
    void fixupBinary(PathRef src, PathRef dest, bool iDependency);

    std::vector<Dependency> m_deps;
    std::map<std::string, std::vector<size_t> > m_deps_per_file;
    std::map<std::string, int> m_dep_state;
    std::map<std::string, Path> m_rpaths_per_file;
    std::map<std::string, Path> m_rpath_to_fullpath;
    Path m_currentFile;
    static DylibBundler *s_instance;
};

#endif
