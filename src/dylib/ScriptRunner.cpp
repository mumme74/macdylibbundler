/*
The MIT License (MIT)

Copyright (c) 2024 Fredrik Johansson

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
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cassert>
#include <cstring>

#include "ScriptRunner.h"
#include "Utils.h"
#include "ScriptRunner.h"
#include "Settings.h"

std::string handleSubProcessReq(const std::string& request);
json::VluType handleJsonReq(json::VluType jsn);

typedef union _bigendian  {
private:
    static constexpr uint32_t uint32_ = 0x01020304;
    static constexpr uint8_t magic_ = (const uint8_t&)uint32_;
    static constexpr bool little = magic_ == 0x04;
    static constexpr bool big = magic_ == 0x01;
    static_assert(little || big, "Cannot determine endianness!");
    static void conv(uint8_t* uTo, uint8_t* uFrom, size_t sz) {
        for (size_t i = 0, j = sz-1; i < sz; ++i, --j)
            uTo[i] = uFrom[j];
    }
public:
    static constexpr bool middle = magic_ == 0x02;
    _bigendian(uint16_t vlu) {
        if constexpr(little) {
            uint8_t *a = (uint8_t*)(&vlu);
            _bigendian::conv(u16arr, a, 2);
        } else {
            auto u = (uint16_t*)u16arr;
            u[0] = vlu;
        }
    }
    _bigendian(uint32_t vlu) {
        if constexpr(little) {
            uint8_t *a = (uint8_t*)(&vlu);
            _bigendian::conv(u32arr, a, 4);
        } else{
            auto u = (uint32_t*)u32arr;
            u[0] = vlu;
        }
    }
    _bigendian(uint64_t vlu) {
        if constexpr(little) {
            uint8_t *a = (uint8_t*)(&vlu);
            _bigendian::conv(u64arr, a, 8);
        } else{
            auto u = (uint64_t*)u64arr;
            u[0] = vlu;
        }
    }

    uint8_t u16arr[2];
    uint16_t u16native() {
        if constexpr(little) {
            uint16_t vlu;
            uint8_t *a = (uint8_t*)(&vlu);
            _bigendian::conv(a, u16arr, 2);
            return vlu;
        } else {
            return (uint16_t)u16arr[0];
        }
    };
    uint8_t u32arr[4];
    uint32_t u32native() {
        if constexpr(little) {
            uint32_t vlu;
            uint8_t *a = (uint8_t*)(&vlu);
            _bigendian::conv(a, u32arr, 4);
            return vlu;
        } else {
            return (uint32_t)u32arr[0];
        }
    };
    uint8_t u64arr[8];
    uint64_t u64native() {
        if constexpr(little) {
            uint64_t vlu;
            uint8_t *a = (uint8_t*)(&vlu);
            _bigendian::conv(a, u64arr, 8);
            return vlu;
        } else {
            return (uint64_t)u64arr[0];
        }
    };

} bigendian_t;

class File{
public:
    File(int fd, const char* mode = "r") :
        m_file{fdopen(fd, mode)}
    {}
    ~File() {
        if (m_file)
            close(fileno(m_file));
    }
    constexpr FILE* &file() { return m_file; }
private:
    FILE* m_file;
};

class Pipes {
public:
    Pipes():
        fds{-1},
        bakStdOut{dup(STDOUT_FILENO)},
        stdout{fdopen(bakStdOut, "w")}
    {}
    ~Pipes() {
        closeAll();
    }
    void closeAll() {
        for (const auto& fd : fds) {
            if (fd > -1) ::close(fd);
        }
        fclose(this->stdout);
    }
    bool create() {
        for (std::size_t i = 0; i < fds.size(); i += 2) {
            if (pipe(&fds[i]) == -1) {
                std::cerr << "Failed to create pipes to process\n";
                return false;
            }
        }
        return true;
    }
    constexpr int &scriptIn() { return fds[2]; }
    constexpr int &scriptOut() { return fds[1]; }
    constexpr int &parentIn() { return fds[0]; }
    constexpr int &parentOut() { return fds[3]; }

    void closeFds(std::array<int, 2> closeFds) {
        for(const auto& fd : closeFds) {
           close(fd);
        }
    }
    void asScript() {
        closeFds(std::array<int, 2>{parentIn(), parentOut()});
    }
    void asParent() {
        closeFds(std::array<int, 2>{scriptIn(), scriptOut()});
    }
    std::array<int, 4> fds;
    int bakStdOut;
    FILE* stdout;
};

bool scriptLogic(
    Pipes& pipes, std::string_view script,
    char* const* argv
) {
    // reroute stdin, stdout
    auto makeDup = [&](int fd, int fd2) {
        if (dup2(fd, fd2) < 0) {
            fprintf(pipes.stdout, "Failed to duplicate fd: %d\n", fd2);
            exit(1);
        }
    };
    makeDup(pipes.scriptIn(), STDIN_FILENO);
    makeDup(pipes.scriptOut(), STDOUT_FILENO);

    fprintf(pipes.stdout, "Before execve\n");
    int res = execvp(script.data(), argv);
    // If we get here command failed.
    // Should be replaced by script process.
    fprintf(pipes.stdout,
            "Failed to run %s error: %s with error code: %d\n",
            script.data(), strerror(errno), res);
    exit(1);
}

bool parentRead(FILE *in, std::string& buf) {
    bigendian_t sz{0u};
    if (fread(sz.u32arr, 1, 4, in) != 4) {
        std::cerr << "Failed to read size from script\n";
        return false;
    }
    auto siz = sz.u32native();
    auto b = std::make_unique<char[]>(siz+1);
    if (fread(b.get(), 1, siz, in) != siz) {
        std::cerr << "Failed to read " << siz << " bytes\n";
        return false;
    }
    buf = b.get();
    return true;
}

bool parentWrite(FILE *out, std::string_view str) {
    std::cout << "Sending: " << str << "\n\n";

    assert(str.size() < UINT32_MAX && "To big string send to script");
    bigendian_t sz{static_cast<uint32_t>(str.size())};

    if ((fwrite(&sz.u32arr, 4, 1, out) != 4) ||
        (fwrite(str.data(), sizeof(char),
                str.size(), out) != str.size()) ||
        (fputc('\n', out) == -1)
    ) {
        std::cerr << "Failed to write n:"<<str.size()
                    << " to script\n" << str << "\n";
        return false;
    }
    fflush(out);
    return true;
}

bool parentValueResponse(FILE *out, json::VluType res) {
    for (const auto& vlu : res->asObject()->values()) {
        if (!parentWrite(out, vlu->serialize(2).str()))
            return false;
    }
    return true;
}

bool parentLoop(Pipes& pipes) {
    File in{pipes.parentIn(), "r"},
         out{pipes.parentOut(), "w"};

    for (std::string input; parentRead(in.file(), input); input.clear()) {
        if (!input.size()) continue;
        json::VluType jsn;
        bool resInJson = false;
        if (input[0] == '{' || input[0] == '[') {
            // support both json and normal commands
            try {
                jsn = json::parse(input);
                resInJson = true;
            } catch (std::string e) {
                std::cerr << e << "\n";
                return false;
            } catch (...) {}
        }
        try {
            if (!jsn)
                jsn = json::parse(std::string("[\"") + input + "\"]");

            auto res = handleJsonReq(std::move(jsn));
            if (resInJson) {
                auto str = res->serialize().str();
                if (!parentWrite(out.file(), str))
                    return false;
            }

            if (!parentValueResponse(out.file(), std::move(res)))
                return false;
        } catch (std::string e) {
            std::cerr << e << "\n";
            return false;
        }
    }
    return true;
}

bool parentLogic(Pipes& pipes, int pidScript, int pidTimeout) {

    bool res = parentLoop(pipes);

    kill(pidTimeout, SIGKILL);
    kill(pidScript, SIGKILL);
    waitpid(pidTimeout, nullptr, 0);
    int wstatus;
    waitpid(pidScript, &wstatus, 0);
    if (WIFEXITED(wstatus)) {
        return WEXITSTATUS(wstatus) == 0;
    }
    return res;
}

bool runScript(
    std::string_view script
) {
    Pipes pipes;
    if (!pipes.create())
        return false;

    int pidScript = fork();
    if (pidScript < 0) {
        std::cerr << "Failed to fork to subprocess\n";
        return false;
    } else if (pidScript == 0) {
        // this is the child becomes script
        pipes.asScript();
        auto scriptBuf = std::make_unique<char[]>(script.size()+1);
        strncpy(scriptBuf.get(), script.data(), script.size());

        auto path = std::filesystem::absolute(
            Settings::appBundlePath()).string();
        auto pathBuf = std::make_unique<char[]>(path.size()+1);
        strncpy(pathBuf.get(), path.data(), path.size());

        char* const argv[3] = {
            scriptBuf.get(),
            pathBuf.get(),
            nullptr
        };
        return scriptLogic(pipes, script, argv);
    }

    // we are the parent, now set up timeout
    int pidTimeout = fork();
    if (pidTimeout == -1) {
        std::cerr << "Failed to fork timeout process\n";
        return false;
    } else if (pidTimeout == 0) {
        // timeout child process
        pipes.closeAll();
        sleep(20); // 20 sec timeout
        std::cerr << "* Script timeout, killed script: "
                  << script << "\n";
        kill(pidScript, SIGKILL);
        exit(0);
    }

    pipes.asParent();
    return parentLogic(pipes, pidScript, pidTimeout);
}


void runPythonScripts_afterHook() {
    auto& scripts = Settings::appBundleScripts();
    if (scripts.empty()) return;

    std::cout << "\n* Running App bundle scripts on "
              << Settings::appBundlePath() << std::endl;

    auto root = std::make_unique<json::Object>();
    root->set("settings", Settings::toJson());

    namespace fs = std::filesystem;

    // run all executable python scripts i bundleScripts path
    for (auto script : scripts) {
        std::cout<<"* Running script " << script << std::endl;
        if (runScript(script.c_str())) {
            std::cout << "Finished script " << script << "\n";
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
    for (std::size_t i = 0; i < sizeof(protocol)/sizeof(protocol[0]); ++i) {
        const auto& prot = protocol[i];
        obj->set(prot.name, std::make_unique<json::String>(prot.description));
    }
    return obj;
}

void handleCmd(const json::Array* params, json::Object* retObj)
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
        if (prot.name == cmdStr) {
            prot.cb(cmdStr.c_str(), retObj);
            return;
        }
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