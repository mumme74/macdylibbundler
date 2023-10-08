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

#include <algorithm>
#include <functional>
#include <filesystem>
#include <cctype>
#include <locale>
#include "Dependency.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/param.h>
#include "Utils.h"
#include "Settings.h"
#include "DylibBundler.h"

#include <stdlib.h>
#include <vector>

namespace fs = std::filesystem;

void cleanupFramwork(const fs::path frameworkPath, std::vector<std::string> keep) {
    std::error_code err;
    keep.push_back("Versions");
    keep.push_back("Resources");

    auto clean = [&err](fs::path path, std::vector<std::string>&keep) {
        for (auto& entry : fs::directory_iterator(path)) {
            if (std::find(keep.begin(), keep.end(), entry.path().filename()) != keep.end())
                continue; // keep this path

            fs::remove_all(entry, err);
            if (err) std::cerr << "Error occurred when cleaning up Framework "
                               << path << "\n error: " << err.message()
                               << std::endl;
        }
    };

    // cleanup inside Versions
    std::vector<std::string> k{"Current"};
    auto curPath = frameworkPath / "Versions" / "Current";
    if (fs::is_symlink(curPath, err))
        k.push_back(fs::read_symlink(curPath));
    clean(frameworkPath / "Versions", k);
    // inside the current versions dir
    clean(frameworkPath / "Versions" / k.back(), keep);

    // cleanup root
    clean(frameworkPath, keep);
}

void installId(const std::string& innerPath, const std::string& installPath){
    // Fix the lib's inner name
    std::string command = Settings::prefixTools() + "install_name_tool -id \"" + innerPath + "\" \"" + installPath + "\"";
    if( systemp( command ) != 0 )
    {
        std::cerr << "\n\nError : An error occured while trying to change identity of library " << installPath << std::endl;
        exit(1);
    }
}

// if some libs are missing prefixes, this will be set to true
// more stuff will then be necessary to do
bool missing_prefixes = false;

Dependency::Dependency(const std::string& path, const std::string& dependent_file)
  : framework(path.find(".framework") != std::string::npos)
{
    auto pathTrim = fs::path(rtrim(path));
    std::error_code err;
    if (isRpath(pathTrim)) {
        original_file = searchFilenameInRpaths(pathTrim, dependent_file);
        canonical_file = original_file;
    } else {
      original_file = pathTrim;

      if (fs::is_symlink(pathTrim, err)) {
        canonical_file = fs::read_symlink(pathTrim, err);
      } else if (err) {
        canonical_file = fs::canonical(original_file, err);
        if (err) canonical_file = pathTrim;
      } else {
        canonical_file = original_file; // no idea what it is?
      }
    }

    // check if given path is a symlink
    if (canonical_file != pathTrim) addSymlink(pathTrim);

    if (framework) {
        auto idx = canonical_file.find(".framework");
        auto root = canonical_file.substr(0, idx +10);
        prefix = root.substr(0, root.rfind("/")+1);
    } else
        prefix = canonical_file.substr(0, canonical_file.rfind("/")+1);

    if( !prefix.empty() && prefix[ prefix.size()-1 ] != '/' ) prefix += "/";

    // check if this dependency is in /usr/lib, /System/Library, or in ignored list
    if (!Settings::isPrefixBundled(prefix)) return;

    std::string p = pathTrim;
    if (!findPrefix(p, dependent_file))
        std::cerr << "\n/!\\ WARNING : Cannot resolve path '" << pathTrim.c_str() << "'" << std::endl;
}

// a separate method to be able to recursively find the string
bool Dependency::findPrefix(std::string &path, const std::string& dependent_file)
{
    auto canonical_name = getCanonicalFileName();
    // check if the lib is in a known location
    if( prefix.empty() || !fs::exists( prefix+canonical_name ) )
    {
        auto fwName = framework ? getFrameworkName() + "/" : "";

        //the paths contains at least /usr/lib so if it is empty we have not initialized it
        int searchPathAmount = Settings::searchPathAmount();

        //check if file is contained in one of the paths
        for( int i=0; i<searchPathAmount; ++i)
        {
            std::string search_path = Settings::searchPath(i) + fwName;
            if (fs::exists( search_path+canonical_name ))
            {
                if (fs::is_symlink(search_path+canonical_name)) {
                    canonical_file = fs::canonical(fs::path(search_path) /
                        fs::read_symlink(search_path+canonical_name));
                }

                if (Settings::verbose())
                    std::cout << "FOUND " << getCanonicalFileName() << " in " << search_path << std::endl;
                prefix = search_path;
                missing_prefixes = true; //the prefix was missing
                break;
            }
        }
    }

    //If the location is still unknown, ask the user for search path
    if( !Settings::isPrefixIgnored(prefix)
        && ( prefix.empty() || !fs::exists( getCanonicalPath() ) ) )
    {
        std::cerr << "\n/!\\ WARNING : Library " << canonical_name << " has an incomplete name (location unknown)" << std::endl;
        missing_prefixes = true;

        Settings::addSearchPath(getUserInputDirForFile(canonical_name));
        return findPrefix(path, dependent_file);
    }

    return fs::exists(prefix + getCanonicalFileName());
}

