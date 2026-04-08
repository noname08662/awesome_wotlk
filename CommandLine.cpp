#include "CommandLine.h"
#include "Hooks.h"
#include "GameClient.h"
#include "Utils.h"
#include <shellapi.h>
#include <vector>
#include <string>

namespace {
    std::vector<std::string> s_commandLine;

    const char* getParam(const char* item) {
        if (!item || s_commandLine.empty()) return nullptr;
        for (size_t i = 1; (i + 1) < s_commandLine.size(); ++i) {
            const char* key = s_commandLine[i].c_str();
            if (char c = key[0]; c == '-' || c == '/') {
                key++;
                if (key[0] == '-') key++;
                if (std::strcmp(key, item) == 0) return s_commandLine[i + 1].c_str();
            }
        }
        return nullptr;
    }

    auto setCVarFromParam = [](const char* paramName, const char* cvarName) {
        if (const char* val = getParam(paramName)) {
            if (auto* cvar = CVar::Find(cvarName)) {
                cvar->SetValue(val, 1, 0, 0, 1);
            }
        }
        };

    void gluexml_charenum() {
        static bool s_once = false;
        if (s_once) return;

        if (const char* character = getParam("character")) {
            LoginUI::CharVector* chars = LoginUI::GetChars();
            if (chars && chars->m_buf) {
                for (int i = 0; i < chars->m_size; i++) {
                    if (std::strcmp(chars->m_buf[i].m_data.m_name, character) == 0) {
                        s_once = true;
                        LoginUI::EnterWorld(i);
                        break;
                    }
                }
            }
        }
    }

    void gluexml_postload() {
        static bool s_once = false;
        if (s_once) return;
        s_once = true;

        setCVarFromParam("realmlist", "realmList");
        setCVarFromParam("realmname", "realmName");

        const char* user = getParam("login");
        const char* pass = getParam("password");
        if (user && pass) NetClient::Login(user, pass);
    }
}

void CommandLine::initialize() {
    int argc = 0;
    if (wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc)) {
        s_commandLine.reserve(static_cast<size_t>(argc));
        for (int i = 0; i < argc; i++) {
            s_commandLine.emplace_back(u16tou8(argv[i]));
        }
        LocalFree(reinterpret_cast<HLOCAL>(argv));
    }
    Hooks::GlueXML::registerCharEnum(gluexml_charenum);
    Hooks::GlueXML::registerPostLoad(gluexml_postload);
}