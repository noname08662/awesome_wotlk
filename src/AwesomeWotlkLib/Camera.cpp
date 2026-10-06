#include "Camera.h"

#include <corecrt_math_defines.h>

#include <bit>
#include <cmath>
#include <cstddef>

#include "Extensions.h"

#include "include/Camera/CGCamera.h"
#include "include/M2/CM2Model.h"
#include "include/M2/CM2Scene.h"
#include "include/Map/CMapDoodadDef.h"
#include "include/Math/Primitives.h"
#include "include/ObjectManager/CGPlayer_C.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/ObjectManager/ObjectManagerEnums.h"
#include "include/Widget/CSimpleCamera.h"
#include "include/World/CGWorldFrame.h"

// since I wanted to experiment with access patterns and lifetime, this version is kind of cluttered
// if you're looking for something easy, look for the older commits

namespace {
class CM2ModelExt : public CM2Model {
public:
    struct FadeState {
        enum class Phase : uint8_t { eIdle, eFading, eCurrent };

        CM2ModelExt* fade_next = nullptr;
        float original_alpha = 0.0f;
        uint32_t grace_timer = 0;
        Phase phase = Phase::eIdle;
    };

    static CM2ModelExt* of(CM2Model* model) { return reinterpret_cast<CM2ModelExt*>(model); }

    [[nodiscard]]
    FadeState& fade() const {
        return *static_cast<FadeState*>(_alignment7C);
    }

    void fadeCreate() { _alignment7C = new FadeState{}; }

    void fadeDestroy() {
        delete &fade();
        _alignment7C = nullptr;
    }

    void stepAlphaTo(float target) { fade_alpha_ += (target - fade_alpha_) * 0.25f; }

    bool restoreAlpha(bool step) {
        if (step) {
            const float original_alpha = fade().original_alpha;
            stepAlphaTo(original_alpha);
            if (std::fabs(fade_alpha_ - original_alpha) >= 0.01f) { return false; }
            fade_alpha_ = original_alpha;
        }
        fade_alpha_ = fade().original_alpha;
        return true;
    }
};

class CM2SceneExt : public CM2Scene {
public:
    static CM2SceneExt* of(CM2Scene* scene) { return reinterpret_cast<CM2SceneExt*>(scene); }

    struct FadeState {
        CM2ModelExt* fade_list = nullptr;
        CM2ModelExt* pending_hit = nullptr;
        void* raw_def = nullptr;
        int active_fade_count = 0;
        float actual_hit_dist = 1.0f;

        void push(CM2ModelExt* model) {
            CM2ModelExt::FadeState& state = model->fade();
            state.fade_next = fade_list;
            state.original_alpha = model->fade_alpha_;
            state.phase = CM2ModelExt::FadeState::Phase::eFading;
            fade_list = model;
            ++active_fade_count;
        }

        void unlink(CM2ModelExt* prev, CM2ModelExt* node) {
            CM2ModelExt::FadeState& state = node->fade();
            if (prev != nullptr) {
                prev->fade().fade_next = state.fade_next;
            } else {
                fade_list = state.fade_next;
            }
            state.fade_next = nullptr;
            state.phase = CM2ModelExt::FadeState::Phase::eIdle;
            --active_fade_count;
        }

        void clear() {
            for (CM2ModelExt* model = fade_list; model != nullptr;) {
                CM2ModelExt::FadeState& state = model->fade();
                CM2ModelExt* next = state.fade_next;
                model->restoreAlpha(false);
                state = CM2ModelExt::FadeState{};
                model = next;
            }
            fade_list = nullptr;
            pending_hit = nullptr;
            raw_def = nullptr;
            active_fade_count = 0;
            actual_hit_dist = 1.0f;
        }
    };

    [[nodiscard]]
    FadeState& fade() const {
        return *reinterpret_cast<FadeState*>(last_hit_doodad_def_);
    }

    void fadeCreate() { last_hit_doodad_def_ = reinterpret_cast<CMapDoodadDef*>(new FadeState{}); }