void Dependency::print()
{
    std::cout << std::endl;
    std::cout << " * " << getOriginalFileName() << " from ";
    if (framework) std::cout << "framework ";
    std::cout << prefix <<  std::endl;

    const int symamount = symlinks.size();
    for(int n=0; n<symamount; n++)
        std::cout << "     symlink --> " << symlinks[n].c_str() << std::endl;;
}

std::string Dependency::getOriginalFileName() const
{
    if (framework) {
        auto idx = original_file.find(".framework");
        auto path = original_file.substr(idx + 11);
        return stripPrefix(path);
    }
    return stripPrefix(original_file);
}

std::string Dependency::getCanonicalFileName() const
{
    if (framework) {
        auto idx = canonical_file.find(".framework");
        auto path = canonical_file.substr(idx + 11);
        return stripPrefix(path);
    }
    return stripPrefix(canonical_file);
}

std::string Dependency::getFrameworkName() const
{
    if (!framework) return "";
    auto idx = canonical_file.find(".framework");
    auto path = stripPrefix(canonical_file.substr(0, idx + 10));
    return path;
}

std::string Dependency::getInstallPath()
{
    if (framework) {
        auto idx = canonical_file.rfind(".framework") + 10;
        return Settings::frameworkDir() +
                getFrameworkName() + canonical_file.substr(idx);
    }
    return Settings::destFolder() + getCanonicalFileName();
}

std::string Dependency::getInnerPath()
{
    if (framework) {
        auto idx = canonical_file.rfind(".framework") + 10;
        return Settings::inside_framework_path() +
                getFrameworkName() + canonical_file.substr(idx);
    }
    return Settings::inside_lib_path() + getCanonicalFileName();
}


void Dependency::addSymlink(const std::string& s)
{
    // calling std::find on this vector is not near as slow as an extra invocation of install_name_tool
    if(std::find(symlinks.begin(), symlinks.end(), s) == symlinks.end()) symlinks.push_back(s);
}

// Compares the given Dependency with this one. If both refer to the same file,
// it returns true and merges both entries into one.
bool Dependency::mergeIfSameAs(Dependency& dep2)
{
    if(dep2.getOriginalFileName().compare(getOriginalFileName()) == 0)
    {
        const int samount = getSymlinkAmount();
        for(int n=0; n<samount; n++) {
            dep2.addSymlink(getSymlink(n));
        }
        return true;
    }
    return false;
}

void Dependency::copyYourself()
{
    auto canonical_path = getCanonicalPath();
    auto idx = canonical_path.find(".framework");
    auto from = framework ? canonical_path.substr(0, idx + 10) : canonical_path,
         to = framework ?
               Settings::frameworkDir() + getFrameworkName() :
                getInstallPath();

    if (Settings::verbose()) {
        std::cout << "Copying dependency" << std::endl
                  << "  - from " << from << std::endl
                  << "  - to " << to << std::endl
                  << "  - inner path " << getInnerPath() << std::endl
                  << "  - install path " << getInstallPath() << std::endl
                  << "  - is framework " << framework << std::endl;
    }

    auto copyOptions = fs::copy_options::update_existing
                     | fs::copy_options::recursive
                     | fs::copy_options::copy_symlinks;
    fs::copy(from, to, copyOptions);

    if (framework)
        cleanupFramwork(to, std::vector<std::string>{getCanonicalFileName()});

    // Fix the lib's inner name
    installId(getInnerPath(), getInstallPath());
}

void Dependency::fixFileThatDependsOnMe(const std::string& file_to_fix)
{
    // for main lib file
    changeInstallName(file_to_fix, original_file, getInnerPath());
    // for symlinks
    const int symamount = symlinks.size();
    for(int n=0; n<symamount; n++)
    {
        changeInstallName(file_to_fix, symlinks[n], getInnerPath());
    }

    // FIXME - hackish
    if(missing_prefixes)
    {
        // for main lib file
        changeInstallName(file_to_fix, original_file, getInnerPath());
        // for symlinks
        const int symamount = symlinks.size();
        for(int n=0; n<symamount; n++)
        {
            changeInstallName(file_to_fix, symlinks[n], getInnerPath());
        }
    }
}
