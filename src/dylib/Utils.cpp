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


#include "Utils.h"
#include "Settings.h"
#include "Common.h"
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

void setWritable(PathRef file, bool isWritable)
{
    std::error_code err;
    using prm = std::filesystem::perms;
    using prmOpt = std::filesystem::perm_options;
    std::filesystem::permissions(file,
        prm::owner_write | prm::group_write | prm::group_write,
        isWritable ? prmOpt::add : prmOpt::remove,
        err
    );
    if (err) {
        std::stringstream ss;
        ss << "\n\nError : An error occurred while trying to "
           << "change write permissions on file " << file
           << " to " << (isWritable ? "+" : "-") << "w\n"
           << "  error: " << err.message() << "\n";
        exitMsg(ss.str());
    }
}

void copyFile(PathRef from, PathRef to)
{
    std::stringstream ss;
    bool override = Settings::canOverwriteFiles();
    if( from != to && !override )
    {
        if(fs::exists(to)) {
            ss << "\n\nError : File " << to <<" already exists. "
               << "Remove it or enable overwriting.";
            exitMsg(ss.str());
        }
    }

    // copy file to local directory
    std::error_code err;
    if (!std::filesystem::copy_file(from, to, err)) {
        ss << "\n\nError : An error occurred while trying to copy "
           << "file " << from << " to " << to << " err: "
           << err.message() << "\n";
        exitMsg(ss.str());
    }

    setWritable(to, true);
}

std::string system_get_output(std::string_view cmd)
{
    FILE * command_output = nullptr;
    char output[128];
    int amount_read = 1,
        return_value = 0;

    std::string full_output;

    try
    {
        command_output = popen(cmd.data(), "r");
        if(command_output == NULL) throw;

        while(amount_read > 0)
        {
            amount_read = fread(output, 1, 127, command_output);
            if(amount_read <= 0) break;
            else
            {
                output[amount_read] = '\0';
                full_output += output;
            }
        }
    }
    catch(...)
    {
        std::cerr << "An error occured while executing command "
                  << cmd.data() << std::endl;
        full_output = "";
    }

    if (command_output != nullptr)
        return_value = pclose(command_output);
    if (return_value != 0)
        full_output = "";

    return full_output;
}

int systemp(std::string_view cmd)
{
    if (Settings::verbose())
        std::cout << "    " << cmd << std::endl;
    return system(cmd.data());
}

void changeInstallName(
    PathRef binary_path,
    PathRef old_path,
    PathRef new_path
) {
    std::stringstream ss;
    ss << Settings::installNameToolCmd() + " -change \""
       << old_path << "\" \"" << new_path << "\" \""
       << binary_path << "\"";

    if( systemp(ss.str()) != 0 ) {
        ss << "\n\nError: An error occurred while trying to fix "
           << "dependencies of " << binary_path << "\n";
        exitMsg(ss.str());
    }
}

Path getUserInputDirForFile(PathRef filename)
{
    for(const auto& searchPath : Settings::searchPaths())
    {
        auto path = searchPath / filename;
        if(fs::exists(path))
        {
            std::cerr << (path) << " was found. /!\\ DYLIBBUNDLER "
                      << "MAY NOT CORRECTLY HANDLE THIS DEPENDENCY:"
                      << " Manually check the executable with '"
                      << Settings::otoolCmd() << " -L'" << std::endl;
            return searchPath;
        }
    }

    while (true)
    {
        std::cout << "Please specify the directory where this "
                  << "library is located (or enter 'quit' to abort): ";
        fflush(stdout);

        std::string prefix;
        std::cin >> prefix;
        std::cout << std::endl;

        if(prefix.compare("quit")==0) exit(1);
        auto path = fs::path(prefix) / filename;

        if(!fs::exists(path))
        {
            std::cerr << path << " does not exist. Try again"
                      << std::endl;
            continue;
        }
        else
        {
            std::cerr << path << " was found. /!\\ DYLIBBUNDLER "
                      << "MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: "
                      << "Manually check the executable with '"
                      << Settings::otoolCmd() << " -L'" << std::endl;
            Settings::addSearchPath(prefix);
            return Path(prefix);
        }
    }
}

