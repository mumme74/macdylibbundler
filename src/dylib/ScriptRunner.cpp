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
#include <functional>

#include "ScriptRunner.h"
#include "Utils.h"
#include "ScriptRunner.h"
#include "Settings.h"
#include "asyncfifo.hpp"

using namespace afifo;

std::string handleSubProcessReq(const std::string& request);

class Msg {
  Bytes _bytes;
  std::size_t _size;
public:
  Msg(const Bytes& bytes):
    _bytes{bytes}, _size{0}
  {
    // big endian read header
    for (size_t i = 0; i < 4; ++i)
      _size |= bytes[i] << (8* (3-i));
  }

  Msg(const std::string& str):
    _bytes{}, _size{str.size() + 4}
  {
      std::cout << "sending " << str << '\n';
    _bytes.reserve(str.size()+5);
    // store size header big endian
    for (size_t i = 0; i < 4; i++)
      _bytes.emplace_back(_size & (0xff << (8*(3-i))));

    for (const auto& c : str)
      _bytes.emplace_back(c);
  }

  std::string str() const
  {
    std::string str;
    str.reserve(_size);
    for (const auto& b : _bytes)
      str += b;
    return str;
  }

  const Bytes& bytes() const
  {
    return _bytes;
  }

  bool isComplete() const
  {
    return _bytes.size() == _size +4;
  }

  void putBytes(const Bytes& bytes)
  {
    for (const auto& b : bytes)
      _bytes.emplace_back(b);
  }
};

class Script : public ScriptIO {
    std::unique_ptr<Msg> _rcvMsg;
public:
  Script(
    const std::string& scriptPath,
    const std::vector<std::string> args,
    const char* toFifo = "./.to.fifo",
    const char* fromFifo = "./.from.fifo"
  ):
    ScriptIO{scriptPath, args, toFifo, fromFifo},
    _rcvMsg{}
  {}

  void fromScript(const Bytes& bytes, Readable* file)
  {
    (void)file;
    std::cout << "got " << bytesToString(bytes)  << " from script\n";
    if (!_rcvMsg || _rcvMsg->isComplete())
      _rcvMsg.reset(new Msg(bytes));

    if (!_rcvMsg->isComplete())
      return;

    const auto str = _rcvMsg->str();
    auto resp = handleSubProcessReq(str);
    toScriptFifo().write(Msg(resp).bytes());
  }
};


void runPythonScripts_afterHook() {
    auto& scripts = Settings::appBundleScripts();
    if (scripts.empty()) return;

    std::cout << "* Running App bundle scripts on "
              << Settings::appBundlePath() << std::endl;

    auto root = std::make_unique<json::Object>();
    root->set("settings", Settings::toJson());

    namespace fs = std::filesystem;

    // run all executable python scripts i bundleScripts path
    for (auto script : scripts) {
        std::cout<<"* Running script " << script << std::endl;
        std::vector<std::string> args {
            fs::absolute(fs::path("to.fifo")),
            fs::absolute(fs::path("from.fifo"))
        };
        try {
            Script sc{script, args, args[0].c_str(), args[1].c_str()};
            std::cout << "ScriptIO created\n"; std::cout.flush();
            sc.run(10000);
        } catch (const std::runtime_error& e) {
            std::cerr << "Failed to run " << script << " "
                      << e.what() << "\n";
        }
    }

    std::cout << "* Done running all appBundle scripts" << std::endl;
}

// -------------------------------------------------------------------


struct ProtocolItem {
    const char *name, *description;
    typedef std::function<void(const char*, json::Object*)> cbType;
    cbType cb;
    ProtocolItem(const char* name, const char* description, cbType cb):
        name{name}, description{description}, cb{cb}
    {}
};

const int indent = 2;

json::VluType listProtocol();