    void fadeDestroy() {
        delete reinterpret_cast<FadeState*>(last_hit_doodad_def_);
        last_hit_doodad_def_ = nullptr;
    }
};

HOOKKIT_BIND(CM2Scene::ctor_hook, [](CM2Scene* self, CM2Cache* cache) {
    CM2Scene* result = self->ctor(cache);
    CM2SceneExt::of(result)->fadeCreate();
    return result;
});

HOOKKIT_BIND(CM2Scene::dtor_hook, [](CM2Scene* self) {
    CM2SceneExt::of(self)->fadeDestroy();
    self->dtor();
});

HOOKKIT_BIND(CM2Model::ctor_hook, [](CM2Model* self) {
    CM2Model* result = self->ctor();
    CM2ModelExt::of(result)->fadeCreate();
    return result;
});

HOOKKIT_BIND(CM2Model::dtor_hook, [](CM2Model* self) {
    CM2ModelExt* target = CM2ModelExt::of(self);
    if (target->fade().phase != CM2ModelExt::FadeState::Phase::eIdle) {
        CM2SceneExt* scenex = CM2SceneExt::of(CM2Scene::get());
        CM2SceneExt::FadeState& scene_state = scenex->fade();
        CM2ModelExt* prev = nullptr;
        for (CM2ModelExt* node = scene_state.fade_list; node != nullptr;) {
            CM2ModelExt* next = node->fade().fade_next;
            if (node == target) {
                scenex->fade().unlink(prev, node);
                break;
            }
            prev = node;
            node = next;
        }
    }
    target->fadeDestroy();
    self->dtor();
});

HOOKKIT_BIND(CGWorldFrame::getDoodadHitDetails_hook, [](WorldIntersectHitBuffer* buf, void* model_data) {
    CM2SceneExt::FadeState& scene_state = CM2SceneExt::of(CM2Scene::get())->fade();
    if (model_data != &scene_state) {
        CGWorldFrame::getDoodadHitDetails{}(buf, model_data);
        return;
    }

    auto* def = static_cast<CMapDoodadDef*>(scene_state.raw_def);
    constexpr uint16_t kM2Kinds = CMapBaseObj::eKindM2Doodad | CMapBaseObj::eKindM2DoodadInWmo;
    if (def == nullptr || (def->CMapBaseObj::flags_ & kM2Kinds) == 0) { return; }

    CM2Model* root = def->model_;
    if (root == nullptr) { return; }
    while (root->attach_parent_ != nullptr) {
        root = root->attach_parent_;
    }
    scene_state.pending_hit = CM2ModelExt::of(root);
});

HOOKKIT_NAMED_BIND_RAW(intersect_call, 0x006060E6, {"jmpback", 0x00606103}) {
    const uintptr_t intersect =
        HOOKKIT_LAMBDA_ADDR([](Vec3f* player_pos, Vec3f* camera_pos, Vec3f* hit_point, float* hit_distance,
                                uint32_t hit_flags, WorldIntersectHitBuffer*) {
            static WorldIntersectHitBuffer sentinel_buffer;

            CM2SceneExt::FadeState& scene_state = CM2SceneExt::of(CM2Scene::get())->fade();
            scene_state.pending_hit = nullptr;

            bool result = CGWorldFrame::intersect{}(
                player_pos, camera_pos, hit_point, hit_distance, hit_flags | 1u | 0x8000000u, &sentinel_buffer);

            CM2ModelExt* hit_model = nullptr;
            if (result) {
                if (CM2ModelExt* model = scene_state.pending_hit) {
                    hit_model = model;
                    auto& fade_state = model->fade();
                    if (fade_state.phase == CM2ModelExt::FadeState::Phase::eIdle) { scene_state.push(model); }

                    model->stepAlphaTo(extensions::console::kCvarRegistry->ref<"cameraIndirectAlpha", float>());
                    fade_state.grace_timer = 0;

                    if (scene_state.actual_hit_dist < 1.0f) {
                        *hit_distance = scene_state.actual_hit_dist;
                    } else {
                        result = false;
                    }
                } else {
                    scene_state.actual_hit_dist = *hit_distance;
                }
            } else if (scene_state.fade_list == nullptr) {
                return result;
            } else {
                scene_state.actual_hit_dist = 1.0f;
            }

            const uint32_t cleanup_timer = scene_state.active_fade_count + 1;
            CM2ModelExt* prev = nullptr;
            CM2ModelExt* model = scene_state.fade_list;
            while (model != nullptr) {
                auto& model_state = model->fade();
                CM2ModelExt* next = model_state.fade_next;

                if (model_state.grace_timer > cleanup_timer && model->restoreAlpha(true)) {
                    scene_state.unlink(prev, model);
                } else {
                    if (hit_model == nullptr) {
                        model_state.phase = CM2ModelExt::FadeState::Phase::eFading;
                    } else if (model == hit_model) {
                        model_state.phase = CM2ModelExt::FadeState::Phase::eCurrent;
                    }
                    ++model_state.grace_timer;
                    prev = model;
                }
                model = next;
            }
            return result;
        });
    constexpr uintptr_t kJmp = intersect_call::target("jmpback");
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .scratchReg(Reg::eCx)
        .step(fld1Step())
        .step(pushImm(0))
        .step(andReg(Reg::eBx, ~1))
        .step(pushReg(Reg::eBx))
        .step(fstpMem(Reg::eBp, 0x18))
        .step(pushLea(Reg::eBp, 0x18))   // hit dist
        .step(pushLea(Reg::eBp, -0x48))  // hit point
        .step(pushLea(Reg::eBp, -0x18))  // end
        .step(pushLea(Reg::eBp, -0x24))  // start
        .build(intersect, jmpTo(kJmp), 0);
};

HOOKKIT_NAMED_BIND_RAW(raycastModelsHit_site, 0x0081E0F5, {"jmpback", 0x0081E0FB}) {
    constexpr auto kRawDefSlot = static_cast<std::int32_t>(offsetof(CM2SceneExt::FadeState, raw_def));
    constexpr uintptr_t kJmp = raycastModelsHit_site::target("jmpback");
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(movRegMem(Reg::eCx, Reg::eSi, 0x13C))  // last_hit_doodad_def_
        .step(movMemReg(Reg::eCx, kRawDefSlot, Reg::eAx))
        .build(jmpTo(kJmp));
};

HOOKKIT_NAMED_BIND_RAW(iterateCollisionList_site, 0x007A279D, {"jmpback", 0x007A27A5}, {"skip", 0x007A2943}) {
    const uintptr_t verdict = HOOKKIT_LAMBDA_ADDR([](CMapDoodadDef* doodad_def, uint32_t flags) {
        if ((flags & 0x8000000) == 0) { return 1; }
        CM2Model* model = doodad_def->model_;
        if (model == nullptr) { return 0; }
        return CM2ModelExt::of(model)->fade().phase != CM2ModelExt::FadeState::Phase::eCurrent ? 1 : 0;
    });
    constexpr uintptr_t kJmpback = iterateCollisionList_site::target("jmpback");
    constexpr uintptr_t kSkip = iterateCollisionList_site::target("skip");
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .scratchReg(Reg::eCx)
        .step(movRegMem(Reg::eSi, Reg::eDx, 4))
        .step(pushfdStep())
        .step(pushRegs(Reg::eAx, Reg::eCx, Reg::eDx))
        .step(pushMem(Reg::eBp, 0x0C))
        .step(pushReg(Reg::eSi))
        .build(verdict,
            branchOnResult(jmpTo(kJmpback, cmpMemReg8(Reg::eSi, 0x25, Reg::eBx)), jmpTo(kSkip),
                popRegs(Reg::eAx, Reg::eCx, Reg::eDx), popfdStep()),
            8);
};

HOOKKIT_NAMED_BIND_RAW(iterateWorldObjCollisionList_site, 0x007A2A1C, {"jmpback", 0x007A2A23}, {"skip", 0x007A2A8A}) {
    const uintptr_t verdict = HOOKKIT_LAMBDA_ADDR([](CM2Model* model, uint32_t flags) {
        if ((flags & 0x8000000) == 0) { return 1; }
        return CM2ModelExt::of(model)->fade().phase != CM2ModelExt::FadeState::Phase::eCurrent ? 1 : 0;
    });
    constexpr uintptr_t kJmpback = iterateWorldObjCollisionList_site::target("jmpback");
    constexpr uintptr_t kSkip = iterateWorldObjCollisionList_site::target("skip");
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .scratchReg(Reg::eCx)
        .step(pushMem(Reg::eBp, 0x0C))
        .step(pushReg(Reg::eAx))
        .build(verdict,
            branchOnResult(
                jmpTo(kJmpback, movRegMem(Reg::eAx, Reg::eBp, -0x68), cmpMemImm(Reg::eAx, 0x2D8, 0)), jmpTo(kSkip)));
};

HOOKKIT_BIND(CSimpleCamera::ctor_hook, [](CSimpleCamera* self, float near_clip, float far_clip, float fov) {
    const auto f = extensions::console::kCvarRegistry->get<"cameraFov", float>();
    fov = static_cast<float>(M_PI / 200.0 * f);
    return self->ctor(near_clip, far_clip, fov);
});
}  // namespace

