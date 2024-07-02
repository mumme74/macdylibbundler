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

#include "DylibBundler.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <map>
#include <regex>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sys/param.h>
#include <algorithm>
#include <cassert>
#ifdef __linux
#include <linux/limits.h>
#endif
#include "Common.h"
#include "Utils.h"
#include "Settings.h"
#include "Dependency.h"

namespace fs = std::filesystem;

void mkAppBundleTemplate();
void mkInfoPlist();

DylibBundler::DylibBundler():
    m_deps{},
    m_deps_per_file{},
    m_dep_state{},
    m_rpaths_per_file{},
    m_rpath_to_fullpath{},
    m_currentFile{}
{
    assert(DylibBundler::s_instance == nullptr &&
         "Should only have one instance");
    DylibBundler::s_instance = this;
}

DylibBundler::~DylibBundler()
{}

DylibBundler*
DylibBundler::instance()
{
    return DylibBundler::s_instance;
}

DylibBundler *DylibBundler::s_instance = nullptr;

void
DylibBundler::changeLibPathsOnFile(PathRef file ) {
    if (m_deps_per_file.find(file.string()) == m_deps_per_file.end()) {
        std::cout << "    ";
        collectDependencies(file, false);
        std::cout << "\n";
    }
    std::cout << "  * Fixing dependencies on " << file << std::endl;

    for(const auto& idx : m_deps_per_file.at(file.string())) {
        m_deps[idx].fixFileThatDependsOnMe(file);
    }
    m_dep_state[file.string()] |= LibPathsChanged;
}

bool // static
DylibBundler::isRpath(PathRef path)
{
    auto it = path.begin();
    auto fn = it != path.end()
            ? it->filename().string() : path.string();
    return fn == "@rpath" || fn == "@loader_path";
}

void
DylibBundler::collectRpaths(PathRef file) {
    if (!fs::exists(file))
    {
        std::cerr << "\n/!\\ WARNING : can't collect rpaths for nonexistent file '" << file << "'\n";
        return;
    }

    std::stringstream cmd;
    cmd << Settings::otoolCmd() << " -l \"" << file << "\"";
    std::string output = system_get_output(cmd.str());

    auto lc_lines = tokenize(output, "\n");

    size_t pos = 0;
    bool read_rpath = false;
    while (pos < lc_lines.size())
    {
        auto line = lc_lines[pos];
        pos++;

        if (read_rpath)
        {
            size_t start_pos = line.find("path ");
            size_t end_pos = line.find(" (");
            if (start_pos == std::string::npos || end_pos == std::string::npos)
            {
                std::cerr << "\n/!\\ WARNING: Unexpected LC_RPATH format\n";
                continue;
            }
            start_pos += 5;
            auto rpath = line.substr(start_pos, end_pos - start_pos);
            m_rpaths_per_file[file.string()].append(rpath);
            read_rpath = false;
            continue;
        }

        if (line.find("LC_RPATH") != std::string::npos)
        {
            read_rpath = true;
            pos++;
        }
    }
}

std::string
DylibBundler::searchFilenameInRPaths(
    PathRef rpath_file,
    PathRef dependent_file
) {
    Path fullpath;
    auto rpathStr = rpath_file.string();
    std::string suffix = std::regex_replace(
        rpathStr, std::regex("^@[a-z_]+path/"), "");

    const auto check_path = [&](PathRef path)
    {
        auto depParentDir = dependent_file.parent_path();
        if (dependent_file != rpath_file)
        {
            auto firstDir = path.begin()->filename();
            if (firstDir == "@loader_path" || firstDir == "@rpath") {
                std::error_code err;
                auto canonical = fs::canonical(
                    path.strip_prefix(), err);
                if (!err) {
                    fullpath = canonical;
                    return true;
                }
            }
        }
        return false;
    };

    // fullpath previously stored
    if (m_rpath_to_fullpath.find(rpathStr) != m_rpath_to_fullpath.end())
    {
        fullpath = m_rpath_to_fullpath[rpathStr];
    }
    else if (!check_path(rpath_file))
    {
        for (const auto& rpath : m_rpaths_per_file[dependent_file.string()]) {
            if (check_path(static_cast<Path>(rpath) / suffix))
                break;
        }
        if (m_rpath_to_fullpath.find(rpathStr) != m_rpath_to_fullpath.end())
        {
            fullpath = m_rpath_to_fullpath[rpathStr];
        }
    }

    if (fullpath.empty()) {
        for (const auto& searchPath : Settings::searchPaths()) {
            if (fs::exists(searchPath / suffix)) {
                fullpath = searchPath / suffix;
                break;
            }
        }

        // let user tell the path to look in
        if (fullpath.empty()) {
            std::cerr << "\n/!\\ WARNING : can't get path for '"
                      << rpath_file << "'\n"
                      << "Consider adding dir to search path as a switch, ie: -s=../dir1 -s=dir2/\n";
            auto dir = getUserInputDirForFile(suffix);
            fullpath = std::string(dir) + suffix;
            std::error_code err;
            auto canonical = fs::canonical(fullpath, err);
            if (!err)
                fullpath = canonical;
            Settings::addSearchPath(dir);
        }
    }

    return fullpath;
}

