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

#include <filesystem>
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>
#include "Settings.h"
#include "Utils.h"

//initialize the dylib search paths
void initSearchPaths(){
    //Check the same paths the system would search for dylibs
    std::string searchPaths;
    char *dyldLibPath = std::getenv("DYLD_LIBRARY_PATH");
    if (dyldLibPath != nullptr)
        searchPaths = dyldLibPath;
    dyldLibPath = std::getenv("DYLD_FALLBACK_FRAMEWORK_PATH");
    if (dyldLibPath != nullptr)
    {
        if (!searchPaths.empty() && searchPaths[ searchPaths.size()-1 ] != ':') searchPaths += ":";
        searchPaths += dyldLibPath;
    }
    dyldLibPath = std::getenv("DYLD_FALLBACK_LIBRARY_PATH");
    if (dyldLibPath != nullptr)
    {
        if (!searchPaths.empty() && searchPaths[ searchPaths.size()-1 ] != ':') searchPaths += ":";
        searchPaths += dyldLibPath;
    }
    if (!searchPaths.empty())
    {
        std::stringstream ss(searchPaths);
        std::string item;
        while(std::getline(ss, item, ':'))
        {
            if (item[ item.size()-1 ] != '/') item += "/";
            Settings::addSearchPath(item);
        }
    }
}

void initAppBundleScripts(int argc, const char* argv[]) {
    (void)argc;
    std::string scPaths = (std::filesystem::weakly_canonical(argv[0]).parent_path() / "scripts").string();
    const char* pscPaths = std::getenv("DYLIBBUNDLER_SCRIPTS_PATH");
    // else get dir of the executable
    if (pscPaths != nullptr)
        scPaths = pscPaths;

    std::vector<std::string> paths;
    const char delim = ':';
    tokenize(scPaths, &delim, &paths);

    std::vector<std::string> skip{"__init__.py", "common.py"};

    for (auto &path : paths) {
        for (auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::find(skip.begin(), skip.end(), entry.path().filename()) != skip.end())
            { continue; } // skip
            if (entry.path().extension() == ".py")
                Settings::setAppBundleScript(entry.path());
        }
    }

}


