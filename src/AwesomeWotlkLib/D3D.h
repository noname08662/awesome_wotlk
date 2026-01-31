#pragma once
#include <d3d9.h>
#include <functional>
#include <string>
#include <span>

namespace D3D {
    void initialize();

    IDirect3DDevice9* GetDevice();

    using ResourceCallback = std::function<void()>;

    typedef std::function<void(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> PresentCallback;
    typedef std::function<void(IDirect3DDevice9*)> BeginSceneCallback;
    typedef std::function<void(IDirect3DDevice9*)> EndSceneCallback;
    typedef std::function<void(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT)> DrawPrimitiveCallback;
    typedef std::function<void(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT)> DrawIndexedPrimitiveCallback;
    typedef std::function<void(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*)> SetTextureCallback;
    typedef std::function<void(IDirect3DDevice9*, D3DRENDERSTATETYPE, DWORD)> SetRenderStateCallback;
    typedef std::function<void(IDirect3DDevice9*, IDirect3DVertexShader9*)> SetVertexShaderCallback;
    typedef std::function<void(IDirect3DDevice9*, IDirect3DPixelShader9*)> SetPixelShaderCallback;
    typedef std::function<void(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*)> CreateTextureCallback;
    typedef std::function<void(IDirect3DDevice9*, DWORD, IDirect3DSurface9*)> SetRenderTargetCallback;
    typedef std::function<void(IDirect3DDevice9*, DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD)> ClearCallback;
    typedef std::function<void(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> ResetCallback;

    typedef HRESULT(STDMETHODCALLTYPE* Present_t)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    typedef HRESULT(STDMETHODCALLTYPE* BeginScene_t)(IDirect3DDevice9*);
    typedef HRESULT(STDMETHODCALLTYPE* EndScene_t)(IDirect3DDevice9*);
    typedef HRESULT(STDMETHODCALLTYPE* DrawPrimitive_t)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
    typedef HRESULT(STDMETHODCALLTYPE* DrawIndexedPrimitive_t)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
    typedef HRESULT(STDMETHODCALLTYPE* SetTexture_t)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);
    typedef HRESULT(STDMETHODCALLTYPE* SetRenderState_t)(IDirect3DDevice9*, D3DRENDERSTATETYPE, DWORD);
    typedef HRESULT(STDMETHODCALLTYPE* SetVertexShader_t)(IDirect3DDevice9*, IDirect3DVertexShader9*);
    typedef HRESULT(STDMETHODCALLTYPE* SetPixelShader_t)(IDirect3DDevice9*, IDirect3DPixelShader9*);
    typedef HRESULT(STDMETHODCALLTYPE* CreateTexture_t)(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*);
    typedef HRESULT(STDMETHODCALLTYPE* SetRenderTarget_t)(IDirect3DDevice9*, DWORD, IDirect3DSurface9*);
    typedef HRESULT(STDMETHODCALLTYPE* Clear_t)(IDirect3DDevice9*, DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD);
    typedef HRESULT(STDMETHODCALLTYPE* Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

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

    void RegisterPresentCallback(PresentCallback callback);
    void RegisterBeginSceneCallback(BeginSceneCallback callback);
    void RegisterEndSceneCallback(EndSceneCallback callback);
    void RegisterDrawPrimitiveCallback(DrawPrimitiveCallback callback);
    void RegisterDrawIndexedPrimitiveCallback(DrawIndexedPrimitiveCallback callback);
    void RegisterSetTextureCallback(SetTextureCallback callback);
    void RegisterSetRenderStateCallback(SetRenderStateCallback callback);
    void RegisterSetVertexShaderCallback(SetVertexShaderCallback callback);
    void RegisterSetPixelShaderCallback(SetPixelShaderCallback callback);
    void RegisterCreateTextureCallback(CreateTextureCallback callback);
    void RegisterSetRenderTargetCallback(SetRenderTargetCallback callback);
    void RegisterClearCallback(ClearCallback callback);
    void RegisterResetCallback(ResetCallback callback);

    void RegisterOnCreate(ResourceCallback callback);
    void RegisterOnDestroy(ResourceCallback callback);
    void RegisterOnRelease(ResourceCallback callback);
    void RegisterOnRestore(ResourceCallback callback);


    struct ResourceParams {
        UINT width = 0, height = 0, levels = 1, surfLevel = 0;
        DWORD usage = 0, quality = 0;
        D3DFORMAT format = D3DFMT_A8R8G8B8;
        D3DPOOL pool = D3DPOOL_DEFAULT;
        D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE;
        bool lockable = false;
        HANDLE* pSharedHandle = nullptr;

        std::string shaderCode = "";
        std::string entryPoint = "main";
        std::string target = "";

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

    struct ShaderData // LLM estimation
    {
        IUnknown* base_interface;                       // 0x00
        DWORD unknown_vtable_ptr;                       // 0x04
        IDirect3DResource9* resource_interface_1;       // 0x08
        IDirect3DResource9* resource_interface_2;       // 0x0C
        IDirect3DBaseTexture9* texture_interface_1;     // 0x10
        IDirect3DBaseTexture9* texture_interface_2;     // 0x14
        void* additional_resource;                      // 0x18
        DWORD reference_count;                          // 0x1C

        union {                                         // 0x20
            IDirect3DPixelShader9* pixel_shader;
            IDirect3DVertexShader9* vertex_shader;
        };

        DWORD shader_version;                           // 0x24
        DWORD reserved_1;                               // 0x28
        DWORD compilation_flags;                        // 0x2C

        DWORD shader_enabled;                           // 0x30
        DWORD reserved_2;                               // 0x34
        DWORD reserved_3;                               // 0x38
        DWORD texture_dimension_flags;                  // 0x3C
        DWORD constant_buffer_size;                     // 0x40
        DWORD active_samplers;                          // 0x44
        DWORD instruction_slots;                        // 0x48
        DWORD bytecode_length;                          // 0x4C

        void* bytecode_memory;                          // 0x50
        DWORD reserved_4;                               // 0x54
        DWORD creation_timestamp;                       // 0x58
        DWORD bytecode_checksum;                        // 0x5C

        void* texture_stage_state;                      // 0x60
        void* sampler_state_block;                      // 0x64
        DWORD active_texture_stages;                    // 0x68
        DWORD primary_sampler_index;                    // 0x6C
        DWORD blend_stage_enabled;                      // 0x70
        DWORD alpha_test_enabled;                       // 0x74
        DWORD render_state_flags;                       // 0x78
        DWORD fog_enabled;                              // 0x7C

        DWORD lighting_enabled;                         // 0x80
        DWORD vertex_shader_constants;                  // 0x84
        DWORD pixel_shader_constants;                   // 0x88
        DWORD texture_filter_flags;                     // 0x8C
        DWORD mipmap_settings;                          // 0x90
        DWORD reserved_5;                               // 0x94
        DWORD reserved_6;                               // 0x98
        DWORD reserved_7;                               // 0x9C

        DWORD font_antialiasing;                        // 0xA0
        DWORD subpixel_rendering;                       // 0xA4
        DWORD reserved_8;                               // 0xA8

        char coordinate_data[20];                       // 0xAC-0xBF

        DWORD extended_flags_1;                         // 0xC0
        DWORD reserved_9;                               // 0xC4
        float unknown_float_1;                          // 0xC8
        DWORD combined_hash;                            // 0xCC
        DWORD validation_flag;                          // 0xD0

        WORD max_texture_width;                         // 0xD4
        WORD max_texture_height;                        // 0xD8
        DWORD padding[8];                               // 0xDC-0xFB
        DWORD final_validation;                         // 0xFC
    };

    struct ShaderEntry {
        uint32_t data[4];
        uint32_t length;
        const char* profile;
    };

    extern std::span<const ShaderEntry> s_shaders;
    extern const size_t s_shaders_count;

    typedef std::function<void(ShaderData*)> VertexShaderInitCallback;
    typedef std::function<void(ShaderData*)> PixelShaderInitCallback;

    void RegisterVertexShaderInit(VertexShaderInitCallback callback);
    void RegisterPixelShaderInit(PixelShaderInitCallback callback);

    IDirect3DVertexShader9* CompileVertexShader(ResourceParams p);
    IDirect3DPixelShader9* CompilePixelShader(ResourceParams p);

    bool CreateTexture(IDirect3DTexture9** ppTexture, ResourceParams p);
    bool CreateRenderTarget(IDirect3DSurface9** ppSurface, ResourceParams p);
}