#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Graphics/CGxShader.h"
#include "Graphics/CGxTex.h"

struct CGxPool;
struct CGxPushedRenderState;
struct CGxAppRenderState;
struct CGxStateBom;
struct IDirect3DDevice9;
struct IDirect3DSurface9;

struct CGxBuf {
    TSLink<CGxBuf> link;
    CGxPool* pool;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t size;
    uint32_t index;
    uint8_t is_filled;
    uint8_t _unk1D;
    uint8_t _unk1E;
    uint8_t _unk1F;
};

static_assert(sizeof(CGxBuf) == 0x20);

struct CGxPool {
    TSLink<CGxPool> link;
    GxPoolTarget target;
    GxPoolUsage usage;
    uint32_t size;
    uint32_t allocated_bytes;
    unk_t unk;
    uint32_t cur_bufs_size;
    TSExplicitList<CGxBuf> list_buf;
    uint32_t flags;
    const char* name;
};

static_assert(sizeof(CGxPool) == 0x34);

struct CGxMatrixStack {
    unk_t _unk[70];
};

static_assert(sizeof(CGxMatrixStack) == 0x118);

struct CGxViewport {
    float min_x;
    float max_x;
    float min_y;
    float max_y;
    float min_z;
    float max_z;
};

static_assert(sizeof(CGxViewport) == 0x18);

class CGxApiLight {
public:
    float f_[16];
};

static_assert(sizeof(CGxApiLight) == 0x40);

struct CGxDeviceLightSlot {
    CGxApiLight light;
    uint32_t enabled;
    uint32_t dirty;
};

static_assert(sizeof(CGxDeviceLightSlot) == 0x48);

struct CGxVertexAttrib {
    uint32_t attrib;
    uint32_t type;
    uint32_t offset;
    unk_t _unk0C;
};

static_assert(sizeof(CGxVertexAttrib) == 0x10);

class EmergencyMem {
public:
    TSGrowableArray<uint8_t> buffer_;
    uint8_t locked_;
    char _pad[3];
};

static_assert(sizeof(EmergencyMem) == 0x14);

struct CGxDeviceRenderTarget {
    CGxTex* tex;
    uint32_t face;
    IDirect3DSurface9* surface;
};

static_assert(sizeof(CGxDeviceRenderTarget) == 0xC);

struct CGxBatch {
    int32_t prim_type;
    uint32_t start;
    uint32_t count;
    uint16_t min_index;
    uint16_t max_index;
};

static_assert(sizeof(CGxBatch) == 0x10);

struct CGxQuery {
    void* api_query;
    uint32_t type;
    TSLink<CGxQuery> link;
};

static_assert(sizeof(CGxQuery) == 0x10);

struct CGxCaps {
    uint32_t num_tmus;
    uint32_t pixel_center_on_edge;
    uint32_t texel_center_on_edge;
    uint32_t max_streams;
    unk_t _unk10;
    unk_t _unk14;
    unk_t _unk18;
    uint32_t max_vertex_index;
    uint32_t can_autogen_mipmap;
    uint32_t tex_format_supported[eGxTexFormatCount];
    unk_t _unk58;
    uint32_t cubemaps;
    unk_t _unk60;
    uint32_t non_pow2;
    unk_t _unk68;
    uint32_t max_tex_size[4];
    uint32_t rt_format_supported[eGxTexFormatCount];
    unk_t _unkB0;
    uint32_t shader_targets[6];
    uint32_t shader_max_constants[6];
    uint32_t trilinear;
    uint32_t anisotropic;
    uint32_t max_anisotropy;
    uint32_t depth_bias;
    uint32_t color_write_enable;
    uint32_t max_clip_planes;
    uint32_t hw_cursor;
    uint32_t occlusion_query;
    unk_t _unk104;
    float max_point_size;
    unk_t _unk10C;
    uint32_t blend_factor;
    unk_t _unk114[5];
    unk_t _unk128;
    unk_t _unk12C;
    unk_t _unk130;
    unk_t _unk134;
    unk_t _unk138;
};

static_assert(sizeof(CGxCaps) == 0x13C);

class CGxDevice {
public:
    inline static const char* (&ui_pixel_shaders_desc)[2] = *reinterpret_cast<const char* (*)[2]>(0x00AC0EF8);