const ProtocolItem protocol[] {
    {
        "get_protocol",
        "Gets info on all protocol commands a script can use",
        [](const char *cmd, json::Object* obj){
            obj->set(cmd, listProtocol());
        }
    },
    {
        "all_settings",
        "Gets all settings in a complete bundle",
        [](const char* cmd, json::Object* obj){ (void)cmd;
            obj->set(cmd, Settings::toJson());
        }
    },
    {
        "appBundlePath",
        "Get the path to the app bundle.",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::appBundlePath().string();
            obj->set(cmd, std::make_unique<json::String>(setting));
        }
    },
    {
        "frameworkDir",
        "Get path to framework dir inside app bundle",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::frameworkDir();
            obj->set(cmd, std::make_unique<json::String>(setting));
        }
    },
    {
        "destFolder",
        "Get path to the destination folder",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::destFolder();
            obj->set(cmd, std::make_unique<json::String>(setting));
        }
    },
    {
        "canOverwriteDir",
        "Is true if we are allowed to overwrite dir",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::canOverwriteDir();
            obj->set(cmd, std::make_unique<json::Bool>(setting));
        }
    },
    {
        "canOverwriteFiles",
        "Is true if we are allowed to overwrite files",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::canOverwriteFiles();
            obj->set(cmd, std::make_unique<json::Bool>(setting));
        }
    },
    {
        "canCodeSign",
        "Is true if we can codeSign the bundle",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::canCodesign();
            obj->set(cmd, std::make_unique<json::Bool>(setting));
        }
    },
    {
        "prefixTools",
        "Prefix tools ie. libtool example: if set to aarch-macho- "
        "becomes aarch-macho-libtool",
        [](const char* cmd, json::Object* obj){
            auto setting = Settings::prefixTools();
            obj->set(cmd, std::make_unique<json::String>(setting));
        }
    },
};

json::VluType listProtocol() {
    auto obj = std::make_unique<json::Object>();
    for (std::size_t i; i < sizeof(protocol)/sizeof(protocol[0]); ++i) {
        const auto& prot = protocol[i];
        obj->set(prot.name, std::make_unique<json::String>(prot.description));
    }
    return obj;
}

json::VluType handleCmd(const json::Array* params, json::Object* retObj)
{
    // cmd is always the first item in array
    auto cmd = params->at(0);
    if (!cmd->isString()) {
        std::stringstream ss;
        ss  << "Expected a string cmd from script, "
            << "got a " << cmd->typeName()
            << " with value: " << cmd->toString() << '\n';
        throw ss.str();
    }

    auto cmdStr = cmd->toString();
    for (std::size_t i = 0; i < sizeof(protocol)/sizeof(protocol[0]); ++i) {
        const auto& prot = protocol[i];
        if (prot.name == cmdStr)
            prot.cb(cmdStr.c_str(), retObj);
    }

    throw std::string("Command: ") + cmdStr + " not valid";
}

json::VluType handleJsonReq(json::VluType jsn)
{
    auto retObj = std::make_unique<json::Object>();

    if (jsn->isArray()) {
        auto arr = jsn->asArray();
        while (arr->length()) {
            auto elem = arr->pop();
            if (elem->isString()) {
                auto params = std::make_unique<json::Array>();
                params->push(std::move(elem));
                handleCmd(params.get(), retObj.get());
            } else if (elem->isArray()) {
                handleCmd(elem->asArray(), retObj.get());
            } else {
                std::stringstream ss;
                ss  << "Unhandled json request type from script, "
                    << "Expected a string or array as request command, "
                    << "got a " << elem->typeName() << '\n';
                throw ss.str();
            }
        }
    } else {
        std::stringstream ss;
        ss << "Mismatched json type in script request "
           << "Must be a json array at root of request\n";
        throw ss.str();
    }

    return retObj;
}

std::string handleSubProcessReq(const std::string& request)
{
    try {
        auto jsn = json::parse(request);
        if (!jsn || jsn->isUndefined())
            throw "Not a json Request";
        return handleJsonReq(std::move(jsn))->serialize().str();
    } catch (std::string& e) {
        std::cerr << e;
        auto res = std::make_unique<json::Object>();
        res->set("error", std::make_unique<json::String>(e));
        return res->serialize().str();
    }
}