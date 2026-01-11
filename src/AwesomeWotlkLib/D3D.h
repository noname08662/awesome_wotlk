#pragma once
#include <d3d9.h>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

namespace D3D {
    void initialize();

    IDirect3DDevice9* GetDevice();

    using ResourceCallback = std::function<void()>;

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

    void RegisterOnLost(ResourceCallback callback);
    void RegisterOnReset(ResourceCallback callback);
    void RegisterOnDestroy(ResourceCallback callback);

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

    enum class ShaderType {
        Vertex,
        Pixel
    };

    using VertexShaderInitCallback = std::function<IDirect3DVertexShader9* (ShaderData*)>;
    using PixelShaderInitCallback = std::function<IDirect3DPixelShader9* (ShaderData*)>;

    IDirect3DVertexShader9* CompileVertexShader(const char* code, const char* entryPoint = "main", const char* target = "vs_3_0");
    IDirect3DPixelShader9* CompilePixelShader(const char* code, const char* entryPoint = "main", const char* target = "ps_3_0");

    void RegisterVertexShaderInit(VertexShaderInitCallback callback);
    void RegisterPixelShaderInit(PixelShaderInitCallback callback);

    bool CreateTexture(
        IDirect3DTexture9** ppTexture,
        UINT width,
        UINT height,
        UINT levels = 1,
        DWORD usage = 0,
        D3DFORMAT format = D3DFMT_A8R8G8B8,
        D3DPOOL pool = D3DPOOL_MANAGED,
        bool clearToZero = true
    );
}