void
DylibBundler::fixRPathsOnFile(
    PathRef original_file,
    PathRef file_to_fix
) {
    if (Settings::createAppBundle()) return; // don't change @rpath on app bundles
    std::vector<std::string> rpaths_to_fix;

    if ((m_dep_state[file_to_fix] & RPathsChanged) != 0) return;

    for (const auto& pair : m_rpaths_per_file){
        if (pair.first == original_file) {
            rpaths_to_fix.emplace_back(pair.second);
            break;
        }
    }

    for (const auto& rpath : rpaths_to_fix) {
        std::stringstream cmd;
        cmd << Settings::installNameToolCmd() << " -rpath \""
            << rpath << "\" \"" << Settings::inside_lib_path()
            << "\" \"" << file_to_fix << "\"";
        if ( systemp(cmd.str()) != 0)
        {
            std::cerr << "\n\nError : An error occurred while "
                      << "trying to fix dependencies of "
                      << file_to_fix << std::endl;
        }
    }
    m_dep_state[file_to_fix] |= RPathsChanged;
}

void
DylibBundler::addDependency(PathRef path, PathRef file)
{
    Dependency dep(path, file, false);

    // we need to check if this library was already added to avoid duplicates
    bool in_deps = false;
    for (auto& d : m_deps) {
        if (dep.mergeIfSameAs(d))
            in_deps = true;
    }

    if (Settings::blacklistedPath(dep.getPrefix())) {
        if (Settings::verbose())
            std::cout << "*Ignoring dependency " << dep.getPrefix()
                      << " prefix not bundled" << std::endl;
        return;
    }

    if (!in_deps) {
        m_deps.push_back(dep);
        m_deps_per_file[dep.getInstallPath()]
            .push_back(m_deps.size()-1);
    }
}

/*
 *  Fill vector 'lines' with dependencies of given 'filename'
 */
void
DylibBundler::collectDep(
    PathRef file,
    std::vector<std::string>& lines
) {
    // execute "otool -l" on the given file and collect the command's output
    std::stringstream ss;
    ss << Settings::otoolCmd() << " -l \"" << file << "\"";
    std::string output = system_get_output(ss.str());

    if(output.find("can't open file")!=std::string::npos or output.find("No such file")!=std::string::npos or output.size()<1)
        exitMsg(std::string("Cannot find file ") + file.string()
                 + " to read its dependencies");

    // split output
    auto raw_lines = tokenize(output, "\n");

    bool searching = false;
    for(const auto& line : raw_lines) {
        const auto &is_prefix = [&line](const char *const p) {
            return line.find(p) != std::string::npos;
        };
        if (is_prefix("cmd LC_LOAD_DYLIB") || is_prefix("cmd LC_REEXPORT_DYLIB"))
        {
            if (searching)
                exitMsg("\n\n/!\\ ERROR: Failed to find name before next cmd");

            searching = true;
        }
        else if (searching)
        {
            size_t found = line.find("name ");
            if (found != std::string::npos) {
                std::string ln{'\t'};
                ln += line.substr(found+5, std::string::npos);
                lines.push_back(ln);
                searching = false;
            }
        }
    }
}

void
DylibBundler::collectDependencies(PathRef file, bool isExecutable)
{
    m_currentFile = file;
    if (m_dep_state.find(file.string()) != m_dep_state.end())
        return;

    collectRpaths(file);

    std::vector<std::string> lines;
    collectDep(file, lines);

    Settings::verbose() ?
        std::cout << std::endl << "Collect dependencies for '"
                  << file.string() << "'" << std::endl
        : std::cout << ".";
    fflush(stdout);

    for (const auto& line : lines)
    {
        if (line[0] != '\t') continue; // only lines beginning with a tab interest us

        // trim useless info, keep only library name
        auto dep_path = line.substr(1, line.rfind(" (") - 1);

        if (line.find(".framework") != std::string::npos &&
            !Settings::bundleFrameworks())
        {
            if (Settings::verbose())
                std::cout << "  ignore framework: " <<  dep_path << std::endl;
            continue;
        }

        if (Settings::isSystemLibrary(dep_path)) {
            if (Settings::verbose())
                std::cout << "  ignore system: " << dep_path << std::endl;
            continue;
        }

        if (Settings::verbose())
          std::cout << "  adding: " << dep_path << " dependent file: " << file <<std::endl;
        else std::cout << ".";
        std::cout.flush();
        addDependency(Path(dep_path.data()), file);
    }

    Dependency exec(file, file, isExecutable);
    m_deps.emplace_back(exec);
    m_deps_per_file[exec.getInstallPath().string()]
        .emplace_back(m_deps.size()-1);
    m_dep_state[file.string()] |= Collected;
}

