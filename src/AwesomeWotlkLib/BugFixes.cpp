#include "BugFixes.h"

#include <d3d9.h>
#include <hookkit/hookkit.h>
#include <windef.h>

#include <array>
#include <cstdint>
#include <cstring>
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
#include "include/Widget/CSimpleTexture.h"

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
// 004BEE60's D3D9 half-pixel offset adds 0.5 * rect_ddc / window_px to a clip-space translation,
// shifting the UI by rect_ddc/4 px instead of 0.5 px; every 1:1 texel then blends ~30%
// with its neighbour and footprints lose their right/bottom texels
HOOKKIT_NAMED_BIND_RAW(screenHalfPixel_site, 0x004BEFE9, {"jmpback", 0x004BF01E}) {
    constexpr uintptr_t kJmpback = screenHalfPixel_site::target("jmpback");
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

HOOKKIT_NAMED_HOOK(fetchGxTex_call, 0x00484DF5, {"jmpback", 0x00484DFA});

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

template <bool Sampling, bool Snap>
void* fetchGxTexDetour() {
    static void* const detour = CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(
        reinterpret_cast<uintptr_t>(&fetchGxTexWithConstants<Sampling, Snap>),
        jmpTo(fetchGxTex_call::target("jmpback")));
    return detour;
}

void applyConstantsHook() {
    const bool sampling =
        static_cast<SamplingMode>(extensions::console::kCvarRegistry->get<"uiTextureSampling", int>()) != eEngine;
    const bool snap = extensions::console::kCvarRegistry->get<"uiPixelSnap", int>() != 0;
    void* detour = nullptr;
    if (sampling && snap) {
        detour = fetchGxTexDetour<true, true>();
    } else if (sampling) {
        detour = fetchGxTexDetour<true, false>();
    } else if (snap) {
        detour = fetchGxTexDetour<false, true>();
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
}  // namespace

void bug_fixes::initialize(hookkit::HookTransaction& tx) {
    auto& cvars = *extensions::console::kCvarRegistry;
    cvars.add<int>({.name = "uiHalfPixelFix", .init = 1, .min{0}, .max{1}, .on_change = applyHalfPixelFix});
    cvars.add<int>(
        {.name = "uiTextureSampling", .init = eBox, .min{eEngine}, .max{eTent}, .on_change = applySamplingMode});
    cvars.add<int>({.name = "uiPixelSnap", .init = 1, .min{0}, .max{1}, .on_change = applyPixelSnap});
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

    // z sample offs; 2.0f -> 50.0f
    std::array<uint8_t, 6> proj_depth_patch = {0xD9, 0x05, 0xA8, 0x93, 0xA3, 0x00};
    tx.patchBytes(reinterpret_cast<void*>(0x00820809), proj_depth_patch.data(), proj_depth_patch.size());

    tx.attach(os::clipboardGetStr_hook{}, os::clipboardSetStr_hook{}, CGxDevice::initUIShaders_hook{});
}
