#include "Utils.h"
#include "Hooks.h"
#include <unordered_dense/include/ankerl/unordered_dense.h>

namespace Hooks {
    namespace {
        struct CustomTokenDetails {
            CustomTokenDetails() { memset(this, NULL, sizeof(*this)); }
            CustomTokenDetails(FrameScript::TokenGuidGetter* getGuid, FrameScript::TokenIdGetter* getId)
                : hasN(false), getGuid(getGuid), getId(getId)
            {
            }
            CustomTokenDetails(FrameScript::TokenNGuidGetter* getGuid, FrameScript::TokenIdNGetter* getId)
                : hasN(true), getGuidN(getGuid), getIdN(getId)
            {
            }

            bool hasN;
            union {
                FrameScript::TokenGuidGetter* getGuid;
                FrameScript::TokenNGuidGetter* getGuidN;
            };
            union {
                FrameScript::TokenIdGetter* getId;
                FrameScript::TokenIdNGetter* getIdN;
            };
        };
        ankerl::unordered_dense::map<std::string, CustomTokenDetails> s_customTokens;

        struct CVarArgs {
            CVar** dst;
            const char* name;
            const char* desc;
            CVar::CVarFlags flags;
            const char* initialValue;
            CVar::Handler_t func;
            std::function<void(CVar*)> initCallback;
        };
        std::vector<CVarArgs> s_customCVars;
        std::vector<const char*> s_customEvents;
        std::vector<Lua::lua_CFunction> s_customLuaLibs;

        std::vector<FunctionCallback_t> s_customOnUpdate;
        std::vector<FunctionCallback_t> s_customOnEnter;
        std::vector<FunctionCallback_t> s_customOnLeave;
        std::vector<FunctionCallback_t> s_glueXmlCharEnum;
        std::vector<FunctionCallback_t> s_glueXmlPostLoad;

        constexpr uintptr_t GetGuidByKeyword_jmpcustom = 0x0060AD57;
        constexpr uintptr_t GetGuidByKeyword_jmporig = 0x0060AD44;

        void FrameScript_FillEventsHk(const char** list, size_t count) {
            std::vector<const char*> events;
            events.reserve(count + s_customEvents.size());
            events.insert(events.end(), list, list + count);
            events.insert(events.end(), s_customEvents.begin(), s_customEvents.end());
            FrameScript::FillEventsFn(events.data(), events.size());
        }

        void __cdecl CVar__InitializeHk() {
            CVar::InitializeFn();
            for (const auto& [dst, name, desc, flags, initialValue, func, cb] : s_customCVars) {
                CVar* cvar = CVar::Register(name, desc, flags, initialValue, func, 0, 0, 0, 0);
                if (dst) *dst = cvar;
                if (cb && cvar) cb(cvar);
            }
        }

        void Lua_OpenFrameXMlApi_bulk() {
            if (lua_State* L = Lua::GetLuaState()) {
                for (auto& func : s_customLuaLibs) func(L);
            }
        }

        void __declspec(naked) Lua_OpenFrameXMLApiHk() {
            __asm {
                pushad;
                pushfd;
                call Lua_OpenFrameXMlApi_bulk;
                popfd;
                popad;
                ret;
            }
        }

        bool __cdecl GetGuidByKeyword_bulk(const char** stackStr, guid_t* guid) {
            for (auto& [token, conv] : s_customTokens) {
                if (strncmp(*stackStr, token.data(), token.size()) == 0) {
                    *stackStr += token.size();
                    if (conv.hasN) {
                        int n = gc_atoi(stackStr);
                        *guid = n > 0 ? conv.getGuidN(n - 1) : 0;
                    }
                    else {
                        *guid = conv.getGuid();
                    }
                    return true;
                }
            }
            return false;
        }

        void __declspec(naked) GetGuidByKeywordHk() {
            __asm {
                pushad;
                pushfd;
                push [ebp + 0xC];
                lea eax, [ebp + 0x8];
                push eax;
                call GetGuidByKeyword_bulk;
                add esp, 8;
                test al, al;
                jnz skip;
                popfd;
                popad;
                push GetGuidByKeyword_jmporig;
                ret;
            skip:
                popfd;
                popad;
                push GetGuidByKeyword_jmpcustom;
                ret;
            }
        }