void
DylibBundler::collectSubDependencies()
{
    size_t dep_amount;

    // recursively collect each dependency's dependencies
    do {
        dep_amount = m_deps.size();

        // can't use range based loop, m_deps might grow
        for (size_t i = 0; i < m_deps.size(); ++i)
        {
            auto original_path = m_deps[i].getOriginal();
            Settings::verbose() ?
                std::cout << "* SubDependencies for: " << original_path << std::endl
              : std::cout << ".";
            fflush(stdout);
            if (isRpath(original_path)) {
                original_path = searchFilenameInRPaths(
                    original_path, original_path);
            } else if (!fs::exists(original_path)) {
                original_path = m_deps[i].getPrefix()
                              / original_path;
            }

            collectDependencies(original_path, false);
        }

    } while (m_deps.size() != dep_amount);
      // no more dependencies were added on this iteration, stop searching
}

bool
DylibBundler::hasFrameworkDep() {
    return m_deps.end() != std::find_if(
        m_deps.begin(), m_deps.end(),[](const auto &dep){
        return dep.isFramework();
    });
}

Json::VluType
DylibBundler::toJson(std::string_view srcFile) const
{
    using namespace Json;
    Array srcFiles;

    std::error_code err;
    auto cwd = fs::current_path(err);
    auto appDir = fs::canonical(
        fs::relative(
            Settings::srcFiles()[0].src.parent_path() /"", cwd, err),
        err);
    if (appDir.empty()) appDir = cwd;

    Object src_files{};
    // find out which src files
    for (const auto& pair : m_deps_per_file) {
        srcFiles.push(String(pair.first));
        if (srcFile == pair.first || srcFile.empty()) {
            Array value;
            for (const auto& idx : pair.second)
                value.push(m_deps[idx].toJson());
            src_files.set(pair.first, value);
        }
    }

    auto root = std::make_unique<Object>(ObjInitializer{
        {"working_dir", String(cwd)},
        {"app_path", String(appDir)},
        {"files_to_fix", std::move(srcFiles)},
        {"src_files", src_files}
    });

    return root;
}

Json::VluType
DylibBundler::fixPathsInBinAndCodesign(const Json::Array* files)
{
    auto resObj = std::make_unique<Json::Object>();
    try {
        for (auto& file : *files) {
            if (!file->isString()) {
                std::stringstream msg;
                msg << "*Expected a string, but got a " << file->typeName()
                    << std::endl;
                throw msg.str();
            }
            collectDependencies(file->asString()->vlu(), false);
        }
        collectSubDependencies();

        std::cout << "\n Postprocess requested by a script: " << std::endl;

        // print info to user
        for(const auto& dep : m_deps) {
            if ((m_dep_state[dep.getInstallPath().string()] & Done) == 0)
                dep.print();
        }
        std::cout << std::endl;

        // copy dependency files if requested by user
        if(Settings::bundleLibs())
        {
            // can't use range based loop here, m_deps might grow
            for(size_t i = 0; i < m_deps.size(); ++i) {
                const auto& dep = m_deps[i];
                if ((m_dep_state[dep.getInstallPath().string()] & Done) == 0)
                    fixupBinary(
                        dep.getCanonical(),
                        dep.getInstallPath(), true);
            }
        }
        resObj->set("result", Json::Bool(true));
    } catch(std::exception& e) {
        resObj->set("error", Json::String(e.what()));
    } catch(std::string& errStr) {
        resObj->set("error", Json::String(errStr));
    }

    return resObj;
}

void
DylibBundler::createDestDir() const
{
    std::error_code err;
    std::stringstream ss;
    auto dest_folder = Settings::destFolder();
    std::cout << "* Checking output directory " << dest_folder << std::endl;

    createFolder(dest_folder);
}

