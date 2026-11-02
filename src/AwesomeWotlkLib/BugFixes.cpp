#include "BugFixes.h"

#include <d3d9.h>
#include <hookkit/hookkit.h>
#include <windef.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "BugFixesShaders.h"
#include "D3D.h"
#include "Extensions.h"
#include "Utils.h"

#include "include/Graphics/CGxDeviceD3d.h"
#include "include/Lib/Storm.h"
#include "include/Math/Math.h"
#include "include/System/System.h"
#include "include/Texture/CTexture.h"
#include "include/Widget/CCooldown.h"
#include "include/Widget/CSimpleTexture.h"
#include "include/Widget/CSimpleTop.h"

namespace {
HOOKKIT_BIND(os::clipboardGetStr_hook, [](HWND hwnd) {
    std::string str = utils::getFromClipboardU8(hwnd);
    auto* buf = static_cast<char*>(storm::memory::alloc{}(str.size() + 1, __FILE__, __LINE__, 0));
    if (buf != nullptr) { std::memcpy(buf, str.c_str(), str.size() + 1); }
    return buf;
});

HOOKKIT_BIND(os::clipboardSetStr_hook, [](const char* buf, HWND hwnd) { return utils::copyToClipboardU8(buf, hwnd); });
}  // namespace

namespace {
// buffered SavedVariables write
constexpr uint32_t kSavedVariablesBufferSize = 256 * 1024;

std::atomic<void*> saved_variables_file{nullptr};
std::array<char, kSavedVariablesBufferSize> saved_variables_buffer;
uint32_t saved_variables_used = 0;

void flushSavedVariables(void* file) {
    if (saved_variables_used == 0) { return; }
    uint32_t written = 0;
    static_cast<void>(os::writeFile{}(file, saved_variables_buffer.data(), saved_variables_used, &written));
    saved_variables_used = 0;
}

void* openSavedVariables(
    const char* name, uint32_t access, uint32_t share_mode, uint32_t disposition, uint32_t attributes) {
    // a file the writer never closed (an error unwound it) still gets its bytes
    if (void* stale = saved_variables_file.exchange(nullptr)) { flushSavedVariables(stale); }
    void* file = os::createFile{}(name, access, share_mode, disposition, attributes);
    if (file != os::kInvalidFile) { saved_variables_file.store(file); }
    return file;
}

bool closeSavedVariables(void* file) {
    if (file == saved_variables_file.load()) {
        flushSavedVariables(file);
        saved_variables_file.store(nullptr);
    }
    return os::closeFile{}(file);
}

HOOKKIT_CALLSITE_HOOK(addonSavedVariablesOpen_call, 0x005F5790, os::createFile);
HOOKKIT_CALLSITE_HOOK(addonSavedVariablesClose_call, 0x005F5833, os::closeFile);
HOOKKIT_CALLSITE_HOOK(addonCharacterSavedVariablesOpen_call, 0x005F5941, os::createFile);
HOOKKIT_CALLSITE_HOOK(addonCharacterSavedVariablesClose_call, 0x005F59E5, os::closeFile);
HOOKKIT_CALLSITE_HOOK(registeredSavedVariablesOpen_call, 0x0052724D, os::createFile);
HOOKKIT_CALLSITE_HOOK(registeredSavedVariablesClose_call, 0x005272EA, os::closeFile);
HOOKKIT_BIND(addonSavedVariablesOpen_call, openSavedVariables);
HOOKKIT_BIND(addonSavedVariablesClose_call, closeSavedVariables);
HOOKKIT_BIND(addonCharacterSavedVariablesOpen_call, openSavedVariables);
HOOKKIT_BIND(addonCharacterSavedVariablesClose_call, closeSavedVariables);
HOOKKIT_BIND(registeredSavedVariablesOpen_call, openSavedVariables);
HOOKKIT_BIND(registeredSavedVariablesClose_call, closeSavedVariables);

HOOKKIT_BIND(os::writeFile_hook, [](void* file, const void* data, uint32_t count, uint32_t* written) -> int {
    void* buffered = saved_variables_file.load(std::memory_order_relaxed);
    if (buffered == nullptr || file != buffered || data == nullptr || written == nullptr) {
        return os::writeFile{}(file, data, count, written);
    }
    if (count > kSavedVariablesBufferSize - saved_variables_used) {
        flushSavedVariables(file);
        if (count >= kSavedVariablesBufferSize) { return os::writeFile{}(file, data, count, written); }
    }
    std::copy_n(static_cast<const char*>(data), count, std::next(saved_variables_buffer.begin(), saved_variables_used));
    saved_variables_used += count;
    *written = count;
    return 1;
});
}  // namespace

