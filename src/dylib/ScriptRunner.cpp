/*
The MIT License (MIT)

Copyright (c) 2023 Fredrik Johansson

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

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include "ScriptRunner.h"
#include "Utils.h"
#include "ScriptRunner.h"
#include "Settings.h"

void runPythonScripts_afterHook() {
    auto& scripts = Settings::appBundleScripts();
    if (scripts.empty()) return;

    std::filesystem::path jsonFile =
        Settings::appBundlePath() / "jsonFile.json";

    std::cout << "* Running App bundle scripts on "
              << Settings::appBundlePath() << std::endl;

    // create a jsonSettings file for this project
    auto root = std::make_unique<json::Object>();
    root->set("settings", Settings::toJson());
    // save settings as json
    std::ofstream file; file.open(jsonFile, std::ios::out);
    file << root->serialize(2).str() << std::endl;
    file.close();


    // run all python scripts i bundleScripts path
    for (auto script : scripts) {
        std::cout<<"* Running script " << script << std::endl;
        auto pythonPath = std::string("PYTHONPATH=")
                            + Settings::scriptDir().string();
        auto cmd = pythonPath + " python " + script.string() + " " +
                    jsonFile.string();
        auto scriptOutput = system_get_output(cmd);
        if (!scriptOutput.empty()) {
            std::cout << "from " << script.filename() << ":\n"
                      << system_get_output(cmd) << std::endl;
        }
    }

    // remove jsonFile
    //if (std::filesystem::exists(jsonFile))
    //    std::filesystem::remove(jsonFile);

    std::cout << "* Done running all appBundle scripts" << std::endl;
}