namespace {
void applyIndirectVisibility(int enabled) {
    if ((enabled != 0) == intersect_call_hook::attached.load()) { return; }

    hookkit::HookTransaction tx;
    if (!tx) { return; }
    if (enabled == 0) {
        tx.detach(intersect_call_hook{}, CGWorldFrame::getDoodadHitDetails_hook{}, iterateCollisionList_site{},
            iterateWorldObjCollisionList_site{});
    } else {
        tx.attach(intersect_call_hook{}, CGWorldFrame::getDoodadHitDetails_hook{}, iterateCollisionList_site{},
            iterateWorldObjCollisionList_site{});
    }
    if (tx.commit() != NO_ERROR) { return; }

    if (enabled == 0) {
        if (CM2Scene* scene = CM2Scene::get()) { CM2SceneExt::of(scene)->fade().clear(); }
    }
}
}  // namespace

void camera::initialize(hookkit::HookTransaction& tx) {
    auto& cvars = *extensions::console::kCvarRegistry;

    if (CM2Scene* scene = CM2Scene::get()) { CM2SceneExt::of(scene)->fadeCreate(); }

    cvars.add<int>({
        .name = "showPlayer",
        .init = 1,
        .min{0},
        .max{1},
        .on_change = [](int show) {
            if (auto* player = object_mgr::get<CGPlayer_C>(object_mgr::getPlayerGuid(), eTypemaskPlayer)) {
                player->scale_ = (show == 0) ? 0.0f : 1.0f;
            }
        }
    });
    cvars.add<float>({
        .name = "cameraFov",
        .init = 100.0f,
        .min{90.0f},
        .max{150.0f},
        .on_change = [](float fov) {
            if (CGCamera* camera = CGCamera::get{}()) { camera->fov_ = static_cast<float>(M_PI / 200.0 * fov); }
        }
    });
    cvars.add<float>({.name = "cameraIndirectAlpha", .init = 0.6f, .min{0.6f}, .max{1.0f}});
    cvars.add<int>(
        {.name = "cameraIndirectVisibility", .init = 0, .min{0}, .max{1}, .on_change = applyIndirectVisibility});

    extensions::framescript::kOnEnter->add([]() { extensions::console::kCvarRegistry->refresh("showPlayer"); });
    extensions::framescript::kOnLeave->add([]() { extensions::console::kCvarRegistry->refresh("showPlayer"); });

    tx.attach(CSimpleCamera::ctor_hook{}, CM2Model::ctor_hook{}, CM2Model::dtor_hook{}, CM2Scene::ctor_hook{},
        CM2Scene::dtor_hook{}, raycastModelsHit_site{});
}
