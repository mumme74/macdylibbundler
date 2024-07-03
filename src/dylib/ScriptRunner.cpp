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
#include "DylibBundler.h"

std::string handleSubProcessReq(const std::string& request);
Json::VluType handleJsonReq(Json::VluType jsn);
void handleCmd(std::string_view cmd, Json::VluBase* params, Json::Object* retObj);


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

    //fprintf(pipes.stdout, "Before execvpe\n");
    int res = execvpe(script.data(), argv, environ);
    // If we get here command failed.
    // Should be replaced by script process.
    fprintf(pipes.stdout,
            "Failed to run %s error: %s with error code: %d\n",
            script.data(), strerror(errno), res);
    exit(1);
}

bool parentRead(FILE *in, std::string& buf) {
    bigendian_t sz{0u};
    size_t n = 0;
    if ((n = fread(sz.u32arr, 1, 4, in)) != 4) {
        if (n != 0)
            std::cerr << "Failed to read size from script\n";
        return false;
    }
    auto siz = sz.u32native();
    //std::cout << "Read " << siz << " bytes from script. << \n";
    auto b = std::make_unique<char[]>(siz+1);
    if ((n = fread(b.get(), 1, siz, in)) != siz) {
        if (n != 0)
            std::cerr << "Failed to read " << siz << " bytes\n";
        return false;
    }
    buf = b.get();
    return true;
}

bool parentWrite(FILE *out, std::string_view str) {
    //std::cout << "Sending: " << str << "\n\n";

    assert(str.size() < UINT32_MAX && "To big string send to script");
    bigendian_t sz{static_cast<uint32_t>(str.size())};

    if ((fwrite(&sz.u32arr, 1, 4, out) != 4) ||
        (fwrite(str.data(), sizeof(char),
                str.size(), out) != str.size())
    ) {
        std::cerr << "Failed to write n:"<<str.size()
                  << " to script\n" << str << "\n";
        return false;
    }
    fflush(out);
    return true;
}

bool parentValueResponse(FILE *out, Json::VluType res) {
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
        Json::VluType jsn;
        bool resInJson = false;
        if (input[0] == '{' || input[0] == '[') {
            // support both json and normal commands
            try {
                jsn = Json::parse(input);
                resInJson = true;
                //std::cout << jsn->serialize(2).str() << "\n";
            } catch (Json::Exception& e) {
                std::cerr << e.what() << "\n";
                return false;
            } catch (...) {}
        }
        try {
            if (!jsn)
                jsn = Json::parse(std::string("[\"") + input + "\"]");

            auto res = handleJsonReq(std::move(jsn));
            if (resInJson) {
                auto str = res->serialize().str();
                if (!parentWrite(out.file(), str))
                    return false;
            } else if (!parentValueResponse(out.file(), std::move(res)))
                return false;
        } catch (Json::Exception& e) {
            std::cerr << e.what() << "\n";
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

    auto root = std::make_unique<Json::Object>();
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
    typedef std::function<void(
        const char* cmd, Json::Object* retObj, Json::VluBase* args)
    > cbType;
    cbType cb;
    ProtocolItem(const char* name, const char* description, cbType cb):
        name{name}, description{description}, cb{cb}
    {}
};

const int indent = 2;

Json::VluType listProtocol();

const ProtocolItem protocol[] {
    {
        "get_protocol",
        "Gets info on all protocol commands a script can use",
        [](const char *cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            obj->set(cmd, listProtocol());
        }
    },
    {
        "all_settings",
        "Gets all settings in a complete bundle",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            obj->set(cmd, Settings::toJson());
        }
    },
    {
        "app_bundle_path",
        "Get the path to the app bundle.",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::appBundlePath().string();
            obj->set(cmd, std::make_unique<Json::String>(setting));
        }
    },
    {
        "framework_dir",
        "Get path to framework dir inside app bundle",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::frameworkDir();
            obj->set(cmd, std::make_unique<Json::String>(setting));
        }
    },
    {
        "dest_folder",
        "Get path to the destination folder",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::destFolder();
            obj->set(cmd, std::make_unique<Json::String>(setting));
        }
    },
    {
        "can_overwrite_dir",
        "Is true if we are allowed to overwrite dir",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::canOverwriteDir();
            obj->set(cmd, std::make_unique<Json::Bool>(setting));
        }
    },
    {
        "can_overwrite_files",
        "Is true if we are allowed to overwrite files",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::canOverwriteFiles();
            obj->set(cmd, std::make_unique<Json::Bool>(setting));
        }
    },
    {
        "can_code_sign",
        "Is true if we can codeSign the bundle",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::canCodesign();
            obj->set(cmd, std::make_unique<Json::Bool>(setting));
        }
    },
    {
        "prefix_tools",
        "Prefix tools ie. libtool example: if set to aarch-macho- "
        "becomes aarch-macho-libtool",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            auto setting = Settings::prefixTools();
            obj->set(cmd, std::make_unique<Json::String>(setting));
        }
    },
    {
        "dylib_info",
        "Get information of all dependencies collected",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            obj->set(cmd, DylibBundler::instance()->toJson());
        }
    },
    {
        "install_name_tool_cmd",
        "Get install_name_tool name, might be named differently",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            obj->set(cmd, std::make_unique<Json::String>(
                Settings::installNameToolCmd()));
        }
    },
    {
        "otool_cmd",
        "Get otool name, might be named differently",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            (void)args;
            obj->set(cmd, std::make_unique<Json::String>(
                Settings::otoolCmd()));
        }
    },
    {
        "add_search_paths",
        "Add search paths to parents process bore it tries to fixup_binaries.",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args) {
            if (!args->isArray()) {
                obj->set(cmd, Json::Bool(false));
                obj->set("error", Json::String("Expected an array"));
            } else {
                for (const auto& p : *args->asArray())
                    Settings::addSearchPath(p->asString()->vlu());
                obj->set(cmd, Json::Bool(true));
            }
        }
    },
    {
        "fixup_binaries",
        "Do things on these binary files after script has finished them.\n"
        "Such as scanning them for dependencies and running install_name_cmd on them.",
        [](const char* cmd, Json::Object* obj, Json::VluBase* args){
            auto dylib = DylibBundler::instance();
            auto res = dylib->fixPathsInBinAndCodesign(args->asArray());
            obj->set(cmd, std::move(res));
        }
    }
};

