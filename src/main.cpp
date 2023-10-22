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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <vector>
#include "Settings.h"

#include "Utils.h"
#include "DylibBundler.h"

/*
 TODO
 - what happens if a library is not remembered by full path but only name? (support improved, still not perfect)
 - could get mixed up if symlink and original are not in the same location (won't happen for UNIX prefixes like /usr/, but in random directories?)

 FIXME: why does it copy plugins i try to fix to the libs directory?

 */

const std::string VERSION = "1.0.5";


// FIXME - no memory management is done at all (anyway the program closes immediately so who cares?)

std::string installPath = "";


void showHelp()
{
    std::cout << "dylibbundler " << VERSION << std::endl;
    std::cout << "dylibbundler is a utility that helps bundle dynamic libraries inside macOS app bundles.\n" << std::endl;

    std::cout << "-x, --fix-file <file to fix (executable or app plug-in)>" << std::endl;
    std::cout << "-a, --app <app bundle name, create a app bundle with this name (default name is -x name)>" << std::endl;
    std::cout << "-as, --app-bundle-script <run this custom python script after bundle is complete, and after default scripts are runned>" << std::endl;
    std::cout << "--no-app-bundle-scripts <don't run app bundle scripts after creation of app bundle>" << std::endl;
    std::cout << "-pl, --app-info-plist <Optional path to a Info.plist to bundle into app>" << std::endl;
    std::cout << "-b, --bundle-deps" << std::endl;
    std::cout << "-f, --bundle-frameworks (Bundle frameworks)" << std::endl;
    std::cout << "-d, --dest-dir <directory to send bundled libraries (relative to cwd)>" << std::endl;
    std::cout << "-p, --install-path <'inner' path of bundled libraries (usually relative to executable, by default '@executable_path/../libs/')>" << std::endl;
    std::cout << "-s, --search-path <directory to add to list of locations searched>" << std::endl;
    std::cout << "-of, --overwrite-files (allow overwriting files in output directory)" << std::endl;
    std::cout << "-od, --overwrite-dir (totally overwrite output directory if it already exists. implies --create-dir)" << std::endl;
    std::cout << "-cd, --create-dir (creates output directory if necessary)" << std::endl;
    std::cout << "-ns, --no-codesign (disables ad-hoc codesigning)" << std::endl;
    std::cout << "-i, --ignore <location to ignore> (will ignore libraries in this directory)" << std::endl;
    std::cout << "-pt, --prefix-tools <'prefix' otool and install_name_tool with prefix, for cross compilation>" << std::endl;
    std::cout << "-cs, --codesign <path to codesigning binary, might be zsign for example>" << std::endl;
    std::cout << "-v, --verbose (verbose mode)" << std::endl;
    std::cout << "-h, --help" << std::endl;
    std::cout << "\n\nEnvironment variable DYLIBBUNDLER_SCRIPTS_PATH=<path to dir with custom python scripts runned after bundle is done, separated by ':'>" << std::endl;
}