    inline static auto& ui_pixel_shaders = *reinterpret_cast<std::array<CGxShader*, 2>*>(0x00B47934);
    inline static auto& ui_vertex_shaders = *reinterpret_cast<std::array<CGxShader*, 2>*>(0x00B47940);
    inline static auto& font_index_pool = *reinterpret_cast<CGxPool**>(0x00C7D2DC);
    inline static auto& font_index_buffer = *reinterpret_cast<CGxBuf**>(0x00C7D2E0);
    // profile names per GxShaderType, indexed by caps_.shader_targets ("none", "vs_1_1", ... / "ps_1_1", ...)
    inline static const auto& kShaderProfileNames = *reinterpret_cast<const char* const* const (*)[6]>(0x00AD8890);
    static constexpr std::array<uint32_t, eGxShaderCount> kShaderProfileCounts = {12, 2, 2, 3, 14, 4};

    // kShaderProfileNames[type][profile], nullptr out of range
    [[nodiscard]]
    static const char* shaderProfileName(GxShaderType type, uint32_t profile) {
        if (type >= eGxShaderCount || profile >= kShaderProfileCounts[type]) { return nullptr; }
        return std::span(kShaderProfileNames[type], kShaderProfileCounts[type])[profile];
    }

    static constexpr uint32_t nextShaderProfile(GxShaderType type, uint32_t profile) {
        if (type == eGxShaderVertex) { return profile == 2 || profile == 3 ? profile - 1 : 0; }
        if (type == eGxShaderPixel) {
            if (profile >= 2 && profile <= 4) { return profile - 1; }
            if (profile == 9 || profile == 10) { return 8; }
        }
        return 0;
    }