Json::VluType listProtocol() {
    auto obj = std::make_unique<Json::Object>();
    for (std::size_t i = 0; i < sizeof(protocol)/sizeof(protocol[0]); ++i) {
        const auto& prot = protocol[i];
        obj->set(prot.name, std::make_unique<Json::String>(prot.description));
    }
    return obj;
}

void handleCmd(std::string_view cmd, Json::VluBase* params, Json::Object* retObj)
{
    // cmd is always the first item in array
    if (!cmd.size())
        throw Json::Exception{"No command given"};

    for (std::size_t i = 0; i < sizeof(protocol)/sizeof(protocol[0]); ++i) {
        const auto& prot = protocol[i];
        if (prot.name == cmd) {
            prot.cb(cmd.data(), retObj, params);
            return;
        }
    }

    throw std::string("*Command: ") + cmd.data() + " not valid";
}

Json::VluType handleJsonReq(Json::VluType jsn)
{
    auto retObj = std::make_unique<Json::Object>();

    if (jsn->isArray()) {
        auto arr = jsn->asArray();
        for (const auto& elem : *arr) {
            if (elem->isString()) {
                handleCmd(elem->asString()->vlu(), nullptr, retObj.get());
            } else {
                std::stringstream ss;
                ss  << "Unhandled json request type from script, "
                    << "Expected a string or object as request command, "
                    << "got a " << elem->typeName() << '\n';
                throw ss.str();
            }
        }
    } else if (jsn->isObject()) {
        auto obj = jsn->asObject();
        for (const auto& pair : *obj) {
            Json::VluBase* params = pair.second.get();
            handleCmd(pair.first, params, retObj.get());
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
    auto sendError = [&](const char* what) -> std::string {
        auto res = std::make_unique<Json::Object>();
        res->set("error", std::make_unique<Json::String>(what));
        return res->serialize().str();
    };

    try {
        auto jsn = Json::parse(request);
        if (!jsn || jsn->isNull())
            throw "Not a json Request";
        return handleJsonReq(std::move(jsn))->serialize().str();
    } catch (Json::Exception& e) {
        std::cerr << e.what() << "\n";
        return sendError(e.what());
    } catch (std::string& e) {
        std::cerr << e << "\n";
        return sendError(e.c_str());
    }
}