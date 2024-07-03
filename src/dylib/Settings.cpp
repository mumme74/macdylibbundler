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
#include <regex>
#include <unistd.h>
#include <sys/wait.h>
#include "Settings.h"
#include "Common.h"
#include "Utils.h"

namespace fs = std::filesystem;

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

    auto paths = tokenize(scPaths, ":");
    std::vector<std::string> skip{"__init__.py", "common.py"};

    for (auto &path : paths) {
        for (auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::find(skip.begin(), skip.end(), entry.path().filename()) != skip.end())
            { continue; } // skip

            if (isExecutable(Path(entry.path()))) {
                Settings::setAppBundleScript(entry.path().string());
            }
        }
    }
}

std::string lookUpTool(
    const std::string& prefix,
    const std::string& tool
) {
    static std::map<std::string, std::string> cache;
    auto found = cache.find(tool);
    if (found != cache.end())
        return found->second;

    std::string toolCmd;

    auto testCmd = [&](std::string_view cmd)->bool {
        FILE* proc = popen(cmd.data(), "r");
        if (proc == nullptr)
            return false;
        int wstatus;
        if (wait(&wstatus) == -1)
            return false;
        auto code = WEXITSTATUS(wstatus);
        if (WIFEXITED(wstatus) && code < 127) {
            toolCmd = cmd;
            cache[tool] = toolCmd;
            pclose(proc);
            std::cout << toolCmd << ": found\n";
            return true;
        }
        return false;
    };

    if (testCmd(prefix + tool))
        return toolCmd;
    if (testCmd(prefix+"llvm-"+tool))
        return toolCmd;
    else if (testCmd(std::regex_replace("llvm-"+tool,
                     std::regex("_"), "-"))
    ) {
        return toolCmd;
    }
    if (testCmd("llvm-strip -V")) {
        auto output = system_get_output("llvm-strip -V");
        std::regex re{"version\\s+(\\d+)"};
        std::smatch match;
        if (std::regex_search(output, match, re)) {
            if (testCmd(std::string{"llvm-"+tool+"-"} + match.str(1)))
                return toolCmd;
            else if (testCmd(std::regex_replace(
                        "llvm-"+tool+"-"+match.str(1),
                        std::regex("_"), "-"))
            ) {
                return toolCmd;
            }
        }
    }

    std::cerr << "**Failed to find either "+tool+" or llvm-"+tool+"!\n";
    exit(1);
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

void setCanOverwriteFiles(bool permission){ overwrite_files = permission; }
void setCanOverwriteDir(bool permission){ overwrite_dir = permission; }
void setCanCreateDir(bool permission){ create_dir = permission; }
void setCanCodesign(bool permission){ codesign = permission; }

bool bundleLibs_bool = false;
bool bundleLibs(){ return bundleLibs_bool; }
void setBundleLibs(bool on){ bundleLibs_bool = on; }

Path dest_folder{"./libs/"};
Path destFolder()
{
    if (createAppBundle())
        return appBundleExecDir() /
            stripPrefix(stripLastSlash(dest_folder.string())) / "";
    return dest_folder;
}

void setDestFolder(std::string_view path)
{
    dest_folder = path;
}

std::string prefixTools_str = "";
std::string prefixTools() { return prefixTools_str; }
void setPrefixTools(std::string_view prefixTools)
{
    prefixTools_str = prefixTools;
}

std::string otool_cmd = "", install_name_cmd = "";
/// give absolute path to otool
void setOToolPath(std::string_view otoolPath)
{
    otool_cmd = otoolPath;
}
/// give absolute path to install_name_tool
void setInstallNameToolPath(std::string_view installNamePath)
{
    install_name_cmd = installNamePath;
}
/// get cmd to use for otool
std::string otoolCmd()
{
    if (otool_cmd.empty())
        return lookUpTool(prefixTools_str, "otool");
    return otool_cmd;
}
/// get cmd to use for install_name_tool
std::string installNameToolCmd()
{
    if (install_name_cmd.empty())
        return lookUpTool(prefixTools_str, "install_name_tool");
    return install_name_cmd;
}

std::string codesign_str = "codesign";
std::string codeSign() { return codesign_str; }
void setCodeSign(std::string_view codesign) { codesign_str = codesign; }

std::vector<Path> files;
void addFileToFix(std::string_view path){
    files.push_back(Path(path.data()));
}
std::vector<Files> srcFiles() {
    std::vector<Files> f;
    for (PathRef file : files) {
        Files pair;
        pair.out = createAppBundle()
                 ? appBundleExecDir() / file.filename()
                 : file;
        pair.src = file;
        f.emplace_back(pair);
    }
    return f;
}

bool create_app_bundle = false;
bool createAppBundle() { return create_app_bundle; }
void setCreateAppBundle(bool on) {
    create_app_bundle = on;
    // automatically bundle libs and frameworks if app bundle
    setBundleLibs(true);
    setBundleFrameworks(on);
}

Path app_bundle_path;
std::string appBundleName() {
    return appBundlePath().end_name();
}
Path appBundlePath() {
    auto path = app_bundle_path.empty() ?
        Path(files.front()) : app_bundle_path;
    auto name = path.end_name().string();
    if (name.rfind(".app") != name.size() -1) name+=".app";
    path.replace_filename(name);
    return path;
}
void setAppBundlePath(std::string_view path) {
    auto n = path.substr(path.find("/")+1);
    app_bundle_path = n.substr(0, n.rfind("/")-1);
    setCreateAppBundle(true);
}

Path _scriptDir{};
std::vector<Path> all_appBundleScripts;
bool scriptsPrevented = false;
bool scriptsOnly = false;
void setAppBundleScript(Path script) {
    if (!std::filesystem::exists(script)) {
        exitMsg(std::string("Script ") + script.string() + "does not exist.");

    } else if (!isExecutable(script)) {
        exitMsg(std::string("Script ") + script.string() +
                std::string(" is not executable"));
    } else if (scriptsPrevented) {
        exitMsg(std::string("Scripts are prevented, can't add ")
                + script.string());
    }
    all_appBundleScripts.push_back(Path(script));
}
/// reference to all app bundle scripts which should run after appBundle is complete
std::vector<Path>& appBundleScripts() {
    return all_appBundleScripts;
}
PathRef& scriptDir() {
    return _scriptDir;
}
void setScriptsDir(Path dir)
{
    _scriptDir = dir;
}
/// prevent all appBundle scripts from beeing run
void preventScripts() {
    scriptsPrevented = true;
}
bool shouldPreventScripts() {
    return scriptsPrevented;
}
void setOnlyRunScripts() {
    scriptsOnly = true;
}
/// If we should only run scripts
bool shouldOnlyRunScripts() {
    return scriptsOnly;
}

Path appBundleContentsDir() {
    return appBundlePath() / "Contents";
}
Path appBundleExecDir() {
    return appBundleContentsDir() / "MacOS";
}

Path plist_path;
PathRef infoPlist() { return plist_path; }
bool setInfoPlist(std::string_view plist) {
    if (std::filesystem::exists(plist)) {
        plist_path = plist;
        return true;
    }
    return false;
}

bool mayAskUser = true;
bool shouldAskUser() { return mayAskUser; }
void preventAskUser() { mayAskUser = false; }

Path inside_path;
Path inside_lib_path(){
    if (inside_path.empty()) {
        auto dir = stripLastSlash(dest_folder.string());
        if (dir[0] == '.' && (dir[1] == '.' || dir[1] == '/'))
            dir = dir.substr(2);
        else if (dir[0] == '/')
            dir = dir.substr(1);
        return std::string("@executable_path/") + dir + "/";
    }
    return inside_path;
}
void set_inside_lib_path(std::string_view p)
{
    inside_path = p;
}
Path inside_framework_path() {
    if (createAppBundle())
        return Path("@rpath/Frameworks/");
    auto path = stripLastSlash(inside_lib_path().string());
    return path.substr(0, path.find("/")) + "/Frameworks/";
}

std::vector<Path> prefixes_to_ignore;
void ignore_prefix(Path prefix)
{
    prefixes_to_ignore.push_back(prefix);
}

bool isSystemLibrary(PathRef prefix)
{
    if(prefix.upto("lib") == Path("/usr/lib")) return true;
    if(prefix.upto("Library") == Path("/System/Library")) return true;

    return false;
}

bool isPrefixIgnored(PathRef prefix)
{
    std::string sPre = prefix;
    for(const auto& pref : prefixes_to_ignore) {
        std::string sP = pref;
        if((sP.size() <= sPre.size()) &&
           (sP == sPre.substr(0, sP.size()))
        ) {
            return true;
        }
    }

    return false;
}

bool blacklistedPath(PathRef prefix)
{
    if(!Settings::bundleFrameworks() &&
        prefix.before(".framework") != prefix)
    { return true; }
    if(prefix.before("@executable_path") == "") return true;
    if(isSystemLibrary(prefix)) return true;
    if(isPrefixIgnored(prefix)) return true;

    return false;
}

std::vector<Path> _searchPaths;
void addSearchPath(Path path){
    _searchPaths.emplace_back(path);
}
const std::vector<Path>& searchPaths(){
    return _searchPaths;
}

bool is_verbose = false;
void setVerbose(bool on) { is_verbose = on; }
bool verbose() { return is_verbose; }

bool bundle_frameworks = false;
bool bundleFrameworks() { return bundle_frameworks; }
void setBundleFrameworks(bool on) { bundle_frameworks = on; }
Path frameworkDir() {
    return (bundle_frameworks ?
        appBundleContentsDir() / "Frameworks" / "" :
            Path(destFolder())).string();
}

std::unique_ptr<Json::Object> toJson() {
    using namespace Json;
    Array searchIn;
    for (const auto& path : searchPaths())
        searchIn.push(path.string());

    auto obj = std::make_unique<Object>(ObjInitializer{
        {"can_overwrite_files", Bool(canOverwriteFiles())},
        {"can_overwrite_dir", Bool(canOverwriteDir())},
        {"can_create_dir", Bool(canCreateDir())},
        {"can_codesign", Bool(canCodesign())},
        {"bundle_libs", Bool(bundleLibs())},
        {"bundle_frameworks", Bool(bundleFrameworks())},
        {"framework_dir", String(frameworkDir())},
        {"create_app_bundle", Bool(createAppBundle())},
        {"verbose", Bool(verbose())},
        {"lib_folder", String(destFolder())},
        {"prefix_tools", String(prefixTools())},
        {"app_bundle_contents_dir", String(appBundleContentsDir())},
        {"inside_lib_path", String(inside_lib_path())},
        {"inside_framework_path", String(inside_framework_path())},
        {"app_bundle_exec_dir", String(appBundleExecDir())},
        {"app_bundle_name", String(appBundleName())},
        {"search_paths", searchIn},
        {"otool_path", String(otoolCmd())},
        {"install_name_tool_path", String(installNameToolCmd())}
      }
    );
    return obj;
}

}
