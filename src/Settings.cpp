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

#include "Settings.h"
#include <vector>
#include <sstream>

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


namespace Settings
{

void init() {
    initSearchPaths();
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
const std::string destFolder(){ return dest_folder_str; }
void destFolder(const std::string& path)
{
    dest_folder_str = path;
    // fix path if needed so it ends with '/'
    if( dest_folder_str[ dest_folder_str.size()-1 ] != '/' ) dest_folder_str += "/";
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
const std::string fileToFix(const int n){ return files[n]; }

std::string inside_path_str = "@executable_path/../libs/";
const std::string inside_lib_path(){ return inside_path_str; }
void inside_lib_path(const std::string& p)
{
    inside_path_str = p;
    // fix path if needed so it ends with '/'
    if( inside_path_str[ inside_path_str.size()-1 ] != '/' ) inside_path_str += "/";
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
    if(prefix.find(".framework") != std::string::npos) return false;
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

}
