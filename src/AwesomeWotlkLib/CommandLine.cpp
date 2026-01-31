#include "CommandLine.h"
#include "Hooks.h"
#include "GameClient.h"
#include "Utils.h"
#include <shellapi.h>

static std::vector<std::string> s_commandLine;


static const char* getParam(const char* item)
{
    for (uint32_t i = 1; (i + 1) < s_commandLine.size(); i++) {
        const char* key = s_commandLine[i].c_str();
        if (char c = *(key++); c == '-' || c == '/') {
            if (key[0] == '-') ++key;
            if (strcmp(key, item) == 0)
                return s_commandLine[i + 1].c_str();
        }
    }
    return NULL;
}

static void gluexml_charenum()
{
    static bool s_once = false;
    if (s_once) return;
    s_once = true;

    if (const char* character = getParam("character")) {
        LoginUI::CharVector* chars = LoginUI::GetChars();
        for (int i = 0; i < chars->size; i++) {
            if (strcmp(chars->buf[i].data.name, character) == 0) {
                LoginUI::EnterWorld(i);
                break;
            }
        }
    }
}

static void gluexml_postload()
{
    static bool s_once = false;
    if (s_once) return;
    s_once = true;

    const char* realmList = getParam("realmlist");
    if (Console::CVar* cvar = Console::FindCVar("realmList"); cvar && realmList)
        Console::SetCVarValue(cvar, realmList, 1, 0, 0, 1);

    const char* realmname = getParam("realmname");
    if (Console::CVar* cvar = Console::FindCVar("realmName"))
        Console::SetCVarValue(cvar, realmname, 1, 0, 0, 1);

    const char* login = getParam("login");
    const char* password = getParam("password");
    if (login && password) NetClient::Login(login, password);
}

void CommandLine::initialize()
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 0; i < argc; i++)
        s_commandLine.emplace_back(u16tou8(argv[i]));

    Hooks::GlueXML::registerCharEnum(gluexml_charenum);
    Hooks::GlueXML::registerPostLoad(gluexml_postload);
}