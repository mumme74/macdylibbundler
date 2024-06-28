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
#include "Common.h"
#include "Utils.h"
#include "Settings.h"
#include "DylibBundler.h"

#include <stdlib.h>
#include <vector>

namespace fs = std::filesystem;

void cleanupFramwork(
    const fs::path frameworkPath,
    std::vector<std::string> keep
) {
    std::error_code err;
    keep.push_back("Versions");
    keep.push_back("Resources");
    keep.push_back("Libraries");
    keep.push_back("Helpers");

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
        k.push_back(fs::read_symlink(curPath).c_str());
    clean(frameworkPath / "Versions", k);
    // inside the current versions dir
    clean(frameworkPath / "Versions" / k.back(), keep);

    // cleanup root
    clean(frameworkPath, keep);
}

void installId(
    std::filesystem::path innerPath,
    std::filesystem::path installPath
){
    // Fix the lib's inner name
    std::stringstream ss;
    ss << Settings::installNameToolCmd() + " -id \""
       << innerPath << "\" \"" << installPath << "\"";
    if( systemp(ss.str()) != 0 ) {
        ss << "\n\nError : An error occurred while trying"
           << " to change identity of library " << installPath;
        exitMsg(ss.str());
    }
}


Dependency::Dependency(PathRef path, PathRef dependent_file) :
    m_original_file{path}, m_canonical_file{},
    m_prefix{}, m_symlinks{},
    m_framework(path.before(".framework") != path),
    m_missing_prefixes{false}
{
    std::error_code err;
    if (DylibBundler::isRpath(path)) {
        m_canonical_file = DylibBundler::instance()->
            searchFilenameInRPaths(path, dependent_file);
    } else {
        if (fs::is_symlink(path, err)) {
            m_canonical_file = fs::read_symlink(path, err);
        } else if (err) {
            m_canonical_file = fs::canonical(m_original_file, err);
            if (err) m_canonical_file = path;
        } else {
            m_canonical_file = m_original_file; // no idea what it is?
        }
    }

    if (fs::is_symlink(m_canonical_file))
        addSymlink(path.string());

    m_prefix = m_framework
                ? m_canonical_file.before(".framework")
                : m_canonical_file;

    // check if this dependency is in /usr/lib, /System/Library, or in ignored list
    if (Settings::blacklistedPath(m_prefix)) return;

    if (!findPrefix(path, dependent_file))
        std::cerr << "/!\\ WARNING : Cannot resolve path '"
                  << path << "'\n\n";
}

// a separate method to be able to recursively find prefix
bool
Dependency::findPrefix(PathRef path, PathRef dependent_file) {
    const auto canonical_name = m_canonical_file.end_name();
    // check if the lib is in a known location
    auto pat = fs::path(m_prefix) / canonical_name;
    if( m_prefix.empty() || !fs::exists(pat) )
    {
        auto fwName = getFrameworkName() + ".framework";

        //the paths contains at least /usr/lib so if it is empty we have not initialized it

        //check if file is contained in one of the paths
        for(const auto& searchPath : Settings::searchPaths())
        {
            auto search_path = searchPath / fwName;
            auto path = search_path / canonical_name;
            if (fs::exists(path))
            {
                if (fs::is_symlink(path)) {
                    m_canonical_file = (fs::canonical(fs::path(search_path) /
                        fs::read_symlink(path))).string();
                }

                if (Settings::verbose())
                    std::cout << "FOUND " << canonical_name
                              << " in " << search_path << std::endl;
                m_prefix = search_path;
                if (!Settings::blacklistedPath(m_prefix))
                    m_missing_prefixes = true; //the prefix was missing
                break;
            }
        }
    }

    //If the location is still unknown, ask the user for search path
    if( !Settings::isPrefixIgnored(m_prefix)
        && (m_prefix.empty() || !fs::exists(getCanonical())))
    {
        std::cerr << "\n/!\\ WARNING : Library " << canonical_name
                  << " has an incomplete name (location unknown)" << std::endl;
        m_missing_prefixes = true;

        Settings::addSearchPath(getUserInputDirForFile(canonical_name));
        return findPrefix(path, dependent_file);
    }

    return fs::exists(fs::path(m_prefix) / getCanonical());
}