namespace {
// 004BEE60's D3D9 half-pixel offset adds 0.5 * rect_ddc / window_px to a clip-space translation,
// shifting the UI by rect_ddc/4 px instead of 0.5 px; every 1:1 texel then blends ~30%
// with its neighbour and footprints lose their right/bottom texels
HOOKKIT_NAMED_BIND_RAW(screenHalfPixel_site, 0x004BEFE9, 0x004BF01E) {
    constexpr uintptr_t kJmpback = screenHalfPixel_site::target();
    const uintptr_t fix = HOOKKIT_LAMBDA_ADDR([](Mat4f* dst, const Rectf* win, const Rectf* rect) {
        const Vec2f ndc = math::ddcToNdc({rect->width(), rect->height()});
        const Vec2f px = ndc * Vec2f{win->width(), win->height()};
        if (px.x > 0.0f) { dst->m[3][0] -= 1.0f / px.x; }
        if (px.y > 0.0f) { dst->m[3][1] += 1.0f / px.y; }
    });
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(pushReg(Reg::eSi))         // rect
        .step(pushLea(Reg::eBp, -0x10))  // window size in screen coords
        .step(pushLea(Reg::eBp, -0x54))  // dst
        .build(fix, jmpTo(kJmpback));
};

void applyHalfPixelFix(int enabled) {
    if ((enabled != 0) == screenHalfPixel_site::attached.load()) { return; }
    hookkit::HookTransaction tx;
    if (!tx) { return; }
    if (enabled != 0) {
        tx.attach(screenHalfPixel_site{});
    } else {
        tx.detach(screenHalfPixel_site{});
    }
    static_cast<void>(tx.commit());
}
}  // namespace