        char** GetKeywordsByGuidHk(guid_t* guid, size_t* size) {
            char** buf = CGGameUI::GetKeywordsByGuidFn(guid, size);
            if (!buf) return buf;
            for (auto& [token, conv] : s_customTokens) {
                if (*size >= 8) break;
                if (conv.hasN) {
                    int id = conv.getIdN(*guid);
                    if (id >= 0) snprintf(buf[(*size)++], 32, "%s%d", token.c_str(), id + 1);
                }
                else {
                    if (conv.getId(*guid)) snprintf(buf[(*size)++], 32, "%s", token.c_str());
                }
            }
            return buf;
        }

        int FrameScript_FireOnUpdateHk(int a1, int a2, int a3, int a4) {
            for (auto& func : s_customOnUpdate) func();
            return FrameScript::FireOnUpdateFn(a1, a2, a3, a4);
        }

        void __fastcall OnEnterWorld() {
            for (auto& func : s_customOnEnter) func();
            return CGGameUI::EnterWorldFn();
        }

        void __fastcall OnLeaveWorld() {
            for (auto& func : s_customOnLeave) func();
            return CGGameUI::LeaveWorldFn();
        }

        void LoadGlueXML_bulk() { for (auto& func : s_glueXmlPostLoad) func(); }

        void __declspec(naked) LoadGlueXMLHk() {
            __asm {
                pop ebx;
                mov esp, ebp;
                pop ebp;

                pushad;
                pushfd;
                call LoadGlueXML_bulk;
                popfd;
                popad;
                ret;
            }
        }

        void LoadCharacters_bulk() { for (auto& func : s_glueXmlCharEnum) func(); }

        void __declspec(naked) LoadCharactersHk() {
            __asm {
                add esp, 8;
                pop esi;

                pushad;
                pushfd;
                call LoadCharacters_bulk;
                popfd;
                popad;

                ret;
            }
        }
    }

    void FrameXML::registerEvent(const char* str) { s_customEvents.push_back(str); }
    void FrameXML::registerCVar(CVar** dst, const char* str, const char* desc, const char* initialValue,
            CVar::Handler_t func, CVar::CVarFlags flags, const std::function<void(CVar*)>& initCallback) {
        s_customCVars.push_back({
           .dst = dst,
           .name = str,
           .desc = desc,
           .flags = flags,
           .initialValue = initialValue,
           .func = func,
           .initCallback = initCallback
            });
    }
    void FrameXML::registerLuaLib(const Lua::lua_CFunction& func) { s_customLuaLibs.push_back(func); }

    void FrameScript::registerToken(const char* token, TokenGuidGetter* getGuid, TokenIdGetter* getId) { s_customTokens[token] = { getGuid, getId }; }
    void FrameScript::registerToken(const char* token, TokenNGuidGetter* getGuid, TokenIdNGetter* getId) { s_customTokens[token] = { getGuid, getId }; }

    void FrameScript::registerOnUpdate(const FunctionCallback_t& func) { s_customOnUpdate.push_back(func); }
    void FrameScript::registerOnEnter(const FunctionCallback_t& func) { s_customOnEnter.push_back(func); }
    void FrameScript::registerOnLeave(const FunctionCallback_t& func) { s_customOnLeave.push_back(func); }

    void GlueXML::registerPostLoad(const FunctionCallback_t& func) { s_glueXmlPostLoad.push_back(func); }
    void GlueXML::registerCharEnum(const FunctionCallback_t& func) { s_glueXmlCharEnum.push_back(func); }
}

void Hooks::initialize() {
    Detour(&CVar::InitializeFn, CVar__InitializeHk);
    Detour(&FrameScript::FireOnUpdateFn, FrameScript_FireOnUpdateHk);
    Detour(&FrameScript::FillEventsFn, FrameScript_FillEventsHk);
    Detour(&CGGameUI::EnterWorldFn, OnEnterWorld);
    Detour(&CGGameUI::LeaveWorldFn, OnLeaveWorld);
    Detour(&CGGameUI::GetGuidByKeywordFn, GetGuidByKeywordHk);
    Detour(&CGGameUI::GetKeywordsByGuidFn, GetKeywordsByGuidHk);
    Detour(&CGlueMgr::LoadGlueXMLFn, LoadGlueXMLHk);
    Detour(&CGlueMgr::LoadCharactersFn, LoadCharactersHk);
    Detour(&Lua::OpenFrameXMLApiFn, Lua_OpenFrameXMLApiHk);
}