void
DylibBundler::fixupBinary(PathRef src, PathRef dest, bool isSubDependency)
{
    if ((m_dep_state[dest.string()] & Done) == Done) {
        std::cout << "\n*Skipping " << dest << " already done \n";
        return;
    }
    if (Settings::verbose()) {
        std::cout << "\n* Processing "
                  << (isSubDependency ? " dependency " : "")
                  << src;
        if (src != dest)
            std::cout << std::string(" into ") << dest;
        std::cout << std::endl;
    }
    if (!fs::exists(dest) &&
        (m_dep_state[dest.string()] & Copied) == 0
    ) {
        copyFile(src, dest); // to set write permission or move
        m_dep_state[dest.string()] |= Copied;
    }
    changeLibPathsOnFile(dest.string());
    fixRPathsOnFile(src.string(), dest.string());

    if ((m_dep_state[dest.string()] & Codesigned) == 0 &&
        Settings::canCodesign()
    ) {
        adhocCodeSign(dest.string());
        m_dep_state[dest.string()] |= Codesigned;
    }

    if (Settings::verbose())
        std::cout << "\n-- Done Processing "
                  << (isSubDependency ? " dependency " : "")
                  << " for " << src << std::endl;

    m_dep_state[dest.string()] |= Done;
}

void
DylibBundler::moveAndFixBinaries()
{
    std::cout << std::endl;

    // print info to user
    for(const auto& dep : m_deps) {
        dep.print();
    }
    std::cout << std::endl;

    if(Settings::createAppBundle()) mkAppBundleTemplate();

    // copy dependency files if requested by user
    if(Settings::bundleLibs())
    {
        createDestDir();
        // can't use rangebase loop here, m_deps might grow
        for(size_t i = 0; i < m_deps.size(); ++i) {
            const auto& dep = m_deps[i];
            fixupBinary(
                dep.getCanonical(), dep.getInstallPath(), true);
        }
    }
}

// -----------------------------------------------------------------------

void mkAppBundleTemplate() {
    // see https://eclecticlight.co/2021/12/13/whats-in-that-app-a-guide-to-app-internal-structure/
    // setting up directory structure and symlinks
    auto curDir = fs::current_path();
    std::error_code err;
    auto bundlePath = Settings::appBundlePath();
    if (fs::exists(bundlePath)) {
        if (Settings::canOverwriteDir()) {
            fs::remove_all(bundlePath, err);
            if (err)
                exitMsg(std::string("Could not remove old bundle dir at: ")
                  + bundlePath.string(), err);

        } else
            exitMsg(std::string("Can't overwrite ")
               + bundlePath.string() + ", need --overwrite-dir switch.");
    }
    auto cont = Settings::appBundleContentsDir();
    if (!fs::create_directories(Settings::appBundleExecDir(), err))
        exitMsg("Could not create AppBundle dirs.", err);

    if (DylibBundler::instance()->hasFrameworkDep() &&
        !fs::create_directory(cont / "Frameworks", err))
        exitMsg("Could not create Frameworks dir in app bundle.", err);

    auto appRootDir = bundlePath / bundlePath.filename();
    fs::create_directory(appRootDir, err);
    if (err)
        exitMsg(std::string("Could not create dir ") + appRootDir.string(), err);

    fs::current_path(appRootDir, err);
    if (err)
        exitMsg(std::string("Could not cd into ") + appRootDir.string(), err);

    fs::create_directory_symlink("../Contents", "Contents", err);
    if (err)
        exitMsg("Could not create symlink Contents in app bundle.", err);

    fs::current_path(curDir, err);

    // not sure if its needed, for old version macs
    std::ofstream pkgInfo;
    pkgInfo.open(cont / "Pkginfo");
    pkgInfo << "APPL????";
    pkgInfo.close();

    mkInfoPlist();
}

void mkInfoPlist() {
    /// default find any a Info.plist file in current dir
    if (Settings::infoPlist().empty() && fs::exists("Info.plist"))
        Settings::setInfoPlist("Info.plist");

    std::stringstream plist;
    if (!Settings::infoPlist().empty()) {
        std::ifstream infoPlist;
        infoPlist.open(Settings::infoPlist());
        plist << infoPlist.rdbuf();
        infoPlist.close();
    } else {
        std::cout << "* Creating a minimal Info.plist file." << std::endl
                  << "    See --app-info-plist option to customize" << std::endl;

        auto appName = Settings::appBundlePath().filename().string();
        appName = appName.substr(0, appName.find("."));

        plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\""
              << " \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
              << "<plist version=\"1.0\">\n"
              << "<dict>\n"
              << "  <key>CFBundleExecutable</key>\n"
              << "  <string>" << appName << "</string>\n"
              << "  <key>CFBundleIconFile</key>\n"
              << "  <string></string>\n"
              << "  <key>CFBundleIdentifer</key>\n"
              << "  <string>com.yourcompany." << appName << "</string>\n"
	          << "  <key>NOTE</key>\n"
	          << "  <string>This file was generated by macdylibbundler.</string>\n"
              << "</dict>\n"
              << "</plist>\n";
    }

    std::ofstream infoPlist;
    infoPlist.open(Settings::appBundleContentsDir() / "Info.plist");
    infoPlist << plist.str();
    infoPlist.close();
}
