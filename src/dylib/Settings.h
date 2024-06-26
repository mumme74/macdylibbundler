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

namespace Settings
{
void init(int argc, const char* argv[]);

bool isSystemLibrary(const std::string& prefix);
bool isPrefixBundled(const std::string& prefix);
bool isPrefixIgnored(const std::string& prefix);
void ignore_prefix(std::string prefix);

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
const std::string frameworkDir();

const std::string destFolder();
void setDestFolder(const std::string& path);

/// For cross compiling prefix otool and install_name_tool
/// with prefix
const std::string prefixTools();
/// Set cross compiling tool prefix
void setPrefixTools(const std::string& prefixTools);
/// give absolute path to otool
void setOToolPath(const std::string& otoolPath);
/// give absolute path to install_name_tool
void setInstallNameToolPath(const std::string& installNamePath);
/// get cmd to use for otool
const std::string otoolCmd();
/// get cmd to use for install_name_tool
const std::string installNameToolCmd();

const std::string codeSign();
void setCodeSign(const std::string& codesign);

/// if we should create a app bundle
bool createAppBundle();
void setCreateAppBundle(bool on);
/// The path to the app bundle
/// ie: name.app
const std::filesystem::path appBundlePath();
/// The path to the app
void setAppBundlePath(const std::string& path);
/// Add a script which is run after appBundle is complete
void setAppBundleScript(const std::filesystem::path script);
/// reference to all app bundle scripts which should run after appBundle is complete
std::vector<std::filesystem::path>& appBundleScripts();
/// prevent all appBundle scripts from beeing run
void preventAppBundleScripts();
/// only run script, nothing else
void setOnlyRunScripts();
/// If we should only run scripts
bool shouldOnlyRunScripts();
/// Get the path to Contents directory in bundle
///  example: name.app/Contents
const std::filesystem::path appBundleContentsDir();
/// Get the path to exec directory in bundle
///  example: name.app/Contents/MacOS
const std::filesystem::path appBundleExecDir();
/// Dir to app common scripts
const std::filesystem::path& scriptDir();
/// Path to Info.plist file
const std::filesystem::path& infoPlist();
bool setInfoPlist(const std::string& plist);

void addFileToFix(const std::string& path);
int fileToFixAmount();
const std::string srcFileToFix(const int n);
const std::string outFileToFix(const int n);

const std::string inside_lib_path();
const std::string inside_framework_path();
void set_inside_lib_path(const std::string& p);

void addSearchPath(const std::string& path);
int searchPathAmount();
const std::string searchPath(const int n);


bool verbose();
void setVerbose(bool on);

/// insert settings into rootObj
std::unique_ptr<Json::Object> toJson();

}
#endif