    virtual void texMarkAsUpdated(CGxTex* tex) = 0;                                  // 0 (0x00)
    virtual void rsSendToHw() = 0;                                                   // 1 (0x04)
    virtual void* cursorCreate(const void* params) = 0;                              // 2 (0x08)
    virtual void cursorDestroy(void* cursor) = 0;                                    // 3 (0x0C)
    virtual void cursorDraw() = 0;                                                   // 4 (0x10)
    virtual void notifyOnDeviceRestored() = 0;                                       // 5 (0x14)
    virtual void notifyOnTextureRecreation() = 0;                                    // 6 (0x18)
    virtual void notifyOnStereoChanged() = 0;                                        // 7 (0x1C)
    virtual ~CGxDevice() = 0;                                                        // 8 (0x20)
    virtual int32_t deviceCreate(void* window_handle, const void* caps) = 0;         // 9 (0x24)
    virtual int32_t deviceCreate() = 0;                                              // 10 (0x28)
    virtual void deviceDestroy() = 0;                                                // 11 (0x2C)
    virtual void deviceEvictResources() = 0;                                         // 12 (0x30)
    virtual int32_t deviceSetFormat(const void* format) = 0;                         // 13 (0x34)
    virtual void deviceSetBaseMipLevel(int32_t level) = 0;                           // 14 (0x38)
    virtual void deviceSetGamma(float gamma, float contrast, float brightness) = 0;  // 15 (0x3C)
    virtual void deviceSetGamma(const void* gamma_ramp) = 0;                         // 16 (0x40)
    virtual void deviceWindow() = 0;                                                 // 17 (0x44)
    virtual void deviceTakeScreenShot() = 0;                                         // 18 (0x48)
    virtual void deviceReadScreenShot(uint32_t* out_width, uint32_t* out_height,
        ColorBGRA<>** out_pixels) = 0;                                                                     // 19 (0x4C)
    virtual void deviceReadPixels(Recti* rect, TSGrowableArray<ColorBGRA<>>* out_pixels) = 0;              // 20 (0x50)
    virtual int32_t deviceReadDepths(void* buffer, uint32_t width, uint32_t height) = 0;                   // 21 (0x54)
    virtual int32_t deviceWM(void* hwnd, uint32_t u_msg, uint32_t w_param, uint32_t l_param) = 0;          // 22 (0x58)
    virtual void deviceSetRenderTarget(uint32_t target, CGxTex* tex, uint32_t face) = 0;                   // 23 (0x5C)
    virtual void blizzardCursorFunc0068E510() = 0;                                                         // 24 (0x60)
    virtual void blizzardCursorFunc0068E540() = 0;                                                         // 25 (0x64)
    virtual void deviceResolveDepthBuffer() = 0;                                                           // 26 (0x68)
    virtual void deviceCopyTex(CGxTex* dst, CGxTex* src) = 0;                                              // 27 (0x6C)
    virtual void deviceOverride(int32_t enable, int32_t option) = 0;                                       // 28 (0x70)
    virtual void addDeviceRestoredCallback(void (*callback)()) = 0;                                        // 29 (0x74)
    virtual void removeDeviceRestoredCallback(void (*callback)()) = 0;                                     // 30 (0x78)
    virtual void addTextureRecreationCallback(void (*callback)()) = 0;                                     // 31 (0x7C)
    virtual void removeTextureRecreationCallback(void (*callback)()) = 0;                                  // 32 (0x80)
    virtual void addStereoChangedCallback(void (*callback)()) = 0;                                         // 33 (0x84)
    virtual void removeStereoChangedCallback(void (*callback)()) = 0;                                      // 34 (0x88)
    virtual void capsWindowSize(Rectf* rect) = 0;                                                          // 35 (0x8C)
    virtual void capsWindowSizeInScreenCoords(Rectf* rect) = 0;                                            // 36 (0x90)
    virtual void logCrashInfo() = 0;                                                                       // 37 (0x94)
    virtual void scenePresent() = 0;                                                                       // 38 (0x98)
    virtual void sceneClear(uint32_t flags, uint32_t color) = 0;                                           // 39 (0x9C)
    virtual void xformSetProjection(const Mat4f* matrix) = 0;                                              // 40 (0xA0)
    virtual void xformSetView(const Mat4f* matrix) = 0;                                                    // 41 (0xA4)
    virtual void draw(const CGxBatch* batch, int32_t indexed) = 0;                                         // 42 (0xA8)
    virtual void primBegin(int primitive_type, uint32_t vertex_count) = 0;                                 // 43 (0xAC)
    virtual void primDrawElements(int primitive_type, uint32_t index_count, const uint16_t* indices) = 0;  // 44 (0xB0)
    virtual void primVertex(float x, float y, float z) = 0;                                                // 45 (0xB4)
    virtual void primTexCoord(float u, float v) = 0;                                                       // 46 (0xB8)
    virtual void primNormal(float nx, float ny, float nz) = 0;                                             // 47 (0xBC)
    virtual void primColor(uint32_t argb) = 0;                                                             // 48 (0xC0)
    virtual void primEnd() = 0;                                                                            // 49 (0xC4)
    virtual void primFlush() = 0;                                                                          // 50 (0xC8)
    virtual void masterEnableSet(int state, int32_t enable) = 0;                                           // 51 (0xCC)
    virtual void poolSizeSet(uint32_t size) = 0;                                                           // 52 (0xD0)
    virtual void poolDestroy(CGxPool* pool) = 0;                                                           // 53 (0xD4)
    virtual void* bufLock(CGxBuf* buf) = 0;                                                                // 54 (0xD8)
    virtual int32_t bufUnlock(CGxBuf* buf, uint32_t size) = 0;                                             // 55 (0xDC)
    virtual void bufData(CGxBuf* buf, const void* data, uint32_t size, uint32_t offset) = 0;               // 56 (0xE0)
    virtual int32_t texCreate(GxTexTarget target, int32_t width, int32_t height, int32_t depth, GxTexFormat format,
        GxTexFormat data_format, uint32_t flags, void* param, CGxTex::ProcFunc callback, const char* name,
        CGxTex** out) = 0;                                                                          // 57 (0xE4)
    virtual void texDestroy(CGxTex* tex) = 0;                                                       // 58 (0xE8)
    virtual void texCopy(CGxTex* dst, CGxTex* src) = 0;                                             // 59 (0xEC)
    virtual void texStretch(CGxTex* dst, CGxTex* src) = 0;                                          // 60 (0xF0)
    virtual void stubTex00632050() = 0;                                                             // 61 (0xF4)
    virtual void* queryCreate(int type) = 0;                                                        // 62 (0xF8)
    virtual void queryDestroy(void* query) = 0;                                                     // 63 (0xFC)
    virtual void queryBegin(void* query) = 0;                                                       // 64 (0x100)
    virtual void queryEnd(void* query) = 0;                                                         // 65 (0x104)
    virtual int32_t queryGetParam(void* query, int param, void* value) = 0;                         // 66 (0x108)
    virtual int32_t queryGetData(void* query, void* data, uint32_t data_size, uint32_t flags) = 0;  // 67 (0x10C)
    virtual uint32_t shaderCreate(CGxShader** shaders, GxShaderType type, const char* profile, const char* name,
        uint32_t mut_count) = 0;                                                                         // 68 (0x110)
    virtual void shaderDestroy(CGxShader** shader) = 0;                                                  // 69 (0x114)
    virtual void shaderConstantsSet(int type, uint32_t reg_idx, const float* data, uint32_t count) = 0;  // 70 (0x118)
    virtual int32_t shaderReload(CGxShader* shader, const char* path, const char* name) = 0;             // 71 (0x11C)
    virtual void iShaderCreate(CGxShader* shader) = 0;                                                   // 72 (0x120)
    virtual void cursorSetVisible(int32_t visible) = 0;                                                  // 73 (0x124)
    virtual void cursorLock(int32_t lock) = 0;                                                           // 74 (0x128)
    virtual void cursorUnlock() = 0;                                                                     // 75 (0x12C)
    virtual void stereoSetConvergence(float val) = 0;                                                    // 76 (0x130)
    virtual float stereoGetConvergence() = 0;                                                            // 77 (0x134)
    virtual void stereoSetSeparation(float val) = 0;                                                     // 78 (0x138)
    virtual float stereoGetSeparation() = 0;                                                             // 79 (0x13C)
    virtual int32_t stereoEnabled() = 0;                                                                 // 80 (0x140)
    virtual void nullsub0() = 0;
    virtual void nullsub1() = 0;
    virtual void nullsub2() = 0;

