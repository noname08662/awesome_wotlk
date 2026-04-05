#include "Utils.h"
#include "Camera.h"
#include "GameClient.h"
#include "Hooks.h"
#include <ranges>
#include <unordered_dense/include/ankerl/unordered_dense.h>

#undef min
#undef max

namespace {
    ankerl::unordered_dense::set<CM2Model*> g_models_current;
    ankerl::unordered_dense::map<CM2Model*, float> g_models_original_alphas;
    ankerl::unordered_dense::set<CM2Model*> g_models_being_faded;
    ankerl::unordered_dense::map<CM2Model*, uint32_t> g_models_grace_timers;

    float g_actual_distance = 1.0f;
    float g_indirectAlpha = 1.0f;

    CVar* s_cvar_cameraIndirectVisibility;
    CVar* s_cvar_cameraIndirectAlpha;
    CVar* s_cvar_cameraFov;
    CVar* s_cvar_showPlayer;

    char __cdecl CGWorldFrame_IntersectHk(C3Vector* playerPos, C3Vector* cameraPos, C3Vector* hitPoint, float* hitDistance, uint32_t hitFlags, uint32_t buffer) {
        void* buf = reinterpret_cast<void*>(buffer);
        std::memset(buf, 0, 2048);

        char result = CGWorldFrame::IntersectFn(playerPos, cameraPos, hitPoint, hitDistance, hitFlags + 1 + 0x8000000, reinterpret_cast<uintptr_t>(buf));
        if (result) {
            const uint32_t* bufData = static_cast<const uint32_t*>(buf);
            if ((bufData[0] == 1 || bufData[0] == 2) && bufData[1] > 0) {
                if (CM2Model* modelPtr = *reinterpret_cast<CM2Model**>(static_cast<uint8_t*>(buf) + 12 + 80)) {
                    g_models_current.insert(modelPtr);
                    g_models_being_faded.insert(modelPtr);

                    if (!g_models_original_alphas.contains(modelPtr)) {
                        g_models_original_alphas[modelPtr] = modelPtr->m_alpha;
                    }
                    modelPtr->m_alpha += (g_indirectAlpha - modelPtr->m_alpha) * 0.25f;

                    g_models_grace_timers[modelPtr] = 0;

                    if (g_actual_distance < 1.0f) {
                        *hitDistance = g_actual_distance;
                    }
                    else {
                        result = false;
                    }
                }
            }
            else {
                g_models_current.clear();
                g_actual_distance = *hitDistance;
            }
        }
        else if (g_models_being_faded.empty()) {
            return result;
        }
        else {
            g_models_current.clear();
            g_actual_distance = 1.0f;
        }

        uint32_t cleanup_timer = static_cast<uint32_t>(g_models_being_faded.size()) + 1;
        for (auto it = g_models_being_faded.begin(); it != g_models_being_faded.end();) {
            CM2Model* model = *it;
            if (g_models_grace_timers[model] > cleanup_timer) {
                float originalAlpha = g_models_original_alphas[model];
                model->m_alpha += (originalAlpha - model->m_alpha) * 0.25f;
                if (std::fabs(model->m_alpha - originalAlpha) < 0.01f) {
                    model->m_alpha = originalAlpha;
                    g_models_original_alphas.erase(model);
                    it = g_models_being_faded.erase(it);
                    continue;
                }
            }
            ++it;
        }
    	for (auto& timer : g_models_grace_timers | std::views::values) timer++;

        return result;
    }

    auto IntersectCallFn = reinterpret_cast<DummyCallback_t>(0x006060E6);
    constexpr uintptr_t IntersectCall_jmpbackaddr = 0x00606103;

    void __declspec(naked) IntersectCallHk() {
        __asm {
            sub esp, 2048
            lea eax, [esp]
            fld1
            push eax
            and ebx, ~1
            push ebx
            fstp dword ptr[ebp + 0x18]
            lea ecx, [ebp + 0x18]
            push ecx
            lea edx, [ebp - 0x48]
            push edx
            lea eax, [ebp - 0x18]
            push eax
            lea ecx, [ebp - 0x24]
            push ecx
            call CGWorldFrame_IntersectHk
            add esp, 2048
            jmp IntersectCall_jmpbackaddr
        }
    }

    bool __cdecl collisionFilter(CM2Model* model) {
        return !g_models_current.contains(model);
    }

    auto IterateCollisionListFn = reinterpret_cast<DummyCallback_t>(0x007A279D);
    constexpr uintptr_t IterateCollisionList_jmpback = 0x007A27A5;
    constexpr uintptr_t IterateCollisionList_skipaddr = 0x007A2943;

    __declspec(naked) void IterateCollisionListHk() {
        __asm {
            mov esi, [edx + 4]       // esi = object entry
            pushfd
            push eax
            push ecx
            push edx
            mov eax, [ebp + 0Ch]
            and eax, 0x8000000
            jz run_collision_processing
            mov eax, [esi + 0x34]    // eax = modelPtr
            test eax, eax
            jz skip_collision_processing
            push eax
            call collisionFilter
            add esp, 4
            test al, al
            je skip_collision_processing
            run_collision_processing :
            pop edx
                pop ecx
                pop eax
                popfd
                cmp byte ptr[esi + 25h], bl
                jmp IterateCollisionList_jmpback
                skip_collision_processing :
            pop edx
                pop ecx
                pop eax
                popfd
                jmp IterateCollisionList_skipaddr
        }
    }