int main (int argc, const char * argv[])
{
    Settings::init(argc, argv);

    // parse arguments
    for(int i=0; i<argc; i++)
    {
        if(strcmp(argv[i],"-x")==0 or strcmp(argv[i],"--fix-file")==0)
        {
            i++;
            Settings::addFileToFix(argv[i]);
            continue;
        }
        else if(strcmp(argv[i],"-a")==0 or strcmp(argv[i], "--app")==0)
        {
            if (argc > i+1 && argv[i+1][0]!='-') {
                i++;
                Settings::setAppBundlePath(argv[i]);
            } else {
                Settings::setCreateAppBundle(true);
            }
            continue;
        }
        else if(strcmp(argv[i], "-as")==0 or strcmp(argv[i], "--app-bundle-script")==0)
        {
            if (argc > i+1 && argv[i+1][0]!='-') {
                i++;
                Settings::setAppBundleScript(argv[i]);
                continue;
            } else {
                std::cerr << "--app-bundle-script requires a argument!" << std::endl;
                exit(1);
            }
        }
        else if(strcmp(argv[i],"--no-app-bundle-scripts")==0)
        {
            Settings::preventAppBundleScripts();
            continue;
        }
        else if (strcmp(argv[i],"-pl")==0 or strcmp(argv[i],"--app-info-plist")==0)
        {
            if (argc > i+1 && argv[i+1][0]!='-') {
                i++;
                if (Settings::setInfoPlist(argv[i]))
                    continue;
            }
            std::cerr << "--app-info-plist requires a valid Info.plist file." << std::endl;
            exit(1);
        }
        else if(strcmp(argv[i],"-b")==0 or strcmp(argv[i],"--bundle-deps")==0)
        {
            Settings::bundleLibs(true);
            continue;
        }
        else if(strcmp(argv[i],"-p")==0 or strcmp(argv[i],"--install-path")==0)
        {
            i++;
            Settings::inside_lib_path(argv[i]);
            continue;
        }
        else if(strcmp(argv[i],"-i")==0 or strcmp(argv[i],"--ignore")==0)
        {
            i++;
            Settings::ignore_prefix(argv[i]);
            continue;
        }
        else if(strcmp(argv[i],"-d")==0 or strcmp(argv[i],"--dest-dir")==0)
        {
            i++;
            Settings::destFolder(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-f")==0 or strcmp(argv[i],"--bundle-frameworks")==0)
        {
            Settings::setBundleFrameworks(true);
            continue;
        }
        else if(strcmp(argv[i],"-of")==0 or strcmp(argv[i],"--overwrite-files")==0)
        {
            Settings::canOverwriteFiles(true);
            continue;
        }
        else if(strcmp(argv[i],"-od")==0 or strcmp(argv[i],"--overwrite-dir")==0)
        {
            Settings::canOverwriteDir(true);
            Settings::canCreateDir(true);
            continue;
        }
        else if(strcmp(argv[i],"-pt")==0 or strcmp(argv[i],"--prefix-tools")==0)
        {
            i++;
            Settings::setPrefixTools(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-cs")==0 or strcmp(argv[i],"--codesign")==0)
        {
           i++;
           Settings::setCodeSign(argv[i]);
           continue;
        }
        else if(strcmp(argv[i],"-cd")==0 or strcmp(argv[i],"--create-dir")==0)
        {
            Settings::canCreateDir(true);
            continue;
        }
        else if(strcmp(argv[i],"-ns")==0 or strcmp(argv[i],"--no-codesign")==0)
        {
            Settings::canCodesign(false);
            continue;
        }
        else if(strcmp(argv[i],"-h")==0 or strcmp(argv[i],"--help")==0)
        {
            showHelp();
            exit(0);
        }
        else if(strcmp(argv[i],"-s")==0 or strcmp(argv[i],"--search-path")==0)
        {
            i++;
            Settings::addSearchPath(argv[i]);
            continue;
        }
        else if(strcmp(argv[i],"-v")==0 or strcmp(argv[i],"--verbose")==0)
        {
            Settings::verbose(true);
            continue;
        }
        else if(i>0)
        {
            // if we meet an unknown flag, abort
            // ignore first one cause it's usually the path to the executable
            showHelp();
            std::cerr << "\n**Unknown flag " << argv[i] << std::endl << std::endl;

            exit(1);
        }
    }

    if(not Settings::bundleLibs() and Settings::fileToFixAmount()<1)
    {
        showHelp();
        exit(0);
    }

    std::cout << "* Collecting dependencies"; fflush(stdout);

    const int amount = Settings::fileToFixAmount();
    for(int n=0; n<amount; n++)
        collectDependencies(Settings::srcFileToFix(n));

    collectSubDependencies();
    doneWithDeps_go();

    std::cout << "\n\n -- Processed " << amount << " file"
              << (amount > 1 ? "s" :"") << "." << std::endl;

    return 0;
}
