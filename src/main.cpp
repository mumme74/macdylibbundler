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
#include "ScriptRunner.h"
#include "ArgParser.h"

/*
 TODO
 - what happens if a library is not remembered by full path but only name? (support improved, still not perfect)
 - could get mixed up if symlink and original are not in the same location (won't happen for UNIX prefixes like /usr/, but in random directories?)

 FIXME: why does it copy plugins i try to fix to the libs directory?

 */

const std::string VERSION = "1.0.5";


// FIXME - no memory management is done at all (anyway the program closes immediately so who cares?)

std::string installPath = "";

void showHelp();

ArgParser args {
  {"x","fix-file", "file to fix (executable or app plug-in)", Settings::addFileToFix, ArgItem::ReqVluString},
  {"a","app", "app bundle name, create a app bundle with this name (default name is -x name)",
    [](std::string vlu){ vlu.empty() ? Settings::setCreateAppBundle(true): Settings::setAppBundlePath(vlu);}
  },
#ifdef USE_SCRIPTS
  {"as","app-bundle-script","run this custom python script after bundle is complete, and after default scripts are runned",
    Settings::setAppBundleScript, ArgItem::ReqVluString
  },
  {nullptr, "no-app-bundle-scripts","Prevent app bundle scripts from running",Settings::preventAppBundleScripts},
  {nullptr, "only-scripts","Don't do anything more than running scripts",Settings::setOnlyRunScripts},
#endif // USE_SCRIPTS
  {"pl","app-info-plist","Optional path to a Info.plist to bundle into app", Settings::setInfoPlist, ArgItem::ReqVluString},
  {"b","bundle-deps","Bundle library dependencies.", Settings::setBundleLibs},
  {"f","bundle-frameworks", "Bundle frameworks into app bundle", Settings::setBundleFrameworks},
  {"d","dest-dir","directory to send bundled libraries (relative to fix-file)",Settings::setDestFolder, ArgItem::ReqVluString},
  {"p","install-path","'inner' path of bundled libraries (usually relative to executable, by default '@executable_path/../libs/')",
   Settings::set_inside_lib_path, ArgItem::ReqVluString
  },
  {"s","search-path","Directory to add to list of locations searched",Settings::addSearchPath, ArgItem::ReqVluString},
  {"of","overwite-files","allow overwriting files in output directory",Settings::setCanOverwriteFiles},
  {"od","overwrite-dir","totally overwrite output directory if it already exists. implies --create-dir", Settings::setCanOverwriteDir},
  {"cd","create-dir","creates output directory if necessary",Settings::setCanCreateDir},
  {"ns","no-codesign","disables ad-hoc codesigning",Settings::setCanCodesign, ArgItem::VluFalse},
  {"i","ignore","Location to ignore (will ignore libraries in this directory)", Settings::ignore_prefix,ArgItem::ReqVluString},
  {"pt","prefix-tools","'prefix' otool and install_name_tool with prefix (for cross compilation)", Settings::setPrefixTools,ArgItem::ReqVluString},
  {nullptr, "otool-path","give the path to otool or llvm-otool, useful when tools not in path", Settings::setOToolPath,ArgItem::ReqVluString},
  {nullptr, "install-name-tool-path","absolute path to install_name_tool, useful when not in path",Settings::setInstallNameToolPath,ArgItem::ReqVluString},
  {"cs","codesign","path to codesigning binary, might be zsign for example",Settings::setCodeSign,ArgItem::ReqVluString},
  {"v","verbose","verbose mode",Settings::setVerbose},
  {"h","help","Show help",showHelp}
};

void showHelp() {
  std::cout << args.programName() << " " << VERSION << std::endl
            << args.programName() << " is a utility that helps bundle dynamic libraries inside macOS app bundles.\n" << std::endl;
  args.help();
  std::cout << "\n\nEnvironment variable DYLIBBUNDLER_SCRIPTS_PATH="
            << "<path to dir with custom python scripts runned after bundle is done, separated by ':'>" << std::endl;
  exit(0);
}

int main (int argc, const char * argv[])
{
    Settings::init(argc, argv);
    args.parse(argc, argv);

    if(not Settings::bundleLibs() and Settings::fileToFixAmount()<1)
    {
        showHelp();
        exit(0);
    }

    std::cout << "* Collecting dependencies\n";
    DylibBundler bundler{};

    const int amount = Settings::fileToFixAmount();
    for(int n=0; n<amount; n++)
        bundler.collectDependencies(Settings::srcFileToFix(n));

    bundler.collectSubDependencies();
    if (!Settings::shouldOnlyRunScripts())
      bundler.doneWithDeps_go();
#ifdef USE_SCRIPTS
    runPythonScripts_afterHook();
#endif

    std::cout << "\n\n -- Processed " << amount << " file"
              << (amount > 1 ? "s" :"") << "." << std::endl;

    return 0;
}
