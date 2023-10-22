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
#ifdef __linux
#include <linux/limits.h>
#endif
#include "Utils.h"
#include "Settings.h"
#include "Dependency.h"

namespace fs = std::filesystem;

void mkAppBundleTemplate();
void mkInfoPlist();
void runPythonScripts_onAppBundle();

std::vector<Dependency> deps;
std::map<std::string, std::vector<Dependency> > deps_per_file;
std::map<std::string, bool> deps_collected;
std::map<std::string, std::vector<std::string> > rpaths_per_file;
std::map<std::string, std::string> rpath_to_fullpath;

void changeLibPathsOnFile(std::string file_to_fix)
{
    if (deps_collected.find(file_to_fix) == deps_collected.end())
    {
        std::cout << "    ";
        collectDependencies(file_to_fix);
        std::cout << "\n";
    }
    std::cout << "  * Fixing dependencies on " << file_to_fix.c_str() << std::endl;

    std::vector<Dependency> deps_in_file = deps_per_file[file_to_fix];
    const int dep_amount = deps_in_file.size();
    for(int n=0; n<dep_amount; n++)
    {
        deps_in_file[n].fixFileThatDependsOnMe(file_to_fix);
    }
}

bool isRpath(const std::string& path)
{
    return path.find("@rpath") == 0 || path.find("@loader_path") == 0;
}