namespace {
// every UI texture is loaded with 0xAC0F00 = linear, no mip filter, and drawn by
// Shaders\Pixel\UI (tex2D * color), a minified quad therefore takes one bilinear tap per pixel, so thin
// features in the art (borders, glyph-like detail) show or vanish depending on the sub-pixel phase
constexpr uint32_t kSamplingConstRegister = 222;  // c222 texel size; c220/c221 belong to MSDF

enum SamplingMode : int { eEngine = 0, eBox = 1, eTent = 2 };

constexpr std::array kSamplingShaders = {CGxShaderExt::Name{"UISampling"}, CGxShaderExt::Name{"UISamplingDesaturate"}};
constexpr std::array<const char*, 2> kSamplingEntries = {"mainNormal", "mainDesaturate"};

// uiPixelSnap: Shaders\Vertex\UI:0/1 plus a round to the nearest pixel edge
constexpr CGxShaderExt::Name kSnapShader{"UISnap"};
constexpr uint32_t kSnapConstRegister = 222;  // vertex c222 viewport size; MSDF's vertex constant is c220

constexpr float kMinSnapPx = 0.25f;  // thinner than this is left alone

HOOKKIT_CALLSITE_HOOK(fetchGxTex_call, 0x00484DF5, CTexture::fetchGxTex);

template <bool Sampling, bool Snap>
CGxTex* fetchGxTexWithConstants(CTexture* texture, int rw_flag, CStatus* status) {
    CGxTex* gx_tex = CTexture::fetchGxTex{}(texture, rw_flag, status);
    CGxDeviceD3dExt* gx = CGxDeviceD3dExt::of();
    if (gx == nullptr) { return gx_tex; }
    if constexpr (Sampling) {
        const float width = gx_tex != nullptr ? static_cast<float>(gx_tex->width_) : 0.0f;
        const float height = gx_tex != nullptr ? static_cast<float>(gx_tex->height_) : 0.0f;
        const std::array constants = {
            width, height, width > 0.0f ? 1.0f / width : 0.0f, height > 0.0f ? 1.0f / height : 0.0f};
        gx->setConstants(eGxShaderPixel, kSamplingConstRegister, constants);
    }
    if constexpr (Snap) {
        const Vec2f vp = gx->viewportPixelSize();
        const std::array viewport = {vp.x, vp.y, vp.x > 0.0f ? 1.0f / vp.x : 0.0f, vp.y > 0.0f ? 1.0f / vp.y : 0.0f};
        gx->setConstants(eGxShaderVertex, kSnapConstRegister, viewport);
    }
    return gx_tex;
}

Vec3f* thinQuadWiden(CSimpleTexture* self, const Rectf* rect, Vec3f* out) {
    Vec3f* quad = CSimpleTexture::calcQuadVertices{}(self, rect, out);
    const CGxDeviceD3dExt* gx = CGxDeviceD3dExt::of();
    if (gx == nullptr) { return quad; }
    const Vec2f vp = gx->viewportPixelSize();
    if (vp.x <= 0.0f || vp.y <= 0.0f) { return quad; }
    const Vec2f aspect = math::aspectNormal();
    const Vec2f ddc_per_px = {aspect.x / vp.x, aspect.y / vp.y};
    // corners: 0 = (left, top), 1 = (left, bottom), 2 = (right, top), 3 = (right, bottom)
    const float right = utils::minPixelSpan(kMinSnapPx, ddc_per_px.x, rect->left, rect->right);
    const float bottom = utils::minPixelSpan(kMinSnapPx, ddc_per_px.y, rect->top, rect->bottom);
    std::span quad_view{quad, 4};
    quad_view[2].x = right;
    quad_view[3].x = right;
    quad_view[1].y = bottom;
    quad_view[3].y = bottom;
    return quad;
}

void applyConstantsHook() {
    const bool sampling =
        static_cast<SamplingMode>(extensions::console::kCvarRegistry->get<"uiTextureSampling", int>()) != eEngine;
    const bool snap = extensions::console::kCvarRegistry->get<"uiPixelSnap", int>() != 0;

    static_cast<void>(hookkit::HookTransaction::reinstallIf<thinQuadWiden>(CSimpleTexture::calcQuadVertices{}, snap));

    void* detour = nullptr;
    if (sampling && snap) {
        detour = fetchGxTex_call::staticDetour<fetchGxTexWithConstants<true, true>>();
    } else if (sampling) {
        detour = fetchGxTex_call::staticDetour<fetchGxTexWithConstants<true, false>>();
    } else if (snap) {
        detour = fetchGxTex_call::staticDetour<fetchGxTexWithConstants<false, true>>();
    }
    static_cast<void>(hookkit::HookTransaction::reinstall(fetchGxTex_call{}, detour));
}

void applySamplingMode(int, bool changed) {
    if (changed) {
        for (CGxShader* shader : CGxDevice::ui_pixel_shaders) {
            if (shader != nullptr) { CGxShaderExt::of(shader)->reload(); }
        }
    }
    applyConstantsHook();
}

void applyPixelSnap(int, bool changed) {
    if (changed) {
        for (CGxShader* shader : CGxDevice::ui_vertex_shaders) {
            if (shader != nullptr) { CGxShaderExt::of(shader)->reload(); }
        }
    }
    applyConstantsHook();
    if (!changed) { return; }
    if (CSimpleTop* top = CSimpleTop::get()) { top->refreshTextureQuads(); }
}

HOOKKIT_BIND(CGxDevice::initUIShaders_hook, []() {
    for (size_t i = 0; i < 2; ++i) {
        CGxDevice::getDevice()->shaderCreate(
            &CGxDevice::ui_pixel_shaders[i], eGxShaderPixel, "Shaders\\Pixel", CGxDevice::ui_pixel_shaders_desc[i], 1);
        CGxShaderExt::replace(eGxShaderPixel, {&CGxDevice::ui_pixel_shaders[i], 1}, kSamplingShaders[i]);
    }
    CGxShaderExt::replace(eGxShaderVertex, CGxDevice::ui_vertex_shaders, kSnapShader);
    applyConstantsHook();
});

// a rect change after OnLayerUpdate (nameplates placed during the world render) reaches resetVerts, which collapses
// the swipe until the next update, so it's rebuilt here; undrawn frames get no OnLayerUpdate, hence the clock, and
// past the end it holds the last step like the engine's reverse hold
HOOKKIT_BIND(CCooldown::onLayoutFrameSizeChanged_hook, [](CLayoutFrame* layout_frame, const Rectf* rect) -> int {
    const int res = CCooldown::onLayoutFrameSizeChanged_hook{}(layout_frame, rect);
    auto* self = static_cast<CCooldown*>(layout_frame);
    if (self->is_shown_ == 0 || self->duration_ == 0) { return res; }
    const uint32_t elapsed = static_cast<uint32_t>(os::getAsyncTimeMs{}()) - self->start_time_;
    self->elapsed_time_ = std::min(elapsed, self->duration_ - 1);
    self->animateVerts();
    return res;
});

void applyCooldownSwipeFix(int enabled) {
    static_cast<void>(hookkit::HookTransaction::reinstallIf(CCooldown::onLayoutFrameSizeChanged_hook{}, enabled != 0));
}
}  // namespace

