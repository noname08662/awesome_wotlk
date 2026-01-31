#include "Hooks.h"
#include "D3D.h"
#include <Detours/detours.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace D3D {
    Present_t oPresent = nullptr;
    BeginScene_t oBeginScene = nullptr;
    EndScene_t oEndScene = nullptr;
    DrawPrimitive_t oDrawPrimitive = nullptr;
    DrawIndexedPrimitive_t oDrawIndexedPrimitive = nullptr;
    SetTexture_t oSetTexture = nullptr;
    SetRenderState_t oSetRenderState = nullptr;
    SetVertexShader_t oSetVertexShader = nullptr;
    SetPixelShader_t oSetPixelShader = nullptr;
    CreateTexture_t oCreateTexture = nullptr;
    SetRenderTarget_t oSetRenderTarget = nullptr;
    Clear_t oClear = nullptr;
    Reset_t oReset = nullptr;

    namespace {
        void LogShaderError(ID3DBlob* pError, uint32_t type) {
            if (pError) {
//#ifdef _DEBUG
                char buf[2048];
                _snprintf_s(buf, sizeof(buf), _TRUNCATE, "[AwesomeWotlk] %s Shader Error: %s\n",
                    type == 1 ? "Vertex" : "Pixel", reinterpret_cast<const char*>(pError->GetBufferPointer()));
                OutputDebugStringA(buf);
//#endif
                pError->Release();
            }
        }

        enum class ResourceType {
            Texture,
            RenderTarget,
            ShaderVertex,
            ShaderPixel
        };

        struct ManagedResource {
            IUnknown** ppResource;
            ResourceType type;
            ResourceParams params;
        };
        std::vector<ManagedResource> g_managedResources;

        void CleanupManagedResources() {
            for (auto& managed : g_managedResources) {
                if (managed.ppResource && *managed.ppResource) {
                    (*managed.ppResource)->Release();
                    *managed.ppResource = nullptr;
                }
                if (managed.params.ppSurface && *managed.params.ppSurface) {
                    (*managed.params.ppSurface)->Release();
                    *managed.params.ppSurface = nullptr;
                }
            }
        }

        void RestoreManagedResources() {
            if (!GetDevice()) return;
            for (auto& managed : g_managedResources) {
                if (!managed.ppResource || *managed.ppResource) continue;

                bool needsCleanup = managed.params.autoCleanup;
                managed.params.autoCleanup = false;

                switch (managed.type) {
                case ResourceType::Texture:
                    CreateTexture(reinterpret_cast<IDirect3DTexture9**>(managed.ppResource), managed.params);
                    break;
                case ResourceType::RenderTarget:
                    CreateRenderTarget(reinterpret_cast<IDirect3DSurface9**>(managed.ppResource), managed.params);
                    break;
                case ResourceType::ShaderVertex:
                    *managed.ppResource = CompileVertexShader(managed.params);
                    break;
                case ResourceType::ShaderPixel:
                    *managed.ppResource = CompilePixelShader(managed.params);
                    break;
                }
                managed.params.autoCleanup = needsCleanup;
            }
        }

        void RegisterForCleanup(IUnknown** ppRes, ResourceType type, ResourceParams p) {
            for (auto& managed : g_managedResources) {
                if (managed.ppResource == ppRes) {
                    managed.params = p;
                    return;
                }
            }
            g_managedResources.push_back({ ppRes, type, p });
        }

        static const ShaderEntry s_shaders_engine[] {
            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0000 }, 76, "ps_3_0",  "float4 main() : COLOR { return float4(1, 0, 1, 1); }"}, // UI
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x3E991687 }, 128, "ps_3_0", "float4 main() : COLOR { return float4(1, 0, 0, 1); }" }, // UI
            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x90080000 }, 88, "ps_3_0",  "float4 main() : COLOR { return float4(0, 1, 1, 1); }" }, // Water trails
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x3E99999A }, 336, "ps_3_0", "float4 main() : COLOR { return float4(1, 1, 1, 1); }" }, // Unk env/world?
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x3F800000 }, 124, "ps_3_0", "float4 main() : COLOR { return float4(0, 0, 1, 1); }" }, // Unk
            //{ { 0xFFFF0200, 0x05000051, 0xA00F0000, 0x3E800000 }, 304, "ps_2_0", "float4 main() : COLOR { return float4(1, 0, 0, 1); }" }, // Unk
            //{ { 0xFFFF0200, 0x05000051, 0xA00F0000, 0x3EC00000 }, 280, "ps_2_0", "float4 main() : COLOR { return float4(1, 0, 0, 1); }" }, // Unk
            //{ { 0xFFFF0200, 0x05000051, 0xA00F0000, 0x3F800000 }, 204, "ps_2_0", "float4 main() : COLOR { return float4(0, 0, 1, 1); }" }, // Unk
            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0000 }, 124, "ps_3_0", "float4 main() : COLOR { return float4(0, 1, 0, 1); }" }, // Obj and non-live model bodies
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x40000000 }, 260, "ps_3_0", "float4 main() : COLOR { return float4(1, 1, 0, 1); }" }, // Weps/Armor on non-live models
            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0000 }, 196, "ps_3_0", "float4 main() : COLOR { return float4(0, 0, 1, 1); }" }  // Materials on non-live models
            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0000 }, 128, "ps_3_0", "float4 main() : COLOR { return float4(0, 1, 0, 1); }" }, // Glow/Fog FX
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x00000000 }, 244, "ps_3_0", "float4 main() : COLOR { return float4(1, 1, 0, 1); }" }, // Lightning FX
            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0000 }, 200, "ps_3_0", "float4 main() : COLOR { return float4(1, 1, 0, 1); }" },  // Ambient glow FX (consecration)

            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0xC05CB08D }, 2728, "ps_3_0", "float4 main() : COLOR { return float4(0, 1, 1, 1); }" }, // Ground, structures base
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0xC05CB08D }, 1948, "ps_3_0", "float4 main() : COLOR { return float4(1, 0, 0, 1); }" }, // Unk (some terrain)
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0xC05CB08D }, 2284, "ps_3_0", "float4 main() : COLOR { return float4(1, 1, 0, 1); }" }, // Unk

            //{ { 0xFFFF0300, 0x0200001F, 0x8000000A, 0x900F0000 }, 200, "ps_3_0", "float4 main() : COLOR { return float4(1, 1, 0, 1); }" },  // Ambient glow FX (consecration)
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0xC05CB08D }, 2816, "ps_3_0", "float4 main() : COLOR { return float4(0, 0, 1, 1); }" }, // Ambient glow FX 1 (characters)
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x00000000 }, 2836, "ps_3_0", "float4 main() : COLOR { return float4(0, 1, 1, 1); }" }, // Ambient glow FX 2 (characters)

            { { 0xFFFF0300, 0x05000051, 0xA00F0000, 0x00000000 }, 1428, "ps_3_0" }, // Water

            //{ { 0xFFFF0300, 0x05000051, 0xA00F0001, 0x00000000 }, 88, "ps_3_0",   "float4 main() : COLOR { return float4(0, 1, 1, 1); }" }, // White
            //{ { 0xFFFF0300, 0x05000051, 0xA00F0001, 0x00000000 }, 152, "ps_3_0",  "float4 main() : COLOR { return float4(0, 1, 0, 1); }" }, // Gray
        };

        std::vector<VertexShaderInitCallback> g_vertexShaderCallbacks;
        std::vector<PixelShaderInitCallback> g_pixelShaderCallbacks;

        std::vector<ResourceCallback> g_onCreateCallbacks;
        std::vector<ResourceCallback> g_onDestroyCallbacks;
        std::vector<ResourceCallback> g_onReleaseCallbacks;
        std::vector<ResourceCallback> g_onRestoreCallbacks;

        std::vector<PresentCallback> g_presentCallbacks;
        std::vector<BeginSceneCallback> g_beginSceneCallbacks;
        std::vector<EndSceneCallback> g_endSceneCallbacks;
        std::vector<DrawPrimitiveCallback> g_drawPrimitiveCallbacks;
        std::vector<DrawIndexedPrimitiveCallback> g_drawIndexedPrimitiveCallbacks;
        std::vector<SetTextureCallback> g_setTextureCallbacks;
        std::vector<SetRenderStateCallback> g_setRenderStateCallbacks;
        std::vector<SetVertexShaderCallback> g_setVertexShaderCallbacks;
        std::vector<SetPixelShaderCallback> g_setPixelShaderCallbacks;
        std::vector<CreateTextureCallback> g_createTextureCallbacks;
        std::vector<SetRenderTargetCallback> g_setRenderTargetCallbacks;
        std::vector<ClearCallback> g_clearCallbacks;
        std::vector<ResetCallback> g_resetCallbacks;


        HRESULT STDMETHODCALLTYPE hkPresent(IDirect3DDevice9* device, const RECT* pSrcRect, const RECT* pDestRect, HWND hDestWnd, const RGNDATA* pDirtyRegion) {
            for (auto& cb : g_presentCallbacks) cb(device, pSrcRect, pDestRect, hDestWnd, pDirtyRegion);
            return oPresent(device, pSrcRect, pDestRect, hDestWnd, pDirtyRegion);
        }

        HRESULT STDMETHODCALLTYPE hkBeginScene(IDirect3DDevice9* device) {
            for (auto& cb : g_beginSceneCallbacks) cb(device);
            return oBeginScene(device);
        }

        HRESULT STDMETHODCALLTYPE hkEndScene(IDirect3DDevice9* device) {
            for (auto& cb : g_endSceneCallbacks) cb(device);
            return oEndScene(device);
        }

        HRESULT STDMETHODCALLTYPE hkDrawPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, UINT startVertex, UINT primCount) {
            for (auto& cb : g_drawPrimitiveCallbacks) cb(device, type, startVertex, primCount);
            return oDrawPrimitive(device, type, startVertex, primCount);
        }

        HRESULT STDMETHODCALLTYPE hkDrawIndexedPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, INT baseVertexIndex, UINT minVertexIndex, UINT numVertices, UINT startIndex, UINT primCount) {
            for (auto& cb : g_drawIndexedPrimitiveCallbacks) cb(device, type, baseVertexIndex, minVertexIndex, numVertices, startIndex, primCount);
            return oDrawIndexedPrimitive(device, type, baseVertexIndex, minVertexIndex, numVertices, startIndex, primCount);
        }

        HRESULT STDMETHODCALLTYPE hkSetTexture(IDirect3DDevice9* device, DWORD stage, IDirect3DBaseTexture9* pTexture) {
            for (auto& cb : g_setTextureCallbacks) cb(device, stage, pTexture);
            return oSetTexture(device, stage, pTexture);
        }

        HRESULT STDMETHODCALLTYPE hkSetRenderState(IDirect3DDevice9* device, D3DRENDERSTATETYPE state, DWORD value) {
            for (auto& cb : g_setRenderStateCallbacks) cb(device, state, value);
            return oSetRenderState(device, state, value);
        }

        HRESULT STDMETHODCALLTYPE hkSetVertexShader(IDirect3DDevice9* device, IDirect3DVertexShader9* pShader) {
            for (auto& cb : g_setVertexShaderCallbacks) cb(device, pShader);
            return oSetVertexShader(device, pShader);
        }

        HRESULT STDMETHODCALLTYPE hkSetPixelShader(IDirect3DDevice9* device, IDirect3DPixelShader9* pShader) {
            for (auto& cb : g_setPixelShaderCallbacks) cb(device, pShader);
            return oSetPixelShader(device, pShader);
        }

        HRESULT STDMETHODCALLTYPE hkCreateTexture(IDirect3DDevice9* device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
            for (auto& cb : g_createTextureCallbacks) cb(device, width, height, levels, usage, format, pool, ppTexture, pSharedHandle);
            return oCreateTexture(device, width, height, levels, usage, format, pool, ppTexture, pSharedHandle);
        }

        HRESULT STDMETHODCALLTYPE hkSetRenderTarget(IDirect3DDevice9* device, DWORD renderTargetIndex, IDirect3DSurface9* pRenderTarget) {
            for (auto& cb : g_setRenderTargetCallbacks) cb(device, renderTargetIndex, pRenderTarget);
            return oSetRenderTarget(device, renderTargetIndex, pRenderTarget);
        }

        HRESULT STDMETHODCALLTYPE hkClear(IDirect3DDevice9* device, DWORD count, const D3DRECT* pRects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) {
            for (auto& cb : g_clearCallbacks) cb(device, count, pRects, flags, color, z, stencil);
            return oClear(device, count, pRects, flags, color, z, stencil);
        }

        HRESULT STDMETHODCALLTYPE hkReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPP) {
            for (auto& cb : g_resetCallbacks) cb(device, pPP);
            return oReset(device, pPP);
        }

        typedef int(__thiscall* CGxDeviceD3d__DeviceSetFormat_t)(char*, const void*);
        CGxDeviceD3d__DeviceSetFormat_t CGxDeviceD3d__DeviceSetFormat_orig = reinterpret_cast<CGxDeviceD3d__DeviceSetFormat_t>(0x006904D0);

        typedef int(__thiscall* CGxDevice__DeviceCreate_t)(void*, IDirect3DDevice9*, int);
        CGxDevice__DeviceCreate_t CGxDevice__DeviceCreate_orig = reinterpret_cast<CGxDevice__DeviceCreate_t>(0x00682CB0);

        typedef int(__thiscall* CGxDeviceD3d__IDestroyD3d_t)(int*);
        CGxDeviceD3d__IDestroyD3d_t CGxDeviceD3d__IDestroyD3d_orig = reinterpret_cast<CGxDeviceD3d__IDestroyD3d_t>(0x006903B0);

        typedef int(__thiscall* CGxDeviceD3d__IReleaseD3dResources_t)(void*, int);
        CGxDeviceD3d__IReleaseD3dResources_t CGxDeviceD3d__IReleaseD3dResources_orig = reinterpret_cast<CGxDeviceD3d__IReleaseD3dResources_t>(0x00690150);

        typedef int(__thiscall* CGxDevice__NotifyOnDeviceRestored_t)(void*);
        CGxDevice__NotifyOnDeviceRestored_t CGxDevice__NotifyOnDeviceRestored_orig = reinterpret_cast<CGxDevice__NotifyOnDeviceRestored_t>(0x006843B0);

        typedef void(__thiscall* CGxDeviceD3d__IShaderCreateVertex_t)(int, ShaderData*);
        CGxDeviceD3d__IShaderCreateVertex_t CGxDeviceD3d__IShaderCreateVertex_orig = reinterpret_cast<CGxDeviceD3d__IShaderCreateVertex_t>(0x006AA0D0);

        typedef void(__thiscall* CGxDeviceD3d__IShaderCreatePixel_t)(int, ShaderData*);
        CGxDeviceD3d__IShaderCreatePixel_t CGxDeviceD3d__IShaderCreatePixel_orig = reinterpret_cast<CGxDeviceD3d__IShaderCreatePixel_t>(0x006AA070);


        int __fastcall CGxDevice__DeviceCreate_hk(void* pThis, void* edx, IDirect3DDevice9* device, int pCreateInfo) {
            const int result = CGxDevice__DeviceCreate_orig(pThis, device, pCreateInfo);
            if (result) {
                IDirect3DDevice9* device = GetDevice();
                if (device) {
                    __try {
                        IDirect3DDevice9Vtbl* vtbl = *reinterpret_cast<IDirect3DDevice9Vtbl**>(device);
                        if (vtbl) {
                            DetourTransactionBegin();

                            if (!g_presentCallbacks.empty()) {
                                oPresent = reinterpret_cast<Present_t>(vtbl->Present);
                                DetourAttach(&(PVOID&)oPresent, hkPresent);
                            }
                            if (!g_beginSceneCallbacks.empty()) {
                                oBeginScene = reinterpret_cast<BeginScene_t>(vtbl->BeginScene);
                                DetourAttach(&(PVOID&)oBeginScene, hkBeginScene);
                            }
                            if (!g_endSceneCallbacks.empty()) {
                                oEndScene = reinterpret_cast<EndScene_t>(vtbl->EndScene);
                                DetourAttach(&(PVOID&)oEndScene, hkEndScene);
                            }
                            if (!g_drawPrimitiveCallbacks.empty()) {
                                oDrawPrimitive = reinterpret_cast<DrawPrimitive_t>(vtbl->DrawPrimitive);
                                DetourAttach(&(PVOID&)oDrawPrimitive, hkDrawPrimitive);
                            }
                            if (!g_drawIndexedPrimitiveCallbacks.empty()) {
                                oDrawIndexedPrimitive = reinterpret_cast<DrawIndexedPrimitive_t>(vtbl->DrawIndexedPrimitive);
                                DetourAttach(&(PVOID&)oDrawIndexedPrimitive, hkDrawIndexedPrimitive);
                            }
                            if (!g_setTextureCallbacks.empty()) {
                                oSetTexture = reinterpret_cast<SetTexture_t>(vtbl->SetTexture);
                                DetourAttach(&(PVOID&)oSetTexture, hkSetTexture);
                            }
                            if (!g_setRenderStateCallbacks.empty()) {
                                oSetRenderState = reinterpret_cast<SetRenderState_t>(vtbl->SetRenderState);
                                DetourAttach(&(PVOID&)oSetRenderState, hkSetRenderState);
                            }
                            if (!g_setVertexShaderCallbacks.empty()) {
                                oSetVertexShader = reinterpret_cast<SetVertexShader_t>(vtbl->SetVertexShader);
                                DetourAttach(&(PVOID&)oSetVertexShader, hkSetVertexShader);
                            }
                            if (!g_setPixelShaderCallbacks.empty()) {
                                oSetPixelShader = reinterpret_cast<SetPixelShader_t>(vtbl->SetPixelShader);
                                DetourAttach(&(PVOID&)oSetPixelShader, hkSetPixelShader);
                            }
                            if (!g_createTextureCallbacks.empty()) {
                                oCreateTexture = reinterpret_cast<CreateTexture_t>(vtbl->CreateTexture);
                                DetourAttach(&(PVOID&)oCreateTexture, hkCreateTexture);
                            }
                            if (!g_setRenderTargetCallbacks.empty()) {
                                oSetRenderTarget = reinterpret_cast<SetRenderTarget_t>(vtbl->SetRenderTarget);
                                DetourAttach(&(PVOID&)oSetRenderTarget, hkSetRenderTarget);
                            }
                            if (!g_clearCallbacks.empty()) {
                                oClear = reinterpret_cast<Clear_t>(vtbl->Clear);
                                DetourAttach(&(PVOID&)oClear, hkClear);
                            }
                            if (!g_resetCallbacks.empty()) {
                                oReset = reinterpret_cast<Reset_t>(vtbl->Reset);
                                DetourAttach(&(PVOID&)oReset, hkReset);
                            }
                            DetourTransactionCommit();
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {}

                    for (auto& callback : g_onCreateCallbacks) callback();
                }
            }
            return result;
        }

        int __fastcall CGxDeviceD3d__IDestroyD3d_hk(int* pThis) {
            for (auto& cb : g_onDestroyCallbacks) cb();
            return CGxDeviceD3d__IDestroyD3d_orig(pThis);
        }

        int __fastcall CGxDeviceD3d__IReleaseD3dResources_hk(void* pThis, void* edx, int res) {
            CleanupManagedResources();
            for (auto& cb : g_onReleaseCallbacks) cb();
            return CGxDeviceD3d__IReleaseD3dResources_orig(pThis, res);
        }

        int __fastcall CGxDevice__NotifyOnDeviceRestored_hk(void* pThis) {
            RestoreManagedResources();
            for (auto& cb : g_onRestoreCallbacks) cb();
            return CGxDevice__NotifyOnDeviceRestored_orig(pThis);
        }

        int __fastcall CGxDeviceD3d__DeviceSetFormat_hk(char* lpParam, void* edx, const void* GxDeviceFormat) {
            const int result = CGxDeviceD3d__DeviceSetFormat_orig(lpParam, GxDeviceFormat);
            RestoreManagedResources();
            for (auto& cb : g_onRestoreCallbacks) cb();
            return result;
        }


        void __fastcall CGxDeviceD3d__IShaderCreateVertex_hk(int pThis, void* edx, ShaderData* shaderData) {
            CGxDeviceD3d__IShaderCreateVertex_orig(pThis, shaderData);
            for (auto& cb : g_vertexShaderCallbacks) cb(shaderData);
        }

        void __fastcall CGxDeviceD3d__IShaderCreatePixel_hk(int pThis, void* edx, ShaderData* shaderData) {
            CGxDeviceD3d__IShaderCreatePixel_orig(pThis, shaderData);
            for (auto& cb : g_pixelShaderCallbacks) cb(shaderData);
        }
    }

    std::span<const ShaderEntry> s_shaders{ s_shaders_engine };
    const size_t s_shaders_count = sizeof(s_shaders) / sizeof(ShaderEntry);


    void RegisterPresentCallback(PresentCallback callback) {
        if (callback) g_presentCallbacks.push_back(callback);
    }

    void RegisterBeginSceneCallback(BeginSceneCallback callback) {
        if (callback) g_beginSceneCallbacks.push_back(callback);
    }

    void RegisterEndSceneCallback(EndSceneCallback callback) {
        if (callback) g_endSceneCallbacks.push_back(callback);
    }

    void RegisterDrawPrimitiveCallback(DrawPrimitiveCallback callback) {
        if (callback) g_drawPrimitiveCallbacks.push_back(callback);
    }

    void RegisterDrawIndexedPrimitiveCallback(DrawIndexedPrimitiveCallback callback) {
        if (callback) g_drawIndexedPrimitiveCallbacks.push_back(callback);
    }

    void RegisterSetTextureCallback(SetTextureCallback callback) {
        if (callback) g_setTextureCallbacks.push_back(callback);
    }

    void RegisterSetRenderStateCallback(SetRenderStateCallback callback) {
        if (callback) g_setRenderStateCallbacks.push_back(callback);
    }

    void RegisterSetVertexShaderCallback(SetVertexShaderCallback callback) {
        if (callback) g_setVertexShaderCallbacks.push_back(callback);
    }

    void RegisterSetPixelShaderCallback(SetPixelShaderCallback callback) {
        if (callback) g_setPixelShaderCallbacks.push_back(callback);
    }

    void RegisterCreateTextureCallback(CreateTextureCallback callback) {
        if (callback) g_createTextureCallbacks.push_back(callback);
    }

    void RegisterSetRenderTargetCallback(SetRenderTargetCallback callback) {
        if (callback) g_setRenderTargetCallbacks.push_back(callback);
    }

    void RegisterClearCallback(ClearCallback callback) {
        if (callback) g_clearCallbacks.push_back(callback);
    }

    void RegisterResetCallback(ResetCallback callback) {
        if (callback) g_resetCallbacks.push_back(callback);
    }


    void RegisterOnCreate(ResourceCallback callback) {
        if (callback) g_onCreateCallbacks.push_back(callback);
    }

    void RegisterOnDestroy(ResourceCallback callback) {
        if (callback) g_onDestroyCallbacks.push_back(callback);
    }

    void RegisterOnRelease(ResourceCallback callback) {
        if (callback) g_onReleaseCallbacks.push_back(callback);
    }

    void RegisterOnRestore(ResourceCallback callback) {
        if (callback) g_onRestoreCallbacks.push_back(callback);
    }


    void RegisterVertexShaderInit(VertexShaderInitCallback callback) {
        if (callback) g_vertexShaderCallbacks.push_back(callback);
    }

    void RegisterPixelShaderInit(PixelShaderInitCallback callback) {
        if (callback) g_pixelShaderCallbacks.push_back(callback);
    }


    IDirect3DDevice9* GetDevice() {
        __try {
            const uintptr_t pDevicePtr = *reinterpret_cast<uintptr_t*>(0x00C5DF88);
            if (!pDevicePtr) return nullptr;
            IDirect3DDevice9* device = *reinterpret_cast<IDirect3DDevice9**>(pDevicePtr + 0x397C);
            if (device && device->TestCooperativeLevel() == D3D_OK) {
                return device;
            }
            return nullptr;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    IDirect3DVertexShader9* CompileVertexShader(D3D::ResourceParams p) {
        if (p.shaderCode.empty()) return nullptr;
        ID3DBlob* pCode = nullptr, * pError = nullptr;
        HRESULT hr = D3DCompile(p.shaderCode.c_str(), p.shaderCode.length(), nullptr, nullptr,
            nullptr, p.entryPoint.c_str(), p.target.c_str(), 0, 0, &pCode, &pError);
        if (FAILED(hr)) { LogShaderError(pError, 1); return nullptr; }
        IDirect3DVertexShader9* shader = nullptr;
        IDirect3DDevice9* device = GetDevice();
        if (!device) { pCode->Release(); return nullptr; }
        hr = device->CreateVertexShader(reinterpret_cast<const DWORD*>(pCode->GetBufferPointer()), &shader);
        pCode->Release();
        if (SUCCEEDED(hr)) {
            if (p.autoCleanup && p.ppResourceAddress) {
                *p.ppResourceAddress = reinterpret_cast<IUnknown*>(shader);
                RegisterForCleanup(reinterpret_cast<IUnknown**>(p.ppResourceAddress), ResourceType::ShaderVertex, p);
            }
            return shader;
        }
        return nullptr;
    }

    IDirect3DPixelShader9* CompilePixelShader(D3D::ResourceParams p) {
        if (p.shaderCode.empty()) return nullptr;
        ID3DBlob* pCode = nullptr, * pError = nullptr;
        HRESULT hr = D3DCompile(p.shaderCode.c_str(), p.shaderCode.length(), nullptr, nullptr,
            nullptr, p.entryPoint.c_str(), p.target.c_str(), 0, 0, &pCode, &pError);
        if (FAILED(hr)) { LogShaderError(pError, 0); return nullptr; }
        IDirect3DPixelShader9* shader = nullptr;
        IDirect3DDevice9* device = GetDevice();
        if (!device) { pCode->Release(); return nullptr; }
        hr = device->CreatePixelShader(reinterpret_cast<const DWORD*>(pCode->GetBufferPointer()), &shader);
        pCode->Release();
        if (SUCCEEDED(hr)) {
            if (p.autoCleanup && p.ppResourceAddress) {
                *p.ppResourceAddress = reinterpret_cast<IUnknown*>(shader);
                RegisterForCleanup(reinterpret_cast<IUnknown**>(p.ppResourceAddress), ResourceType::ShaderPixel, p);
            }
            return shader;
        }
        return nullptr;
    }

    bool CreateTexture(IDirect3DTexture9** ppTexture, D3D::ResourceParams p) {
        if (!ppTexture) return false;
        *ppTexture = nullptr; if (p.ppSurface) *p.ppSurface = nullptr;
        IDirect3DDevice9* device = GetDevice();
        if (!device) return false;
        if (p.width == 0 || p.height == 0) {
            D3DVIEWPORT9 vp;
            if (FAILED(device->GetViewport(&vp))) return false;
            if (p.width == 0) p.width = vp.Width;
            if (p.height == 0) p.height = vp.Height;
        }
        if (FAILED(device->CreateTexture(p.width, p.height, p.levels, p.usage, p.format, p.pool, ppTexture, p.pSharedHandle))) return false;
        if (p.clearToZero && p.pool != D3DPOOL_DEFAULT) {
            D3DLOCKED_RECT lr;
            if (FAILED((*ppTexture)->LockRect(0, &lr, nullptr, 0))) return false;
            std::memset(lr.pBits, 0, p.height * lr.Pitch);
            (*ppTexture)->UnlockRect(0);
        }
        if (p.autoCleanup) RegisterForCleanup(reinterpret_cast<IUnknown**>(ppTexture), ResourceType::Texture, p);
        if (p.ppSurface) (*ppTexture)->GetSurfaceLevel(p.surfLevel, p.ppSurface);
        return true;
    }

    bool CreateRenderTarget(IDirect3DSurface9** ppSurface, D3D::ResourceParams p) {
        if (!ppSurface) return false;
        IDirect3DDevice9* device = GetDevice();
        if (!device) return false;
        if (p.width == 0 || p.height == 0) {
            D3DVIEWPORT9 vp;
            device->GetViewport(&vp);
            if (p.width == 0) p.width = vp.Width;
            if (p.height == 0) p.height = vp.Height;
        }
        if (SUCCEEDED(device->CreateRenderTarget(p.width, p.height, p.format, p.multisample, p.quality, p.lockable, ppSurface, p.pSharedHandle))) {
            if (p.autoCleanup)  RegisterForCleanup(reinterpret_cast<IUnknown**>(ppSurface), ResourceType::RenderTarget, p);
            return true;
        }
        return false;
    }
}

void D3D::initialize() {
    DetourAttach(&(PVOID&)CGxDevice__DeviceCreate_orig, CGxDevice__DeviceCreate_hk);
    DetourAttach(&(PVOID&)CGxDeviceD3d__DeviceSetFormat_orig, CGxDeviceD3d__DeviceSetFormat_hk);
    DetourAttach(&(PVOID&)CGxDeviceD3d__IDestroyD3d_orig, CGxDeviceD3d__IDestroyD3d_hk);
    DetourAttach(&(PVOID&)CGxDeviceD3d__IReleaseD3dResources_orig, CGxDeviceD3d__IReleaseD3dResources_hk);
    DetourAttach(&(PVOID&)CGxDevice__NotifyOnDeviceRestored_orig, CGxDevice__NotifyOnDeviceRestored_hk);

    DetourAttach(&(PVOID&)CGxDeviceD3d__IShaderCreateVertex_orig, CGxDeviceD3d__IShaderCreateVertex_hk);
    DetourAttach(&(PVOID&)CGxDeviceD3d__IShaderCreatePixel_orig, CGxDeviceD3d__IShaderCreatePixel_hk);
}