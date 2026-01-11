#include "Hooks.h"
#include "D3D.h"
#include <Detours/detours.h>
#include <d3dcompiler.h>
#include <algorithm>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace D3D {
    namespace {
        std::vector<VertexShaderInitCallback> g_vertexShaderCallbacks;
        std::vector<PixelShaderInitCallback> g_pixelShaderCallbacks;

        std::vector<IDirect3DVertexShader9*> g_cachedVertexShaders;
        std::vector<IDirect3DPixelShader9*> g_cachedPixelShaders;

        std::vector<ResourceCallback> g_onLostCallbacks;
        std::vector<ResourceCallback> g_onResetCallbacks;
        std::vector<ResourceCallback> g_onDestroyCallbacks;

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


        HRESULT STDMETHODCALLTYPE hkPresent(IDirect3DDevice9* device, const RECT* pSrcRect, const RECT* pDestRect, HWND hDestWnd, const RGNDATA* pDirtyRegion) {
            for (auto& cb : g_presentCallbacks) {
                cb(device, pSrcRect, pDestRect, hDestWnd, pDirtyRegion);
            }
            return oPresent(device, pSrcRect, pDestRect, hDestWnd, pDirtyRegion);
        }

        HRESULT STDMETHODCALLTYPE hkBeginScene(IDirect3DDevice9* device) {
            for (auto& cb : g_beginSceneCallbacks) {
                cb(device);
            }
            return oBeginScene(device);
        }

        HRESULT STDMETHODCALLTYPE hkEndScene(IDirect3DDevice9* device) {
            for (auto& cb : g_endSceneCallbacks) {
                cb(device);
            }
            return oEndScene(device);
        }

        HRESULT STDMETHODCALLTYPE hkDrawPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, UINT startVertex, UINT primCount) {
            for (auto& cb : g_drawPrimitiveCallbacks) {
                cb(device, type, startVertex, primCount);
            }
            return oDrawPrimitive(device, type, startVertex, primCount);
        }

        HRESULT STDMETHODCALLTYPE hkDrawIndexedPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, INT baseVertexIndex, UINT minVertexIndex, UINT numVertices, UINT startIndex, UINT primCount) {
            for (auto& cb : g_drawIndexedPrimitiveCallbacks) {
                cb(device, type, baseVertexIndex, minVertexIndex, numVertices, startIndex, primCount);
            }
            return oDrawIndexedPrimitive(device, type, baseVertexIndex, minVertexIndex, numVertices, startIndex, primCount);
        }

        HRESULT STDMETHODCALLTYPE hkSetTexture(IDirect3DDevice9* device, DWORD stage, IDirect3DBaseTexture9* pTexture) {
            for (auto& cb : g_setTextureCallbacks) {
                cb(device, stage, pTexture);
            }
            return oSetTexture(device, stage, pTexture);
        }

        HRESULT STDMETHODCALLTYPE hkSetRenderState(IDirect3DDevice9* device, D3DRENDERSTATETYPE state, DWORD value) {
            for (auto& cb : g_setRenderStateCallbacks) {
                cb(device, state, value);
            }
            return oSetRenderState(device, state, value);
        }

        HRESULT STDMETHODCALLTYPE hkSetVertexShader(IDirect3DDevice9* device, IDirect3DVertexShader9* pShader) {
            for (auto& cb : g_setVertexShaderCallbacks) {
                cb(device, pShader);
            }
            return oSetVertexShader(device, pShader);
        }

        HRESULT STDMETHODCALLTYPE hkSetPixelShader(IDirect3DDevice9* device, IDirect3DPixelShader9* pShader) {
            for (auto& cb : g_setPixelShaderCallbacks) {
                cb(device, pShader);
            }
            return oSetPixelShader(device, pShader);
        }

        HRESULT STDMETHODCALLTYPE hkCreateTexture(IDirect3DDevice9* device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
            for (auto& cb : g_createTextureCallbacks) {
                cb(device, width, height, levels, usage, format, pool, ppTexture, pSharedHandle);
            }
            return oCreateTexture(device, width, height, levels, usage, format, pool, ppTexture, pSharedHandle);
        }

        HRESULT STDMETHODCALLTYPE hkSetRenderTarget(IDirect3DDevice9* device, DWORD renderTargetIndex, IDirect3DSurface9* pRenderTarget) {
            for (auto& cb : g_setRenderTargetCallbacks) {
                cb(device, renderTargetIndex, pRenderTarget);
            }
            return oSetRenderTarget(device, renderTargetIndex, pRenderTarget);
        }

        HRESULT STDMETHODCALLTYPE hkClear(IDirect3DDevice9* device, DWORD count, const D3DRECT* pRects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) {
            for (auto& cb : g_clearCallbacks) {
                cb(device, count, pRects, flags, color, z, stencil);
            }
            return oClear(device, count, pRects, flags, color, z, stencil);
        }

        HRESULT STDMETHODCALLTYPE hkReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPP) {
            for (auto& cb : g_resetCallbacks) {
                cb(device, pPP);
            }
            return oReset(device, pPP);
        }


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


        typedef int(__thiscall* CGxDeviceD3d__ICreateD3d_t)(int);
        CGxDeviceD3d__ICreateD3d_t CGxDeviceD3d__ICreateD3d_orig = reinterpret_cast<CGxDeviceD3d__ICreateD3d_t>(0x00690680);

        typedef int(__thiscall* CGxDeviceD3d__IDestroyD3D_t)(int*);
        CGxDeviceD3d__IDestroyD3D_t CGxDeviceD3d__IDestroyD3d_orig = reinterpret_cast<CGxDeviceD3d__IDestroyD3D_t>(0x006903B0);

        typedef void(__thiscall* CGxDeviceD3d__IShaderCreateVertex_t)(int, ShaderData*);
        CGxDeviceD3d__IShaderCreateVertex_t CGxDeviceD3d__IShaderCreateVertex_orig = reinterpret_cast<CGxDeviceD3d__IShaderCreateVertex_t>(0x006AA0D0);

        typedef void(__thiscall* CGxDeviceD3d__IShaderCreatePixel_t)(int, ShaderData*);
        CGxDeviceD3d__IShaderCreatePixel_t CGxDeviceD3d__IShaderCreatePixel_orig = reinterpret_cast<CGxDeviceD3d__IShaderCreatePixel_t>(0x006AA070);

        struct IDirect3DDevice9Vtbl {
            // IUnknown methods
            void* QueryInterface;              // 0
            void* AddRef;                      // 1
            void* Release;                     // 2

            // IDirect3DDevice9 methods
            void* TestCooperativeLevel;        // 3
            void* GetAvailableTextureMem;      // 4
            void* EvictManagedResources;       // 5
            void* GetDirect3D;                 // 6
            void* GetDeviceCaps;               // 7
            void* GetDisplayMode;              // 8
            void* GetCreationParameters;       // 9
            void* SetCursorProperties;         // 10
            void* SetCursorPosition;           // 11
            void* ShowCursor;                  // 12
            void* CreateAdditionalSwapChain;   // 13
            void* GetSwapChain;                // 14
            void* GetNumberOfSwapChains;       // 15
            void* Reset;                       // 16
            void* Present;                     // 17
            void* GetBackBuffer;               // 18
            void* GetRasterStatus;             // 19
            void* SetDialogBoxMode;            // 20
            void* SetGammaRamp;                // 21
            void* GetGammaRamp;                // 22
            void* CreateTexture;               // 23
            void* CreateVolumeTexture;         // 24
            void* CreateCubeTexture;           // 25
            void* CreateVertexBuffer;          // 26
            void* CreateIndexBuffer;           // 27
            void* CreateRenderTarget;          // 28
            void* CreateDepthStencilSurface;   // 29
            void* UpdateSurface;               // 30
            void* UpdateTexture;               // 31
            void* GetRenderTargetData;         // 32
            void* GetFrontBufferData;          // 33
            void* StretchRect;                 // 34
            void* ColorFill;                   // 35
            void* CreateOffscreenPlainSurface; // 36
            void* SetRenderTarget;             // 37
            void* GetRenderTarget;             // 38
            void* SetDepthStencilSurface;      // 39
            void* GetDepthStencilSurface;      // 40
            void* BeginScene;                  // 41
            void* EndScene;                    // 42
            void* Clear;                       // 43
            void* SetTransform;                // 44
            void* GetTransform;                // 45
            void* MultiplyTransform;           // 46
            void* SetViewport;                 // 47
            void* GetViewport;                 // 48
            void* SetMaterial;                 // 49
            void* GetMaterial;                 // 50
            void* SetLight;                    // 51
            void* GetLight;                    // 52
            void* LightEnable;                 // 53
            void* GetLightEnable;              // 54
            void* SetClipPlane;                // 55
            void* GetClipPlane;                // 56
            void* SetRenderState;              // 57
            void* GetRenderState;              // 58
            void* CreateStateBlock;            // 59
            void* BeginStateBlock;             // 60
            void* EndStateBlock;               // 61
            void* SetClipStatus;               // 62
            void* GetClipStatus;               // 63
            void* GetTexture;                  // 64
            void* SetTexture;                  // 65
            void* GetTextureStageState;        // 66
            void* SetTextureStageState;        // 67
            void* GetSamplerState;             // 68
            void* SetSamplerState;             // 69
            void* ValidateDevice;              // 70
            void* SetPaletteEntries;           // 71
            void* GetPaletteEntries;           // 72
            void* SetCurrentTexturePalette;    // 73
            void* GetCurrentTexturePalette;    // 74
            void* SetScissorRect;              // 75
            void* GetScissorRect;              // 76
            void* SetSoftwareVertexProcessing; // 77
            void* GetSoftwareVertexProcessing; // 78
            void* SetNPatchMode;               // 79
            void* GetNPatchMode;               // 80
            void* DrawPrimitive;               // 81
            void* DrawIndexedPrimitive;        // 82
            void* DrawPrimitiveUP;             // 83
            void* DrawIndexedPrimitiveUP;      // 84
            void* ProcessVertices;             // 85
            void* CreateVertexDeclaration;     // 86
            void* SetVertexDeclaration;        // 87
            void* GetVertexDeclaration;        // 88
            void* SetFVF;                      // 89
            void* GetFVF;                      // 90
            void* CreateVertexShader;          // 91
            void* SetVertexShader;             // 92
            void* GetVertexShader;             // 93
            void* SetVertexShaderConstantF;    // 94
            void* GetVertexShaderConstantF;    // 95
            void* SetVertexShaderConstantI;    // 96
            void* GetVertexShaderConstantI;    // 97
            void* SetVertexShaderConstantB;    // 98
            void* GetVertexShaderConstantB;    // 99
            void* SetStreamSource;             // 100
            void* GetStreamSource;             // 101
            void* SetStreamSourceFreq;         // 102
            void* GetStreamSourceFreq;         // 103
            void* SetIndices;                  // 104
            void* GetIndices;                  // 105
            void* CreatePixelShader;           // 106
            void* SetPixelShader;              // 107
            void* GetPixelShader;              // 108
            void* SetPixelShaderConstantF;     // 109
            void* GetPixelShaderConstantF;     // 110
            void* SetPixelShaderConstantI;     // 111
            void* GetPixelShaderConstantI;     // 112
            void* SetPixelShaderConstantB;     // 113
            void* GetPixelShaderConstantB;     // 114
            void* DrawRectPatch;               // 115
            void* DrawTriPatch;                // 116
            void* DeletePatch;                 // 117
            void* CreateQuery;                 // 118
        };

        int __fastcall CGxDeviceD3d__ICreateD3d_hk(int pThis) {
            const int result = CGxDeviceD3d__ICreateD3d_orig(pThis);
            if (result) {
                IDirect3DDevice9* device = GetDevice();
                if (device) {
                    __try {
                        IDirect3DDevice9Vtbl** vtbl = reinterpret_cast<IDirect3DDevice9Vtbl**>(device);
                        if (vtbl && *vtbl) {
                            DetourTransactionBegin();
                            DetourUpdateThread(GetCurrentThread());

                            if (!g_presentCallbacks.empty()) {
                                oPresent = reinterpret_cast<Present_t>((*vtbl)->Present);
                                DetourAttach(&(PVOID&)oPresent, hkPresent);
                            }
                            if (!g_beginSceneCallbacks.empty()) {
                                oBeginScene = reinterpret_cast<BeginScene_t>((*vtbl)->BeginScene);
                                DetourAttach(&(PVOID&)oBeginScene, hkBeginScene);
                            }
                            if (!g_endSceneCallbacks.empty()) {
                                oEndScene = reinterpret_cast<EndScene_t>((*vtbl)->EndScene);
                                DetourAttach(&(PVOID&)oEndScene, hkEndScene);
                            }
                            if (!g_drawPrimitiveCallbacks.empty()) {
                                oDrawPrimitive = reinterpret_cast<DrawPrimitive_t>((*vtbl)->DrawPrimitive);
                                DetourAttach(&(PVOID&)oDrawPrimitive, hkDrawPrimitive);
                            }
                            if (!g_drawIndexedPrimitiveCallbacks.empty()) {
                                oDrawIndexedPrimitive = reinterpret_cast<DrawIndexedPrimitive_t>((*vtbl)->DrawIndexedPrimitive);
                                DetourAttach(&(PVOID&)oDrawIndexedPrimitive, hkDrawIndexedPrimitive);
                            }
                            if (!g_setTextureCallbacks.empty()) {
                                oSetTexture = reinterpret_cast<SetTexture_t>((*vtbl)->SetTexture);
                                DetourAttach(&(PVOID&)oSetTexture, hkSetTexture);
                            }
                            if (!g_setRenderStateCallbacks.empty()) {
                                oSetRenderState = reinterpret_cast<SetRenderState_t>((*vtbl)->SetRenderState);
                                DetourAttach(&(PVOID&)oSetRenderState, hkSetRenderState);
                            }
                            if (!g_setVertexShaderCallbacks.empty()) {
                                oSetVertexShader = reinterpret_cast<SetVertexShader_t>((*vtbl)->SetVertexShader);
                                DetourAttach(&(PVOID&)oSetVertexShader, hkSetVertexShader);
                            }
                            if (!g_setPixelShaderCallbacks.empty()) {
                                oSetPixelShader = reinterpret_cast<SetPixelShader_t>((*vtbl)->SetPixelShader);
                                DetourAttach(&(PVOID&)oSetPixelShader, hkSetPixelShader);
                            }
                            if (!g_createTextureCallbacks.empty()) {
                                oCreateTexture = reinterpret_cast<CreateTexture_t>((*vtbl)->CreateTexture);
                                DetourAttach(&(PVOID&)oCreateTexture, hkCreateTexture);
                            }
                            if (!g_setRenderTargetCallbacks.empty()) {
                                oSetRenderTarget = reinterpret_cast<SetRenderTarget_t>((*vtbl)->SetRenderTarget);
                                DetourAttach(&(PVOID&)oSetRenderTarget, hkSetRenderTarget);
                            }
                            if (!g_clearCallbacks.empty()) {
                                oClear = reinterpret_cast<Clear_t>((*vtbl)->Clear);
                                DetourAttach(&(PVOID&)oClear, hkClear);
                            }
                            if (!g_presentCallbacks.empty()) {
                                oReset = reinterpret_cast<Reset_t>((*vtbl)->Reset);
                                DetourAttach(&(PVOID&)oReset, hkReset);
                            }
                            DetourTransactionCommit();
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
            }
            return result;
        }

        int __fastcall CGxDeviceD3d__IDestroyD3d_hk(int* pThis) {
            for (auto& callback : g_onDestroyCallbacks) {
                callback();
            }
            return  CGxDeviceD3d__IDestroyD3d_orig(pThis);
        }

        void __fastcall CGxDeviceD3d__IShaderCreateVertex_hk(int pThis, void* edx, ShaderData* shaderData) {
            CGxDeviceD3d__IShaderCreateVertex_orig(pThis, shaderData);
            for (auto& callback : g_vertexShaderCallbacks) {
                IDirect3DVertexShader9* shader = callback(shaderData);
                if (shader) {
                    if (std::find(g_cachedVertexShaders.begin(), g_cachedVertexShaders.end(), shader) == g_cachedVertexShaders.end()) {
                        g_cachedVertexShaders.push_back(shader);
                    }
                }
            }
        }

        void __fastcall CGxDeviceD3d__IShaderCreatePixel_hk(int pThis, void* edx, ShaderData* shaderData) {
            CGxDeviceD3d__IShaderCreatePixel_orig(pThis, shaderData);
            for (auto& callback : g_pixelShaderCallbacks) {
                IDirect3DPixelShader9* shader = callback(shaderData);
                if (shader) {
                    if (std::find(g_cachedPixelShaders.begin(), g_cachedPixelShaders.end(), shader) == g_cachedPixelShaders.end()) {
                        g_cachedPixelShaders.push_back(shader);
                    }
                }
            }
        }
    }


    void RegisterOnDestroy(ResourceCallback callback) {
        if (callback) {
            g_onDestroyCallbacks.push_back(callback);
        }
    }

    void RegisterVertexShaderInit(VertexShaderInitCallback callback) {
        if (callback) {
            g_vertexShaderCallbacks.push_back(callback);
        }
    }

    void RegisterPixelShaderInit(PixelShaderInitCallback callback) {
        if (callback) {
            g_pixelShaderCallbacks.push_back(callback);
        }
    }


    IDirect3DDevice9* GetDevice() {
        __try {
            const uintptr_t pDevicePtr = *reinterpret_cast<uintptr_t*>(0x00C5DF88);
            if (!pDevicePtr) return nullptr;
            return *reinterpret_cast<IDirect3DDevice9**>(pDevicePtr + 0x397C);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    IDirect3DVertexShader9* CompileVertexShader(const char* code, const char* entryPoint, const char* target) {
        if (!code) return nullptr;

        ID3DBlob* pCode = nullptr;
        ID3DBlob* pError = nullptr;

        HRESULT hr = D3DCompile(code, strlen(code), nullptr, nullptr, nullptr,
            entryPoint, target, 0, 0, &pCode, &pError);

        if (FAILED(hr)) {
            if (pError) pError->Release();
            return nullptr;
        }

        IDirect3DDevice9* device = GetDevice();
        if (!device) {
            pCode->Release();
            return nullptr;
        }

        IDirect3DVertexShader9* shader = nullptr;
        hr = device->CreateVertexShader(
            reinterpret_cast<const DWORD*>(pCode->GetBufferPointer()), &shader);
        pCode->Release();

        return SUCCEEDED(hr) ? shader : nullptr;
    }

    IDirect3DPixelShader9* CompilePixelShader(const char* code, const char* entryPoint, const char* target) {
        if (!code) return nullptr;

        ID3DBlob* pCode = nullptr;
        ID3DBlob* pError = nullptr;

        HRESULT hr = D3DCompile(code, strlen(code), nullptr, nullptr, nullptr,
            entryPoint, target, 0, 0, &pCode, &pError);

        if (FAILED(hr)) {
            if (pError) pError->Release();
            return nullptr;
        }

        IDirect3DDevice9* device = GetDevice();
        if (!device) {
            pCode->Release();
            return nullptr;
        }

        IDirect3DPixelShader9* shader = nullptr;
        hr = device->CreatePixelShader(
            reinterpret_cast<const DWORD*>(pCode->GetBufferPointer()), &shader);
        pCode->Release();

        return SUCCEEDED(hr) ? shader : nullptr;
    }

    bool CreateTexture(IDirect3DTexture9** ppTexture, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, bool clearToZero) {
        if (!ppTexture) return false;

        IDirect3DDevice9* device = GetDevice();
        if (!device) return false;

        const HRESULT hr = device->CreateTexture(
            width, height, levels, usage,
            format, pool,
            ppTexture, nullptr
        );
        if (FAILED(hr)) return false;

        if (clearToZero) {
            D3DLOCKED_RECT lockedRect{};
            if (SUCCEEDED((*ppTexture)->LockRect(0, &lockedRect, nullptr, 0))) {
                std::memset(lockedRect.pBits, 0, height * lockedRect.Pitch);
                (*ppTexture)->UnlockRect(0);
            }
        }
        return true;
    }
}

void D3D::initialize() {
    DetourAttach(&(PVOID&)CGxDeviceD3d__ICreateD3d_orig, CGxDeviceD3d__ICreateD3d_hk);
    DetourAttach(&(PVOID&)CGxDeviceD3d__IDestroyD3d_orig, CGxDeviceD3d__IDestroyD3d_hk);

    DetourAttach(&(PVOID&)CGxDeviceD3d__IShaderCreateVertex_orig, CGxDeviceD3d__IShaderCreateVertex_hk);
    DetourAttach(&(PVOID&)CGxDeviceD3d__IShaderCreatePixel_orig, CGxDeviceD3d__IShaderCreatePixel_hk);
}