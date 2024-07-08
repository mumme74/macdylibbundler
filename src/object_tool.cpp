/*
The MIT License (MIT)

Copyright (c) 2023 Fredrik Johansson @ github.com/mumme74

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

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include "ArgParser.h"
#include "Types.h"
#include "MachO.h"

struct inputs {

  enum Actions {
    Help, ListCmds, RPaths, WeakLoad, ReexportLoad, Load,
    AllPaths, ExtractTo, ChangeRPaths, ChangeDylibPaths,
    TargetInfo
  };
  static void setInputFile(std::string path) {
    inputFile = Path(path);
  }

  static void setOutputFile(std::string path) {
    outputFile = Path(path);
  }

  static void setAction(Actions action) {
    inputs::action = action;
  }

  static void setOverwrite(bool overwrite) {
    inputs::overwrite = overwrite;
  }

  static void setOverwriteInput(bool overwrite) {
    inputs::overwriteInputFile = overwrite;
  }
  static void setArch(std::string arch) {
    inputs::arch = arch;
  }
  static void setOldPath(std::string path) {
    inputs::oldPath = path;
  }
  static void setNewPath(std::string path) {
    inputs::newPath = path;
  }

  static Actions action;

  static Path inputFile,
              outputFile;

  static bool overwrite,
              overwriteInputFile;
  static std::string arch, oldPath, newPath;
};
bool inputs::overwrite = false;
bool inputs::overwriteInputFile = false;
Path inputs::inputFile{};
Path inputs::outputFile{};
inputs::Actions inputs::action{inputs::Help};
std::string inputs::arch{};
std::string inputs::oldPath{};
std::string inputs::newPath{};

void showHelp();

ArgParser args {
  {
    "i","input file",
    "file to inspect (executable or app plug-in)",
    inputs::setInputFile, ArgItem::ReqVluString
  },
  {
    "o","output file", "output changes to",
    inputs::setOutputFile, ArgItem::ReqVluString
  },
  {
    "l","list-cmds", "print all load commands for this file",
    [](){
      inputs::setAction(inputs::ListCmds);
    }
  },
  {
    "r","list-relative-paths", "print all relative paths for input",
    [](){
      inputs::setAction(inputs::RPaths);
    }
  },
  {
    "e","list-reexport-paths", "print all reexported paths for input",
    [](){
      inputs::setAction(inputs::ReexportLoad);
    }
  },
  {
    "L","list-load-paths", "print all load paths for input",
    [](){
      inputs::setAction(inputs::ReexportLoad);
    }
  },
  {
    "a","list-all-paths", "print all paths for input",
    [](){
      inputs::setAction(inputs::AllPaths);
    }
  },
  {
    nullptr, "change-rpath", "Change rpath in binary",
    [](){
      inputs::setAction(inputs::ChangeRPaths);
    }
  },
  {
    nullptr, "change-dylib-path", "Change dylib path in binary",
    [](){
      inputs::setAction(inputs::ChangeDylibPaths);
    }
  },
  {
    nullptr,"extract-arch", "Extract a mach-o object from a fat binary."
    " On non fat binaries it works just like copy",
    [](){
      inputs::setAction(inputs::ExtractTo);
    }
  },
  {
    nullptr,"target-info", "Print target info.",
    [](){
      inputs::setAction(inputs::TargetInfo);
    }
  },
  {
    nullptr,"arch", "Select this architecture in a fat binary. "
                    "Default is first arch in file.",
    inputs::setArch, ArgItem::ReqVluString
  },
  {
    nullptr,"old-path", "Set the path to look for to change",
    inputs::setOldPath, ArgItem::ReqVluString
  },
  {
    nullptr,"new-path", "Change to this path",
    inputs::setNewPath, ArgItem::ReqVluString
  },
  {
    nullptr, "force-overwrite",
    "overwrite output file",
    inputs::setOverwrite, ArgItem::VluTrue},
  {
    nullptr, "force-overwrite-src",
    "overwrite source file with changes if in and out are the same",
    inputs::setOverwriteInput, ArgItem::VluTrue
  },
  {"h","help","Show help",showHelp}
};

void showHelp() {
  std::cout << args.programName() << " " << std::endl
            << args.programName() << " is a utility to inspect, change mach-o binaries.\n" << std::endl;
  args.help();
  exit(0);
}

namespace fs = std::filesystem;

int writeToFile(MachO::mach_object& obj)
{
  std::ofstream file;
  file.open(inputs::outputFile.string(), std::ios::out);
  if (file.bad()) {
    std::cerr << "Failed to open file '" << inputs::outputFile << "'\n";
  } else if (obj.write(file)) {
    std::cout << "Written to " << inputs::outputFile << "\n";
    return 0;
  }
  std::cout << "Failed to write to " << inputs::outputFile << "\n";
  return 2;
}

int runAction(MachO::mach_object& obj, inputs::Actions action)
{
  switch (action) {
  case inputs::RPaths: {
    std::cout << "LC_RPATH\n";
    for (const auto& path : obj.rpaths())
      std::cout << "  " << path << "\n";
  } break;
  case inputs::ReexportLoad: {
    std::cout << "LC_REEXPORT\n";
    for (const auto& path : obj.reexportDylibPaths())
      std::cout << "  " << path << "\n";
  } break;
  case inputs::WeakLoad: {
    std::cout << "LC_WEAK_LOAD\n";
    for (const auto& path : obj.weakLoadDylib())
      std::cout << "  " << path << "\n";
  } break;
  case inputs::Load: {
    std::cout << "LC_LOAD\n";
    for (const auto& path : obj.loadDylibPaths())
      std::cout << "  " << path << "\n";
  } break;
  case inputs::AllPaths:
    runAction(obj, inputs::Load);
    runAction(obj, inputs::WeakLoad);
    runAction(obj, inputs::ReexportLoad);
    runAction(obj, inputs::RPaths);
    break;
  case inputs::ListCmds: {
    MachO::introspect_object insp{&obj};
    std::cout << "Load command for: " << inputs::inputFile << "\n";
    std::cout << insp.loadCmds();
  } break;
  case inputs::ExtractTo: {
    if (inputs::outputFile.empty()) {
      std::cerr << "Must give out file to copy to\n";
      return 1;
    } else if (!inputs::overwrite && fs::exists(inputs::outputFile)) {
      std::cerr << "Can't overwrite file without force \n";
      return 1;
    } else if (inputs::inputFile == inputs::outputFile) {
      std::cerr << "Can't copy to source file\n";
      return 1;
    }
    std::ofstream file;
    file.open(inputs::outputFile.string(), std::ios::out);
    if (file.bad()) {
      std::cerr << "Failed to open file '" << inputs::outputFile << "'\n";
      return 2;
    }

    obj.write(file);
  } break;
  case inputs::ChangeRPaths: {
    if (inputs::oldPath.empty() || inputs::newPath.empty()) {
      std::cerr << "Must give both path to look for and to change to.\n";
      return 2;
    }
    if (obj.changeRPath(Path(inputs::oldPath), Path(inputs::newPath)))
      return writeToFile(obj);
    std::cerr << "Failed to change path, does it exist in this binary?\n";
    return 2;
  } break;
  case inputs::ChangeDylibPaths: {
    if (inputs::oldPath.empty() || inputs::newPath.empty()) {
      std::cerr << "Must give both path to look for and to change to.\n";
      return 2;
    }
    if (obj.changeDylibPaths(Path(inputs::oldPath), Path(inputs::newPath)))
      return writeToFile(obj);

    std::cerr << "Failed to change path, does it exist in this binary?\n";
    return 2;
  } break;
  case inputs::TargetInfo: {
    MachO::introspect_object insp(&obj);
    std::cout << insp.targetInfo();
  } break;
  default:
    std::cerr << "**Unknown action \n";
    showHelp();
  }

  return 0;
}

int main(int argc, const char *argv[])
{
  args.parse(argc, argv);

  if (inputs::inputFile.empty())
    showHelp();

  if (!fs::exists(inputs::inputFile)) {
    std::cerr << "File '" << inputs::inputFile << "' not found.\n";
    return 2;
  }

  if (inputs::inputFile == inputs::outputFile &&
      !inputs::overwriteInputFile)
  {
    std::cerr << "Not allowed to overwrite source file,"
              << "Try again with --force-overwrite-src";
    return 1;
  }

  if (!inputs::outputFile.empty() &&
      fs::exists(inputs::outputFile) &&
      !inputs::overwrite)
  {
    std::cerr << "Not allowed to overwrite existing file,"
              << " try again with --force-overwrite\n";
    return 1;
  }

  std::ifstream file;
  file.open(inputs::inputFile.string(), std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open input file."
              << "" << strerror(errno) << "\n";
    return 1;
  }

  MachO::mach_fat_object fat{file};
  if (!fat.failure()) {
    std::string keyArch = inputs::arch;

    std::transform(keyArch.begin(), keyArch.end(),
      keyArch.begin(), ::toupper);

    if (!inputs::arch.empty()) {
      auto arches = fat.architectures();
      auto found = std::find_if(arches.begin(), arches.end(),
        [&](const auto& arch) {
          std::string name = MachO::CpuTypeStr(arch.cputype());
          return name.size() && name.substr(3) == keyArch;
        }
      );

      if (found != arches.end())
        return runAction(fat.objects()
            .at(found - arches.begin()), inputs::action);

      std::cerr << "Architecture: " << inputs::arch << " not found\n";
      exit(2);
    }

    return runAction(fat.objects().at(0), inputs::action);
  }

  if (inputs::arch.size()) {
    std::cerr << "Architecture --arch=... not valid on a non fat binary\n";
    return 1;
  }

  file.clear();
  file.seekg(file.beg);
  MachO::mach_object obj{file};
  runAction(obj, inputs::action);
}