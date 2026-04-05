#pragma once
#include "GameClient.h"
#include <d3d9.h>
#include <functional>
#include <string>
#include <span>

namespace D3D {
    void initialize();

    IDirect3DDevice9* GetDevice();

    using ResourceCallback = std::function<void()>;
    using PresentCallback = std::function<void(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)>;
    using BeginSceneCallback = std::function<void(IDirect3DDevice9*)>;
    using EndSceneCallback = std::function<void(IDirect3DDevice9*)>;
    using DrawPrimitiveCallback = std::function<void(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT)>;
    using DrawIndexedPrimitiveCallback = std::function<void(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT)>;
    using SetTextureCallback = std::function<void(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*)>;
    using SetRenderStateCallback = std::function<void(IDirect3DDevice9*, D3DRENDERSTATETYPE, DWORD)>;
    using SetVertexShaderCallback = std::function<void(IDirect3DDevice9*, IDirect3DVertexShader9*)>;
    using SetPixelShaderCallback = std::function<void(IDirect3DDevice9*, IDirect3DPixelShader9*)>;
    using CreateTextureCallback = std::function<void(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*)>;
    using SetRenderTargetCallback = std::function<void(IDirect3DDevice9*, DWORD, IDirect3DSurface9*)>;
    using ClearCallback = std::function<void(IDirect3DDevice9*, DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD)>;
    using ResetCallback = std::function<void(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)>;

    using Present_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    using BeginScene_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*);
    using EndScene_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*);
    using DrawPrimitive_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
    using DrawIndexedPrimitive_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
    using SetTexture_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);
    using SetRenderState_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DRENDERSTATETYPE, DWORD);
    using SetVertexShader_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, IDirect3DVertexShader9*);
    using SetPixelShader_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, IDirect3DPixelShader9*);
    using CreateTexture_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*);
    using SetRenderTarget_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, DWORD, IDirect3DSurface9*);
    using Clear_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD);
    using Reset_t = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

    extern Present_t oPresent;
    extern BeginScene_t oBeginScene;
    extern EndScene_t oEndScene;
    extern DrawPrimitive_t oDrawPrimitive;
    extern DrawIndexedPrimitive_t oDrawIndexedPrimitive;
    extern SetTexture_t oSetTexture;
    extern SetRenderState_t oSetRenderState;
    extern SetVertexShader_t oSetVertexShader;
    extern SetPixelShader_t oSetPixelShader;
    extern CreateTexture_t oCreateTexture;
    extern SetRenderTarget_t oSetRenderTarget;
    extern Clear_t oClear;
    extern Reset_t oReset;

    void RegisterPresentCallback(const PresentCallback& callback);
    void RegisterBeginSceneCallback(const BeginSceneCallback& callback);
    void RegisterEndSceneCallback(const EndSceneCallback& callback);
    void RegisterDrawPrimitiveCallback(const DrawPrimitiveCallback& callback);
    void RegisterDrawIndexedPrimitiveCallback(const DrawIndexedPrimitiveCallback& callback);
    void RegisterSetTextureCallback(const SetTextureCallback& callback);
    void RegisterSetRenderStateCallback(const SetRenderStateCallback& callback);
    void RegisterSetVertexShaderCallback(const SetVertexShaderCallback& callback);
    void RegisterSetPixelShaderCallback(const SetPixelShaderCallback& callback);
    void RegisterCreateTextureCallback(const CreateTextureCallback& callback);
    void RegisterSetRenderTargetCallback(const SetRenderTargetCallback& callback);
    void RegisterClearCallback(const ClearCallback& callback);
    void RegisterResetCallback(const ResetCallback& callback);

    void RegisterOnCreate(const ResourceCallback& callback);
    void RegisterOnDestroy(const ResourceCallback& callback);
    void RegisterOnRelease(const ResourceCallback& callback);
    void RegisterOnRestore(const ResourceCallback& callback);


    struct ResourceParams {
        UINT width = 0, height = 0, levels = 1, surfLevel = 0;
        DWORD usage = 0, quality = 0;
        D3DFORMAT format = D3DFMT_A8R8G8B8;
        D3DPOOL pool = D3DPOOL_DEFAULT;
        D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE;
        bool lockable = false;
        HANDLE* pSharedHandle = nullptr;

        std::string_view shaderCode;
        std::string entryPoint = "main";
        std::string target;

        IUnknown** ppResourceAddress = nullptr;
        IDirect3DSurface9** ppSurface = nullptr;

        bool clearToZero = false;
        bool autoCleanup = false;
    };

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

    struct ShaderEntry {
        uint32_t data[4];
        uint32_t length;
        const char* profile;
    };

    extern std::span<const ShaderEntry> s_shaders;
    extern const size_t s_shaders_count;

    using VertexShaderInitCallback = std::function<void(CGxDevice::ShaderData*)>;
    using PixelShaderInitCallback = std::function<void(CGxDevice::ShaderData*)>;

    void RegisterVertexShaderInit(const VertexShaderInitCallback& callback);
    void RegisterPixelShaderInit(const PixelShaderInitCallback& callback);

    IDirect3DVertexShader9* CompileVertexShader(const ResourceParams& p);
    IDirect3DPixelShader9* CompilePixelShader(const ResourceParams& p);

    bool CreateTexture(IDirect3DTexture9** ppTexture, ResourceParams p);
    bool CreateRenderTarget(IDirect3DSurface9** ppSurface, ResourceParams p);
}