void createFolder(PathRef folder) {
    std::error_code err;
    std::stringstream ss;
        // ----------- check dest folder stuff ----------
    bool dest_exists = fs::exists(folder, err);
    if(dest_exists and Settings::canOverwriteDir())
    {
        std::cout << "* Erasing old directory "
                  << folder << std::endl;
        fs::remove_all(folder, err);
        if(err) {
            ss << "\n\nError : An error occurred while attempting to overwrite dest folder."
               << " error: " << err.message() << "\n";
            exitMsg(ss.str());
        }

        dest_exists = false;
    }

    if(!dest_exists)
    {
        if(Settings::canCreateDir() || Settings::createAppBundle())
        {
            std::cout << "* Creating directory "
                      << folder << std::endl;
            fs::create_directories(folder, err);
            if (err) {
                ss << "\n\nError : An error occurred while creating dest folder."
                   << " error: " << err.message() << "\n";
                exitMsg(ss.str());
            }

        }
        else
            exitMsg("\n\nError : Destination folder does not exist."
                    " Create it or pass the appropriate flag for "
                    "automatic dest folder creation.");
    }
}

void adhocCodeSign(PathRef file)
{
    if( Settings::canCodesign() == false ) return;
    if( Settings::verbose() )
        std::cout << "Signing '" << file << "'" << std::endl;

    // Add ad-hoc signature for ARM (Apple Silicon) binaries
    std::stringstream ss;
    ss << Settings::codeSign()
       << " --force --deep --preserve-metadata=entitlements"
       <<",requirements,flags,runtime --sign - \"" << file << "\"";

    auto signCommand = ss.str();
    if( systemp(signCommand) != 0 )
    {
        // If the codesigning fails, it may be a bug in Apple's codesign utility.
        // A known workaround is to copy the file to another inode, then move it back
        // erasing the previous file. Then sign again.
        std::cerr << "  * Error : An error occurred while applying "
                  << "ad-hoc signature to " << file
                  << ". Attempting workaround" << std::endl;

        auto machine = system_get_output("machine");
        bool isArm = machine.find("arm") != std::string::npos;
        auto tempDirTemplate = std::string(std::getenv("TMPDIR") +
                               std::string("dylibbundler.XXXXXXXX"));
        std::string filename = fs::path(file).filename();
        char* tmpDirCstr = mkdtemp((char *)(tempDirTemplate.c_str()));
        if( tmpDirCstr == NULL )
        {
            std::cerr << "  * Error : Unable to create temp "
                      << "directory for signing workaround"
                      << std::endl;
            if( isArm )
            {
                exit(1);
            }
        }
        std::string tmpDir = std::string(tmpDirCstr);
        std::string tmpFile = tmpDir+"/"+filename;
        const auto runCommand = [isArm](const std::string& command, const std::string& errMsg)
        {
            if( systemp( command ) != 0 )
            {
                std::cerr << errMsg << std::endl;
                if( isArm )
                {
                    exit(1);
                }
            }
        };
        std::stringstream err, cmd;
        cmd << "cp -p \"" << file << "\" \"" << tmpFile << "\"";
        err << "  * Error : An error occurred copying " << file
            << " to " << tmpDir;
        runCommand(cmd.str(), err.str());

        cmd << "mv -f \"" << tmpFile << "\" \"" << file << "\"";
        err << "  * Error : An error occurred moving " << tmpFile
            << " to " << file;
        runCommand(cmd.str(), err.str());

        cmd << "rm -rf \"" << tmpDir << "\"";
        systemp(cmd.str());
        err << "  * Error : An error occurred while applying "
            << "ad-hoc signature to " << file;
        runCommand(signCommand, err.str());
    }
}

bool isExecutable(PathRef path)
{
    namespace fs = std::filesystem;
    using fs::perms;
    auto perm = fs::status(path).permissions();
    if (fs::is_regular_file(path) &&
        ((perms::none != (perm & perms::owner_exec)) ||
            (perms::none != (perm & perms::group_exec)) ||
            (perms::none != (perm & perms::others_exec)))
    ) {
        return true;
    }
    return false;
}