namespace Settings
{

void init(int argc, const char* argv[]) {
    initSearchPaths();
    initAppBundleScripts(argc, argv);
}

bool overwrite_files = false;
bool overwrite_dir = false;
bool create_dir = false;
bool codesign = true;

bool canOverwriteFiles(){ return overwrite_files; }
bool canOverwriteDir(){ return overwrite_dir; }
bool canCreateDir(){ return create_dir; }
bool canCodesign(){ return codesign; }

void canOverwriteFiles(bool permission){ overwrite_files = permission; }
void canOverwriteDir(bool permission){ overwrite_dir = permission; }
void canCreateDir(bool permission){ create_dir = permission; }
void canCodesign(bool permission){ codesign = permission; }

bool bundleLibs_bool = false;
bool bundleLibs(){ return bundleLibs_bool; }
void bundleLibs(bool on){ bundleLibs_bool = on; }

std::string dest_folder_str = "./libs/";
const std::string destFolder()
{
    if (createAppBundle())
        return appBundleExecDir() /
            stripPrefix(stripLastSlash(dest_folder_str)) / "";
    return dest_folder_str;
}

void destFolder(const std::string& path)
{
    dest_folder_str = path;
    // fix path if needed so it ends with '/'
    if( dest_folder_str[ dest_folder_str.size()-1 ] != '/' )
        dest_folder_str += "/";
}

std::string prefixTools_str = "";
const std::string prefixTools() { return prefixTools_str; }
void setPrefixTools(const std::string& prefixTools)
{
    prefixTools_str = prefixTools;
}

std::string codesign_str = "codesign";
const std::string codeSign() { return codesign_str; }
void setCodeSign(const std::string &codesign) { codesign_str = codesign; }

std::vector<std::string> files;
void addFileToFix(const std::string& path){ files.push_back(path); }
int fileToFixAmount(){ return files.size(); }
const std::string srcFileToFix(const int n) { return files[n]; }
const std::string outFileToFix(const int n) {
    if (createAppBundle()) {
        auto file = std::filesystem::path(files[n]).filename();
        return appBundleExecDir() / file;
    }
    return files[n];
}

bool create_app_bundle = false;
bool createAppBundle() { return create_app_bundle; }
void setCreateAppBundle(bool on) {
    create_app_bundle = on;
    // automatically bundle libs and frameworks if app bundle
    bundleLibs(true);
    setBundleFrameworks(on);
}

std::filesystem::path app_bundle_path;
const std::filesystem::path appBundlePath() {
    auto path = app_bundle_path.empty() ?
        std::filesystem::path(files.front()) : app_bundle_path;
    auto name = path.filename().string();
    if (name.rfind(".app") != name.size() -1) name+=".app";
    path.replace_filename(name);
    return path;
}
void setAppBundlePath(const std::string& path) {
    auto n = path.substr(path.find("/")+1);
    app_bundle_path = n.substr(0, n.rfind("/")-1);
    setCreateAppBundle(true);
}

std::filesystem::path _scriptDir;
std::vector<std::filesystem::path> all_appBundleScripts;
bool scriptsPrevented = false;
void setAppBundleScript(const std::filesystem::path script) {
    if (!std::filesystem::exists(script)) {
        exitMsg(std::string("Script ") + script.string() + "does not exist.");

    } else if (script.extension() != ".py") {
        exitMsg(std::string("Script ") + script.string() + " ext:"
                + script.extension().string() + "is not a *.py script.");
    } else if (scriptsPrevented) {
        exitMsg(std::string("Scripts are prevented, can't add ")
                + script.string());
    }
    all_appBundleScripts.push_back(std::filesystem::path(script));
}
/// reference to all app bundle scripts which should run after appBundle is complete
std::vector<std::filesystem::path>& appBundleScripts() {
    return all_appBundleScripts;
}
/// prevent all appBundle scripts from beeing run
void preventAppBundleScripts() {
    scriptsPrevented = true;
}
const std::filesystem::path appBundleContentsDir() {
    return appBundlePath() / "Contents" / "";
}
const std::filesystem::path appBundleExecDir() {
    return appBundleContentsDir() / "MacOS" / "";
}
const std::filesystem::path& scriptDir() {
    return _scriptDir;
}

std::filesystem::path plist_path;
const std::filesystem::path& infoPlist() { return plist_path; }
bool setInfoPlist(const std::string& plist) {
    if (std::filesystem::exists(plist)) {
        plist_path = plist;
        return true;
    }
    return false;
}

std::string inside_path_str;
const std::string inside_lib_path(){
    if (inside_path_str.empty()) {
        auto dir = stripLastSlash(dest_folder_str);
        if (dir[0] == '.' && (dir[1] == '.' || dir[1] == '/'))
            dir = dir.substr(2);
        else if (dir[0] == '/')
            dir = dir.substr(1);
        return std::string("@executable_path/") + dir + "/";
    }
    return inside_path_str;
}
void inside_lib_path(const std::string& p)
{
    inside_path_str = p;
    // fix path if needed so it ends with '/'
    if( inside_path_str[ inside_path_str.size()-1 ] != '/' )
        inside_path_str += "/";
}
const std::string inside_framework_path() {
    if (createAppBundle())
        return "@rpath/Frameworks/";
    auto path = stripLastSlash(inside_lib_path());
    return path.substr(0, path.find("/")) + "/Frameworks/";
}

std::vector<std::string> prefixes_to_ignore;
void ignore_prefix(std::string prefix)
{
    if( prefix[ prefix.size()-1 ] != '/' ) prefix += "/";
    prefixes_to_ignore.push_back(prefix);
}

bool isSystemLibrary(const std::string& prefix)
{
    if(prefix.find("/usr/lib/") == 0) return true;
    if(prefix.find("/System/Library/") == 0) return true;

    return false;
}

bool isPrefixIgnored(const std::string& prefix)
{
    const int prefix_amount = prefixes_to_ignore.size();
    for(int n=0; n<prefix_amount; n++)
    {
        if(prefix.compare(prefixes_to_ignore[n]) == 0) return true;
    }

    return false;
}

bool isPrefixBundled(const std::string& prefix)
{
    if(!Settings::bundleFrameworks() &&
        prefix.find(".framework") != std::string::npos)
    { return false; }
    if(prefix.find("@executable_path") != std::string::npos) return false;
    if(isSystemLibrary(prefix)) return false;
    if(isPrefixIgnored(prefix)) return false;

    return true;
}

std::vector<std::string> searchPaths;
void addSearchPath(const std::string& path){
  if( path[path.size() - 1] != '/' )
    searchPaths.push_back(path + "/");
  else
    searchPaths.push_back(path);
}
int searchPathAmount(){ return searchPaths.size(); }
const std::string searchPath(const int n){ return searchPaths[n]; }

bool is_verbose = false;
void verbose(bool on) { is_verbose = on; }
bool verbose() { return is_verbose; }

bool bundle_frameworks = false;
bool bundleFrameworks() { return bundle_frameworks; }
void setBundleFrameworks(bool on) { bundle_frameworks = on; }
const std::string frameworkDir() {
    return bundle_frameworks ?
        appBundleContentsDir() / "Frameworks" / "" :
            std::filesystem::path(destFolder());
}

std::unique_ptr<json::Object> toJson() {
    using namespace json;
    auto obj = std::make_unique<Object>(
      std::initializer_list<std::pair<std::string, VluBase>>{
        {"canOverWriteFiles", Bool(canOverwriteFiles())},
        {"canOverWriteDir", Bool(canOverwriteDir())},
        {"canCreateDir", Bool(canCreateDir())},
        {"canCodeSign", Bool(canCodesign())},
        {"bundleLibs", Bool(bundleLibs())},
        {"bundleFrameworks", Bool(bundleFrameworks())},
        {"frameworkDir", String(frameworkDir())},
        {"createAppBundle", Bool(createAppBundle())},
        {"verbose", Bool(verbose())},
        {"libFolder", String(destFolder())},
        {"prefixTools", String(prefixTools())},
        {"appBundleContentsDir", String(appBundleContentsDir())},
        {"inside_lib_path", String(inside_lib_path())},
        {"inside_framework_path", String(inside_framework_path())}
      }
    );
    return obj;
}

}