void collectRpaths(const std::string& filename)
{
    if (!fileExists(filename))
    {
        std::cerr << "\n/!\\ WARNING : can't collect rpaths for nonexistent file '" << filename << "'\n";
        return;
    }

    std::string cmd = Settings::prefixTools() + "otool -l \"" + filename + "\"";
    std::string output = system_get_output(cmd);

    std::vector<std::string> lc_lines;
    tokenize(output, "\n", &lc_lines);

    size_t pos = 0;
    bool read_rpath = false;
    while (pos < lc_lines.size())
    {
        std::string line = lc_lines[pos];
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
            std::string rpath = line.substr(start_pos, end_pos - start_pos);
            rpaths_per_file[filename].push_back(rpath);
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

std::string searchFilenameInRpaths(const std::string& rpath_file, const std::string& dependent_file)
{
    char buffer[PATH_MAX];
    std::string fullpath;
    std::string suffix = std::regex_replace(rpath_file, std::regex("^@[a-z_]+path/"), "");

    const auto check_path = [&](std::string path)
    {
        char buffer[PATH_MAX];
        std::string file_prefix = dependent_file.substr(0, dependent_file.rfind('/')+1);
        if (dependent_file != rpath_file)
        {
            std::string path_to_check;
            if (path.find("@loader_path") != std::string::npos)
            {
                path_to_check = std::regex_replace(path, std::regex("@loader_path/"), file_prefix);
            }
            else if (path.find("@rpath") != std::string::npos)
            {
                path_to_check = std::regex_replace(path, std::regex("@rpath/"), file_prefix);
            }
            if (realpath(path_to_check.c_str(), buffer))
            {
                fullpath = buffer;
                rpath_to_fullpath[rpath_file] = fullpath;
                return true;
            }
        }
        return false;
    };

    // fullpath previously stored
    if (rpath_to_fullpath.find(rpath_file) != rpath_to_fullpath.end())
    {
        fullpath = rpath_to_fullpath[rpath_file];
    }
    else if (!check_path(rpath_file))
    {
        for (auto rpath : rpaths_per_file[dependent_file])
        {
            if (rpath[rpath.size()-1] != '/') rpath += "/";
            if (check_path(rpath+suffix)) break;
        }
        if (rpath_to_fullpath.find(rpath_file) != rpath_to_fullpath.end())
        {
            fullpath = rpath_to_fullpath[rpath_file];
        }
    }

    if (fullpath.empty())
    {
        const int searchPathAmount = Settings::searchPathAmount();
        for (int n=0; n<searchPathAmount; n++)
        {
            std::string search_path = Settings::searchPath(n);
            if (fileExists(search_path+suffix))
            {
                fullpath = search_path + suffix;
                break;
            }
        }

        if (fullpath.empty())
        {
            std::cerr << "\n/!\\ WARNING : can't get path for '" << rpath_file << "'\n";
            fullpath = getUserInputDirForFile(suffix) + suffix;
            if (realpath(fullpath.c_str(), buffer))
            {
                fullpath = buffer;
            }
        }
    }

    return fullpath;
}

std::string searchFilenameInRpaths(const std::string& rpath_dep)
{
    return searchFilenameInRpaths(rpath_dep, rpath_dep);
}

void fixRpathsOnFile(const std::string& original_file, const std::string& file_to_fix)
{
    if (Settings::createAppBundle()) return; // don't change @rpath on app bundles
    std::vector<std::string> rpaths_to_fix;
    std::map<std::string, std::vector<std::string> >::iterator found = rpaths_per_file.find(original_file);
    if (found != rpaths_per_file.end())
    {
        rpaths_to_fix = found->second;
    }

    for (size_t i=0; i < rpaths_to_fix.size(); ++i)
    {
        std::string command = Settings::prefixTools() + "install_name_tool -rpath \"" +
                rpaths_to_fix[i] + "\" \"" + Settings::inside_lib_path() + "\" \"" + file_to_fix + "\"";
        if ( systemp(command) != 0)
        {
            std::cerr << "\n\nError : An error occured while trying to fix dependencies of " << file_to_fix << std::endl;
        }
    }
}

void addDependency(const std::string& path, const std::string& filename)
{
    Dependency dep(path, filename);

    // we need to check if this library was already added to avoid duplicates
    bool in_deps = false;
    const int dep_amount = deps.size();
    for(int n=0; n<dep_amount; n++)
    {
        if(dep.mergeIfSameAs(deps[n])) in_deps = true;
    }

    // check if this library was already added to |deps_per_file[filename]| to avoid duplicates
    std::vector<Dependency> deps_in_file = deps_per_file[filename];
    bool in_deps_per_file = false;
    const int deps_in_file_amount = deps_in_file.size();
    for(int n=0; n<deps_in_file_amount; n++)
    {
        if(dep.mergeIfSameAs(deps_in_file[n])) in_deps_per_file = true;
    }

    if(!Settings::isPrefixBundled(dep.getPrefix())) {
        if (Settings::verbose())
            std::cout << "*Ignoring dependency " << dep.getPrefix()
                      << " prefix not bundled" << std::endl;
        return;
    }

    if(!in_deps) deps.push_back(dep);
    if(!in_deps_per_file) deps_per_file[filename].push_back(dep);
}

/*
 *  Fill vector 'lines' with dependencies of given 'filename'
 */
void collectDependencies(const std::string& filename, std::vector<std::string>& lines)
{
    // execute "otool -l" on the given file and collect the command's output
    std::string cmd = Settings::prefixTools() + "otool -l \"" + filename + "\"";
    std::string output = system_get_output(cmd);

    if(output.find("can't open file")!=std::string::npos or output.find("No such file")!=std::string::npos or output.size()<1)
        exitMsg(std::string("Cannot find file ") + filename + " to read its dependencies");

    // split output
    std::vector<std::string> raw_lines;
    tokenize(output, "\n", &raw_lines);

    bool searching = false;
    for(const auto& line : raw_lines) {
        const auto &is_prefix = [&line](const char *const p) { return line.find(p) != std::string::npos; };
        if (is_prefix("cmd LC_LOAD_DYLIB") || is_prefix("cmd LC_REEXPORT_DYLIB"))
        {
            if (searching)
                exitMsg("\n\n/!\\ ERROR: Failed to find name before next cmd");

            searching = true;
        }
        else if (searching)
        {
            size_t found = line.find("name ");
            if (found != std::string::npos)
            {
                lines.push_back('\t' + line.substr(found+5, std::string::npos));
                searching = false;
            }
        }
    }
}

void collectDependencies(const std::string& filename)
{
    if (deps_collected.find(filename) != deps_collected.end()) return;

    collectRpaths(filename);

    std::vector<std::string> lines;
    collectDependencies(filename, lines);

    Settings::verbose() ? std::cout << std::endl << "Collect dependencies for '" + filename + "'" << std::endl : std::cout << ".";
    fflush(stdout);

    for (const auto& line : lines)
    {
        if (line[0] != '\t') continue; // only lines beginning with a tab interest us

        // trim useless info, keep only library name
        std::string dep_path = line.substr(1, line.rfind(" (") - 1);

        if (line.find(".framework") != std::string::npos &&
            !Settings::bundleFrameworks())
        {
            if (Settings::verbose()) std::cout << "  ignore framework: " <<  dep_path << std::endl;
            continue;
        }

        if (Settings::isSystemLibrary(dep_path)) {
            if (Settings::verbose()) std::cout << "  ignore system: " << dep_path << std::endl;
            continue;
        }

        if (Settings::verbose())
          std::cout << "  adding: " << dep_path << " dependent file: " << filename <<std::endl;
        else std::cout << ".";
        fflush(stdout);
        addDependency(dep_path, filename);
    }

    deps_collected[filename] = true;
}

void collectSubDependencies()
{
    // print status to user
    size_t dep_amount = deps.size();

    // recursively collect each dependencie's dependencies
    while(true)
    {
        dep_amount = deps.size();
        for (size_t n=0; n<dep_amount; n++)
        {
            std::string original_path = deps[n].getOriginalPath();
            Settings::verbose() ? std::cout << "* SubDependencies for: " + original_path << std::endl : std::cout << ".";
            fflush(stdout);
            if (isRpath(original_path)) original_path = searchFilenameInRpaths(original_path);

            collectDependencies(original_path);
        }

        if(deps.size() == dep_amount) break; // no more dependencies were added on this iteration, stop searching
    }
}

void createDestDir()
{
    std::string dest_folder = Settings::destFolder();
    std::cout << "* Checking output directory " << dest_folder.c_str() << std::endl;

    // ----------- check dest folder stuff ----------
    bool dest_exists = fileExists(dest_folder);

    if(dest_exists and Settings::canOverwriteDir())
    {
        std::cout << "* Erasing old output directory " << dest_folder.c_str() << std::endl;
        std::string command = std::string("rm -r \"") + dest_folder + "\"";
        if( systemp( command ) != 0)
            exitMsg("\n\nError : An error occurred while attempting to overwrite dest folder.");

        dest_exists = false;
    }

    if(!dest_exists)
    {
        if(Settings::canCreateDir())
        {
            std::cout << "* Creating output directory " << dest_folder.c_str() << std::endl;
            std::string command = std::string("mkdir -p \"") + dest_folder + "\"";
            if( systemp( command ) != 0)
                exitMsg("\n\nError : An error occurred while creating dest folder.");

        }
        else
            exitMsg("\n\nError : Dest folder does not exist. Create it or pass the appropriate flag for automatic dest dir creation.");
    }

}

void doneWithDeps_go()
{
    std::cout << std::endl;
    const int dep_amount = deps.size();
    // print info to user
    for(int n=0; n<dep_amount; n++)
    {
        deps[n].print();
    }
    std::cout << std::endl;

    if(Settings::createAppBundle()) mkAppBundleTemplate();

    // copy files if requested by user
    if(Settings::bundleLibs())
    {
        createDestDir();

        for(int n=dep_amount-1; n>=0; n--)
        {
            if (Settings::verbose())
                std::cout << "\n* Processing dependency " << deps[n].getInstallPath() << std::endl;
            deps[n].copyYourself();
            changeLibPathsOnFile(deps[n].getInstallPath());
            fixRpathsOnFile(deps[n].getOriginalPath(), deps[n].getInstallPath());
            adhocCodeSign(deps[n].getInstallPath());
            if (Settings::verbose())
                std::cout << "\n-- Done Processing dependency for " << deps[n].getInnerPath() << std::endl;
        }
    }

    const int fileToFixAmount = Settings::fileToFixAmount();
    for(int n=fileToFixAmount-1; n>=0; n--)
    {
        auto outFile = Settings::outFileToFix(n),
             srcFile = Settings::srcFileToFix(n);
        if (Settings::verbose())
            std::cout << "\n* Processing " << srcFile
                      << (outFile != srcFile ? std::string(" into ") + outFile : "")
                      << std::endl;
        copyFile(srcFile, outFile); // to set write permission or move
        changeLibPathsOnFile(outFile);
        fixRpathsOnFile(outFile, outFile);
        adhocCodeSign(outFile);
        if (Settings::verbose())
            std::cout << "\n-- Done Processing dependency for "
                      << srcFile << std::endl;
    }
}

bool hasFrameworkDep() {
    return deps.end() != std::find_if(deps.begin(), deps.end(),[](Dependency &dep){
        return dep.isFramework();
    });
}

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

    if (hasFrameworkDep() && !fs::create_directory(cont / "Frameworks", err))
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