    TSGrowableArray<CGxPushedRenderState> pushed_render_states_;
    TSGrowableArray<uint32_t> _14;
    TSGrowableArray<GxRenderState> _24;
    uint32_t prim_type_;
    unk_t _38[2];
    uint32_t prim_active_;
    Vec3f prim_cur_vertex_;
    Vec2f prim_cur_texcoords_[8];
    Vec3f prim_cur_normal_;
    ColorBGRA<> prim_cur_color_;
    TSGrowableArray<Vec3f> prim_vertices_;
    TSGrowableArray<Vec2f> prim_texcoords_[8];
    TSGrowableArray<Vec3f> prim_normals_;
    TSGrowableArray<ColorBGRA<>> prim_colors_;
    TSGrowableArray<uint16_t> prim_indices_;
    GxPrimMask prim_mask_;
    Rectf def_window_;
    Rectf cur_window_;
    TSGrowableArray<void (*)()> device_restored_callbacks_;
    TSGrowableArray<void (*)()> texture_recreation_callbacks_;
    TSGrowableArray<void (*)()> stereo_changed_callbacks_;
    uint32_t processor_features_;
    uint32_t cpu_features_;
    char format_[88];
    CGxCaps caps_;
    uint32_t base_mip_level_;
    char gamma_ramp_saved_[1536];
    char gamma_ramp_current_[1536];
    void* window_proc_;
    uint32_t has_context_;
    unk_t _F5C;
    uint32_t window_visible_;
    unk_t _F64;
    unk_t _F68;
    uint32_t viewport_dirty_;
    CGxViewport viewport_;
    Mat4f _F88;
    Mat4f _FC8;
    CGxMatrixStack matrix_stacks1_[11];
    CGxMatrixStack matrix_stacks2_[8];
    uint32_t clip_planes_dirty_;
    Plane clip_planes_[6];
    uint32_t scissor_dirty_;
    float scissor_rect_[4];
    CGxDeviceLightSlot lights_[4];
    TSHashTable<CGxShader> shader_tables_[6];
    uint32_t master_enables_;
    uint32_t hw_master_enables_;
    TSExplicitList<CGxPool> pools_;
    TSExplicitList<CGxBuf> bufs_;
    CGxBuf* locked_bufs_[2];
    CGxPool* vertex_pool_;
    CGxPool* index_pool_;
    CGxBuf* vertex_buf_;
    CGxBuf* index_buf_;
    CGxVertexAttrib vertex_attribs_[14];
    CGxBuf* vertex_attrib_bufs_[14];
    uint32_t vertex_attrib_mask_;
    uint32_t vertex_attrib_dirty_;
    uint32_t vertex_format_;
    CGxBuf* vertex_format_buf_;
    uint32_t vertex_format_stride_;
    CGxBuf* prim_index_;
    uint32_t prim_index_active_;
    EmergencyMem emergency_mem_[2];
    TSBaseArray<CGxAppRenderState> app_render_states_;
    TSBaseArray<CGxStateBom> state_boms_;
    TSExplicitList<CGxTex> textures_;
    CGxDeviceRenderTarget render_targets_[2];
    TSExplicitList<CGxQuery> queries_;
    uint32_t screenshot_pending_;
    uint32_t screenshot_size_[2];
    TSGrowableArray<ColorBGRA<>> screenshot_pixels_;
    uint32_t cursor_visible_;
    uint32_t cursor_hardware_;
    uint32_t cursor_hotspot_[2];
    uint32_t cursor_bits_[32 * 32];
    CGxTex* cursor_tex_;
    float cursor_depth_;

    static constexpr uintptr_t getAddr() { return 0x00C5DF88; }

    static CGxDevice* getDevice() { return *reinterpret_cast<CGxDevice**>(getAddr()); }