void bug_fixes::initialize(hookkit::HookTransaction& tx) {
    auto& cvars = *extensions::console::kCvarRegistry;
    cvars.add<int>({.name = "uiHalfPixelFix", .init = 1, .min{0}, .max{1}, .on_change = applyHalfPixelFix});
    cvars.add<int>(
        {.name = "uiTextureSampling", .init = eBox, .min{eEngine}, .max{eTent}, .on_change = applySamplingMode});
    cvars.add<int>({.name = "uiPixelSnap", .init = 1, .min{0}, .max{1}, .on_change = applyPixelSnap});
    cvars.add<int>({.name = "uiCooldownSwipeFix", .init = 1, .min{0}, .max{1}, .on_change = applyCooldownSwipeFix});
    for (size_t i = 0; i < 2; ++i) {
        CGxShaderExt::define(eGxShaderPixel, kSamplingShaders[i],
            {
                .hlsl = bug_fixes_shaders::kPixelShaderHlsl,
                .entries = {kSamplingEntries[i]},
                .defines = [] {
                    return std::vector<CGxShaderExt::Define>{
                        {
                            .name = "TENT",
                            .value =
                                static_cast<SamplingMode>(
                                    extensions::console::kCvarRegistry->get<"uiTextureSampling", int>()) == eTent
                                ? "1"
                                : "0"
                        }
                    };
                },
                .fallback = d3d::bls::Bls{.path = "Shaders\\Pixel", .name = CGxDevice::ui_pixel_shaders_desc[i]},
                .use_fallback = [] {
                    return static_cast<SamplingMode>(
                               extensions::console::kCvarRegistry->get<"uiTextureSampling", int>()) == eEngine;
                }
            });
    }
    CGxShaderExt::define(eGxShaderVertex, kSnapShader,
        {
            .hlsl = bug_fixes_shaders::kVertexShaderHlsl,
            .entries = {"mainSnap", "mainSnapStereo"},
            .fallback = d3d::bls::Bls{.path = "Shaders\\Vertex", .name = "UI"},
            .use_fallback = [] { return extensions::console::kCvarRegistry->get<"uiPixelSnap", int>() == 0; }
        });

    tx.attach(os::clipboardGetStr_hook{}, os::clipboardSetStr_hook{}, CGxDevice::initUIShaders_hook{});
    tx.attach(os::writeFile_hook{}, addonSavedVariablesOpen_call{}, addonSavedVariablesClose_call{},
        addonCharacterSavedVariablesOpen_call{}, addonCharacterSavedVariablesClose_call{},
        registeredSavedVariablesOpen_call{}, registeredSavedVariablesClose_call{});
}
