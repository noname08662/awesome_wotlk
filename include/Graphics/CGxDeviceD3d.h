#pragma once

#include <d3d9.h>
#include <hookkit/hook.h>

#include <cstddef>
#include <cstdint>

#include "BaseTypes.h"

#include "Graphics/CGxDevice.h"

struct GxVertexDecl;

class CGxDeviceD3d : public CGxDevice {
public:
    HWND window_;
    uint16_t window_class_atom_;
    uint16_t _pad396E;
    unk_t _unk3970;
    HMODULE d3d_lib_;
    IDirect3D9* d3d_;
    IDirect3DDevice9* d3d_device_;
    D3DCAPS9 caps_;
    unk_t _unk3AB0[2];
    uint32_t stereo_mode_;
    unk_t _unk3ABC[2];
    float stereo_convergence_;
    float stereo_separation_;
    uint32_t stereo_dirty_;
    TSGrowableArray<GxVertexDecl> vertex_decls_;
    IDirect3DVertexDeclaration9* vertex_format_decls_[14];
    D3DDISPLAYMODE adapter_mode_;
    unk_t _unk3B28;
    unk_t _unk3B2C;
    unk_t _unk3B30;
    D3DFORMAT adapter_format_;
    IDirect3DSurface9* rt_depth_surface_;
    IDirect3DSurface9* default_color_surface_;
    IDirect3DSurface9* default_depth_surface_;
    IDirect3DSurface9* readback_surface_;
    IUnknown* _unk3B48;
    uint32_t cursor_dirty_;
    IDirect3DTexture9* cursor_texture_;
    IDirect3DSurface9* cursor_surface_;
    unk_t _unk3B58;
    IDirect3DVertexDeclaration9* cur_vertex_decl_;
    unk_t _unk3B60[25];
    uint32_t ds_cache_[eGxDsCount];
    uint8_t stage_has_texture_[8];

    static constexpr uintptr_t kD3dVtbl = 0x00A2E718;
    static constexpr uintptr_t kD3d9ExVtbl = 0x00A2F500;

    // the highest caps_.shader_targets index ISetCaps reports for d3d9 (shader model 3: vs_3_0 = 3, ps_3_0 = 4); a
    // DeviceOverride(0, v) writes the pixel target past it unchecked
    static constexpr uint32_t maxShaderProfile(GxShaderType type) { return type == eGxShaderPixel ? 4 : 3; }

    [[nodiscard]]
    static CGxDeviceD3d* of(CGxDevice* device) {
        if (device == nullptr) { return nullptr; }
        const uintptr_t vtbl = *reinterpret_cast<const uintptr_t*>(device);
        if (vtbl != kD3dVtbl && vtbl != kD3d9ExVtbl) { return nullptr; }
        return static_cast<CGxDeviceD3d*>(device);
    }

    HOOKKIT_HOOK(deviceSetFormat, 0x006904D0, ::hookkit::Conv::eThiscall, int, CGxDeviceD3d*, const void*);
    HOOKKIT_HOOK(destroyD3d, 0x006903B0, ::hookkit::Conv::eThiscall, int, CGxDeviceD3d*);
    HOOKKIT_HOOK(releaseD3dResources, 0x00690150, ::hookkit::Conv::eThiscall, int, CGxDeviceD3d*, int);
    // cached d3d render/sampler/texture-stage state (raw d3d values, samplers 0-15); the cache is reset on release
    HOOKKIT_HOOK(dsSet, 0x006A3C40, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, GxD3dDsState, uint32_t);

    // vtable impls for the d3d9 (kD3dVtbl) and d3d9ex (kD3d9ExVtbl) devices; SceneClear is shared
    HOOKKIT_HOOK(implScenePresent, 0x006A3450, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*);
    HOOKKIT_HOOK(implScenePresentEx, 0x006A7610, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*);
    HOOKKIT_HOOK(implSceneClear, 0x006A74B0, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, uint32_t, uint32_t);
    HOOKKIT_HOOK(implDraw, 0x006A3620, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, const CGxBatch*, int32_t);
    HOOKKIT_HOOK(implDrawEx, 0x006A77C0, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, const CGxBatch*, int32_t);
    // target (0 color, 1 depth), render target texture or null for the default surface, cube face
    HOOKKIT_HOOK(
        implSetRenderTarget, 0x0068F770, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, uint32_t, CGxTex*, uint32_t);
    HOOKKIT_HOOK(implReadPixels, 0x0068FED0, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, Recti*,
        TSGrowableArray<ColorBGRA<>>*);
    HOOKKIT_HOOK(implReadPixelsEx, 0x006A16D0, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, Recti*,
        TSGrowableArray<ColorBGRA<>>*);
    HOOKKIT_HOOK(ensureReadbackSurface, 0x0068F6A0, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*);
    HOOKKIT_HOOK(implSetRenderTargetEx, 0x006A13A0, ::hookkit::Conv::eThiscall, void, CGxDeviceD3d*, uint32_t, CGxTex*,
        uint32_t);
};

static_assert(sizeof(CGxDeviceD3d) == 0x3EA4);