    auto IterateWorldObjCollisionListFn = reinterpret_cast<DummyCallback_t>(0x007A2A1C);
    constexpr uintptr_t IterateWorldObjCollisionList_jmpback = 0x007A2A23;
    constexpr uintptr_t IterateWorldObjCollisionList_skipaddr = 0x007A2A8A;

    __declspec(naked) void IterateWorldObjCollisionListHk() {
        __asm {
            mov edx, [ebp + 0Ch]
            test edx, 0x8000000
            jz run_collision_processing
            push eax    // eax = modelPtr
            call collisionFilter
            add esp, 4
            test al, al
            je skip_collision_processing
            run_collision_processing :
            mov eax, [ebp - 68h]
                cmp dword ptr[eax + 2D8h], 0
                jmp IterateWorldObjCollisionList_jmpback
                skip_collision_processing :
            jmp IterateWorldObjCollisionList_skipaddr
        }
    }

    void __fastcall CSimpleCamera__constructorHk(CSimpleCamera* pthis, void* edx, float a2, float a3, float fov) {
        float f;
        s_cvar_cameraFov->Sync(s_cvar_cameraFov->m_str, &f, 90.0f, 150.0f, "%.2f");
        fov = static_cast<float>(M_PI / 200.0 * f);
        pthis->constructor(a2, a3, fov);
    }

    int CVarHandler_cameraFov(CVar* cvar, const char*, const char* value, void*) {
        float f;
        const int result = cvar->Sync(value, &f, 90.0f, 150.0f, "%.2f");
        if (CGCamera* camera = CGCamera::GetActiveCamera()) camera->m_fov = static_cast<float>(M_PI / 200.0 * f);
        return result;
    }

    int CVarHandler_showPlayer(CVar* cvar, const char*, const char* value, void*) {
        int f;
        const int val = cvar->Sync(value, &f, 0, 1, "%d");
        if (CGPlayer_C* player = ObjectMgr::Get<CGPlayer_C>(ObjectMgr::GetPlayerGuid(), TYPEMASK_PLAYER)) {
            player->m_scaleX = (f == 0) ? 0.0f : 1.0f;
        }
        return val;
    }

    int CVarHandler_cameraIndirectVisibility(CVar* cvar, const char*, const char* value, void*) {
        int f;
        const int result = cvar->Sync(value, &f, 0, 1, "%d");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        if (std::atoi(value) == 0) {
            for (CM2Model* model : g_models_being_faded) {
                if (g_models_original_alphas.contains(model)) {
                    model->m_alpha = g_models_original_alphas[model];
                }
            }
            g_models_being_faded.clear();
            g_models_original_alphas.clear();
            g_models_current.clear();

            Hooks::Detach(&IterateCollisionListFn, IterateCollisionListHk);
            Hooks::Detach(&IterateWorldObjCollisionListFn, IterateWorldObjCollisionListHk);
            Hooks::Detach(&IntersectCallFn, IntersectCallHk);
        }
        else {
            Hooks::Detour(&IterateCollisionListFn, IterateCollisionListHk);
            Hooks::Detour(&IterateWorldObjCollisionListFn, IterateWorldObjCollisionListHk);
            Hooks::Detour(&IntersectCallFn, IntersectCallHk);
        }
        DetourTransactionCommit();
        return result;
    }

    int CVarHandler_cameraIndirectAlpha(CVar* cvar, const char*, const char* value, void*) {
        return cvar->Sync(value, &g_indirectAlpha, 0.6f, 1.0f, "%.2f");
    }
}

void Camera::initialize() {
    g_models_current.reserve(64);
    g_models_being_faded.reserve(64);
    g_models_original_alphas.reserve(64);
    g_models_grace_timers.reserve(64);

    Hooks::FrameXML::registerCVar(&s_cvar_showPlayer, "showPlayer", nullptr, "1", CVarHandler_showPlayer);
    Hooks::FrameXML::registerCVar(&s_cvar_cameraFov, "cameraFov", nullptr, "100", CVarHandler_cameraFov);
    Hooks::FrameXML::registerCVar(&s_cvar_cameraIndirectAlpha, "cameraIndirectAlpha", nullptr, "0.6", CVarHandler_cameraIndirectAlpha);
    Hooks::FrameXML::registerCVar(&s_cvar_cameraIndirectVisibility, "cameraIndirectVisibility", nullptr, "0", CVarHandler_cameraIndirectVisibility);

    Hooks::Detour(&CSimpleCamera::constructorFn, CSimpleCamera__constructorHk);

    Hooks::FrameScript::registerOnEnter([]() { CVarHandler_showPlayer(s_cvar_showPlayer, nullptr, s_cvar_showPlayer->m_str, nullptr); });
    Hooks::FrameScript::registerOnLeave([]() { CVarHandler_showPlayer(s_cvar_showPlayer, nullptr, s_cvar_showPlayer->m_str, nullptr); });
}
