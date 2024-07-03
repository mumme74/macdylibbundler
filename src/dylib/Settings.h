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

#ifndef _settings_
#define _settings_

#include <string>
#include <vector>
#include <filesystem>
#include "Json.h"
#include "Types.h"

namespace Settings
{
struct Files {
  Path src, out;
};

void init(int argc, const char* argv[]);

bool isSystemLibrary(PathRef prefix);
bool blacklistedPath(PathRef prefix);
bool isPrefixIgnored(PathRef prefix);
void ignore_prefix(Path prefix);

bool canOverwriteFiles();
void setCanOverwriteFiles(bool permission);

bool canOverwriteDir();
void setCanOverwriteDir(bool permission);

bool canCreateDir();
void setCanCreateDir(bool permission);

bool canCodesign();
void setCanCodesign(bool permission);

bool bundleLibs();
void setBundleLibs(bool on);
/// If we should bundle frameworks
bool bundleFrameworks();
/// Set if we should bundle frameworks
void setBundleFrameworks(bool on);
/// path to Frameworks dir in app bundle or same as libs/ folder
Path frameworkDir();

Path destFolder();
void setDestFolder(std::string_view path);

/// For cross compiling prefix otool and install_name_tool
/// with prefix
std::string prefixTools();
/// Set cross compiling tool prefix
void setPrefixTools(std::string_view prefixTools);
/// give absolute path to otool
void setOToolPath(std::string_view otoolPath);
/// give absolute path to install_name_tool
void setInstallNameToolPath(std::string_view installNamePath);
/// get cmd to use for otool
std::string otoolCmd();
/// get cmd to use for install_name_tool
std::string installNameToolCmd();

std::string codeSign();
void setCodeSign(std::string_view codesign);

/// if we should create a app bundle
bool createAppBundle();
void setCreateAppBundle(bool on);
/// The path to the app bundle
/// ie: name.app
Path appBundlePath();
/// The path to the app
void setAppBundlePath(std::string_view path);
/// Add a script which is run after appBundle is complete
void setAppBundleScript(Path script);
/// reference to all app bundle scripts which should run after appBundle is complete
std::vector<Path>& appBundleScripts();
/// prevent all appBundle scripts from beeing run
void preventScripts();
bool shouldPreventScripts();
/// only run script, nothing else
void setOnlyRunScripts();
/// If we should only run scripts
bool shouldOnlyRunScripts();
/// Get the path to Contents directory in bundle
///  example: name.app/Contents
Path appBundleContentsDir();
/// Get the path to exec directory in bundle
///  example: name.app/Contents/MacOS
Path appBundleExecDir();
/// The name to app bundle
std::string appBundleName();
/// Dir to app common scripts
PathRef scriptDir();
/// Path to Info.plist file
PathRef infoPlist();
bool setInfoPlist(std::string_view plist);
/// Prevent us from asking user for input
bool shouldAskUser();
void preventAskUser();

void addFileToFix(std::string_view path);
std::vector<Files> srcFiles();

Path inside_lib_path();
Path inside_framework_path();
void set_inside_lib_path(std::string_view p);

void addSearchPath(Path path);
const std::vector<Path>& searchPaths();


bool verbose();
void setVerbose(bool on);

/// insert settings into rootObj
std::unique_ptr<Json::Object> toJson();

}
#endif