    [[nodiscard]]
    Vec2f viewportPixelSize() const {
        const double win_w = cur_window_.right;
        const double win_h = cur_window_.top;
        const long x = std::lround(viewport_.min_x * win_w);
        const long y = std::lround((1.0 - viewport_.max_y) * win_h);
        return {static_cast<float>(std::lround(viewport_.max_x * win_w - static_cast<double>(x))),
            static_cast<float>(std::lround((1.0 - viewport_.min_y) * win_h - static_cast<double>(y)))};
    }

    HOOKKIT_HOOK(deviceCreate, 0x00682CB0, ::hookkit::Conv::eThiscall, int, CGxDevice*, void*, const void*);
    HOOKKIT_HOOK(notifyOnDeviceRestored, 0x006843B0, ::hookkit::Conv::eThiscall, int, CGxDevice*);
    // shaders, type, path, name, count: opens path/<profile>/name.bls from the best supported profile down and runs
    HOOKKIT_HOOK(iShaderLoad, 0x00684970, ::hookkit::Conv::eThiscall, uint32_t, CGxDevice*, CGxShader**, GxShaderType,
        const char*, const char*, uint32_t);
    HOOKKIT_HOOK(shaderCreateVertex, 0x006AA0D0, ::hookkit::Conv::eThiscall, void, CGxDevice*, CGxShader*);
    HOOKKIT_HOOK(shaderCreatePixel, 0x006AA070, ::hookkit::Conv::eThiscall, void, CGxDevice*, CGxShader*);
    // state, value
    HOOKKIT_HOOK(rsSet, 0x00685F50, ::hookkit::Conv::eThiscall, void, CGxDevice*, GxRenderState, void*);
    // target, item size, item count; the device's per-target stream buffer, grown to fit and reset
    HOOKKIT_HOOK(
        bufStream, 0x00684850, ::hookkit::Conv::eThiscall, CGxBuf*, CGxDevice*, GxPoolTarget, uint32_t, uint32_t);
    HOOKKIT_HOOK(saveRenderTarget, 0x00682D50, ::hookkit::Conv::eThiscall, int, CGxDevice*, int, CGxTex**);
    HOOKKIT_HOOK(deviceScreenShot, 0x006841D0, ::hookkit::Conv::eThiscall, void, CGxDevice*);
    // size, flag, name
    HOOKKIT_HOOK(poolCreate, 0x006876D0, ::hookkit::Conv::eThiscall, CGxPool*, CGxDevice*, GxPoolTarget, GxPoolUsage,
        uint32_t, uint32_t, const char*);

    // bbox, color, matrix, z_bias, flags, vtx_mode, depth_bias
    HOOKKIT_HOOK_HANDLE(
        projectTex2d, 0x007E4370, ::hookkit::Conv::eCdecl, void*, AaBox*, Vec4u8*, Mat4f*, float, int, char, float);
    // stream buf, vert count; unlocks, draws the verts with the font index buffer and relocks, returns the new write ptr
    HOOKKIT_HOOK_HANDLE(flushBuffer, 0x006C48D0, ::hookkit::Conv::eCdecl, void*, CGxBuf**, uint32_t);
    HOOKKIT_HOOK_HANDLE(initFontIndexBuffer, 0x006C47B0, ::hookkit::Conv::eCdecl, CGxBuf*);
    // pool, stride, element count (bytes = stride * count), index
    HOOKKIT_HOOK_HANDLE(bufCreate, 0x00687660, ::hookkit::Conv::eStdcall, CGxBuf*, CGxPool*, uint32_t, uint32_t, void*);
    HOOKKIT_HOOK_HANDLE(setRenderTarget, 0x0057E4F0, ::hookkit::Conv::eCdecl, void, uint32_t, CGxTex*, uint32_t);
    HOOKKIT_HOOK_HANDLE(gxSceneClear, 0x006813B0, ::hookkit::Conv::eCdecl, void, uint32_t, uint32_t);
    HOOKKIT_HOOK_HANDLE(initUIShaders, 0x00483060, hookkit::Conv::eCdecl, void);
    HOOKKIT_HOOK_HANDLE(shutdownUIShaders, 0x004830A0, hookkit::Conv::eCdecl, void);
    // cfmt, fmt, blp_fmt, alphadepth
    HOOKKIT_HOOK_HANDLE(resolveGxFormat, 0x004B5FE0, hookkit::Conv::eCdecl, char*, CPixelFormat*, GxTexFormat*,
        blpcodec::BLPPixelFormat, int8_t);
};

static_assert(sizeof(CGxDevice) == 0x3968);