void
Dependency::print() const
{
    std::cout << std::endl;
    std::cout << " * " << getOriginal().filename() << " from ";
    if (m_framework) std::cout << "framework ";
    std::cout << m_prefix <<  std::endl;

    for(const auto& link : m_symlinks)
        std::cout << "     symlink --> " << link
                  << std::endl;;
}

std::string
Dependency::getFrameworkName() const
{
    if (!m_framework) return "";
    auto frmDir = m_original_file.upto(".framework")
                                 .filename().string();
    return frmDir.substr(0, frmDir.size()-10);
}

Path
Dependency::getInstallPath() const
{
    if (m_framework) {
        auto path = Settings::frameworkDir()
                  / getFrameworkName()
                  + ".framework"
                  / m_canonical_file.after(".framework");
        return path;
    }
    return Settings::destFolder() / getCanonical().filename();
}

Path
Dependency::getInnerPath() const
{
    if (m_framework) {
        auto path = Settings::inside_framework_path()
                  / m_canonical_file.from(".framework");
        return path;
    }
    return Settings::inside_lib_path() / getCanonical().filename();
}


void
Dependency::addSymlink(PathRef link)
{
    // calling std::find on this vector is not near as slow as an extra invocation of install_name_tool
    const auto lnk = link.string();
    if(std::find(m_symlinks.begin(), m_symlinks.end(), lnk) == m_symlinks.end())
        m_symlinks.push_back(Path(link));
}

// Compares the given Dependency with this one. If both refer to the same file,
// it returns true and merges both entries into one.
bool
Dependency::mergeIfSameAs(Dependency& dep2)
{
    if(dep2.getOriginal().filename() == getOriginal().filename()) {
        for(auto& link : m_symlinks) {
            dep2.addSymlink(link);
        }
        return true;
    }
    return false;
}

void
Dependency::copyMyself() const
{
    std::error_code err;
    std::stringstream ss;
    Path from, to;
    if (m_framework) {
        from = getCanonical().upto(".framework");
        to = Settings::frameworkDir()
           / getFrameworkName()
           + ".framework";
    } else {
        from = getCanonical();
        to = getInnerPath();
    }

    if (Settings::verbose()) {
        std::cout << "Copying dependency" << "\n"
                  << "  - from " << from << "\n"
                  << "  - to " << to << "\n"
                  << "  - inner path " << getInnerPath() << "\n"
                  << "  - install path " << getInstallPath() << "\n"
                  << "  - is framework " << m_framework << "\n";
    }

    if (!fs::exists(to, err))
        createFolder(to);

    auto copyOptions = fs::copy_options::update_existing
                     | fs::copy_options::recursive
                     | fs::copy_options::copy_symlinks;
    fs::copy(from, to, copyOptions, err);
    if (err) {
        ss << "\nFailed to copy " << from << " to: " << to
           << ", error: " << err.message() << "\n";
        exitMsg(ss.str());
    }

    if (m_framework)
        cleanupFramwork(to,
            std::vector<std::string>{getCanonical().filename()});

    // Fix the lib's inner name
    installId(getInnerPath(), getInstallPath());
}

void
Dependency::fixFileThatDependsOnMe(PathRef file) const
{
    // for main lib file
    changeInstallName(file, m_original_file, getInnerPath());
    // for symlinks
    for(const auto& link : m_symlinks) {
        changeInstallName(file, link, getInnerPath());
    }

    // FIXME - hackish
    if(m_missing_prefixes)
    {
        // for main lib file
        changeInstallName(file, m_original_file, getInnerPath());
        // for symlinks
        for(const auto& link : m_symlinks) {
            changeInstallName(file, link, getInnerPath());
        }
    }
}

Json::VluType
Dependency::toJson() const
{
    using namespace Json;

    Array links{};
    for (const auto& link : m_symlinks)
        links.push(String(link));

    auto obj = std::make_unique<Object>(
      std::initializer_list<std::pair<std::string, VluBase>>{
        {"original_file", String(m_original_file)},
        {"canonical_file", String(m_canonical_file)},
        {"prefix", String(m_prefix)},
        {"symlinks", links},
        {"isFramework", Bool(m_framework)}
      }
    );
    return obj;
}
