#pragma once

#include <blpcodec/include/blpcodec.h>

#include <cstdint>

#include "BaseTypes.h"

#include "Async/Async.h"
#include "Common/Common.h"
#include "Graphics/CGxTex.h"
#include "Lib/Storm.h"

struct TCACHEENTRY {
    inline static auto& TSHT = *reinterpret_cast<TSHashTable<TCACHEENTRY>*>(0x00B6BA54);

    enum State : uint32_t {
        eTcacheFlagSizeMask = 0xFFFFF,
        eTcacheFlagLoadFailed = 0x100000,
    };

    struct TexDesc {
        enum Flags : uint16_t {
            eTcacheFlaG16Opaque = 0x1,
            eTcacheFlaG16GenMips = 0x2,
            eTcacheFlaG16Nomip = 0x4,
        };

        uint16_t width;
        uint16_t height;
        uint8_t mip_count;
        uint8_t alpha_depth;
        Flags flags;
    };

    static_assert(sizeof(TexDesc) == 0x8);

    TSHashObject<TCACHEENTRY> hash_obj;
    CAsyncObject* asc_obj;
    TexDesc desc;
    char name[128];
    uint32_t ref_count;
    uint32_t heap_list_id;  // 0x00B4AFC0
    uint8_t* buffer;
    State load_status;

    blpcodec::BLP2Header* getHeader() const { return reinterpret_cast<blpcodec::BLP2Header*>(this->buffer); }
};

static_assert(sizeof(TCACHEENTRY) == 0xB4);

struct CTextureBlobEntry {
    uint32_t name_offset;
    uint32_t data_offset;
    uint8_t width;
    uint8_t height;
    uint8_t flags;
    uint8_t pixel_format;
};

static_assert(sizeof(CTextureBlobEntry) == 0xC);

class CTextureBlob {
public:
    inline static auto* TSL = reinterpret_cast<TSExplicitList<CTextureBlob>*>(0x00AC3754);

    char path_[260];
    void* sfile_;
    CTextureBlobEntry* entries_;
    uint32_t entry_count_;
    const char* string_table_;
    uint8_t* data_block_;
    TSLink<CTextureBlob> link_;
};

static_assert(sizeof(CTextureBlob) == 0x120);

class CTextureBlobTexture {
public:
    inline static auto* TSHT = reinterpret_cast<TSHashTable<CTextureBlobTexture>*>(0x00B4A2D0);

    TSHashObject<CTextureBlobTexture, const char*> hash_obj_;
    CTextureBlob* parent_;
    CTextureBlobEntry* entry_;
};

static_assert(sizeof(CTextureBlobTexture) == 0x20);

class CTexture : CHandle {
public:
    struct CTextureHashedCache {
        TSHashTable<CTexture> table;
        TSExplicitList<CTexture> list;
    };

    inline static auto& cache = *reinterpret_cast<CTextureHashedCache*>(0x00B49CA4);
    inline static auto& TSL = *reinterpret_cast<TSExplicitList<CTexture>*>(0x00AC3348);

    inline static auto& TSL_async_buffered = *reinterpret_cast<TSExplicitList<CAsyncObject>*>(0x00AC337C);
    inline static auto& TSL_async_unbuffered = *reinterpret_cast<TSExplicitList<CAsyncObject>*>(0x00AC3388);

    inline static auto& in_crit_section = *reinterpret_cast<int*>(0x00B4A1EC);

    struct CTextureCacheKey {
        const char* filename;
        GxTexFlags tex_flags;
        uint32_t any_flags;
    };

    static_assert(sizeof(CTextureCacheKey) == 0xC);

    enum EcTextureState : uint16_t {
        eTexStateNone = 0x0,
        eTexStateOpaque = 0x1,
        eTexStateDirectOpen = 0x2,
        eTexStateUseAtlas = 0x4,
        eTexStateLowPrio = 0x10,
        eTexStateStreaming = 0x20,
    };

    TSHashObject<CTexture, const char*> hash_obj_;
    GxTexFlags cache_lookup_flags_;
    uint32_t cache_wildcard_flags_;
    EcTextureState state_flags_;
    uint8_t base_mip_level_;
    uint8_t alpha_depth_;
    CStatus status_;
    CAsyncObject* async_object_;
    CGxTex* gx_tex_;
    uint32_t is_cube_map_;
    uint16_t width_;
    uint16_t height_;
    GxTexFormat format_;
    GxTexFormat data_format_;
    GxTexFlags creation_flags_;
    CGxTexAtlasPage* cache_entry_;
    uint32_t atlas_slot_;
    TSLink<CTexture> link_;
    char name_[260];

    // rw flag, status
    HOOKKIT_HOOK_HANDLE(fetchGxTex, 0x004B6CB0, hookkit::Conv::eCdecl, CGxTex*, CTexture*, int, CStatus*);
    // hash, src
    HOOKKIT_HOOK_HANDLE(
        fetchFromCache, 0x004B6D90, hookkit::Conv::eCdecl, CTexture*, CTexture*, HashKeyStri, const char*);
    // src, fmt, status, flags
    HOOKKIT_HOOK_HANDLE(
        create, 0x004B9760, hookkit::Conv::eCdecl, CTexture*, const char*, GxTexFlags, CStatus*, uint32_t);

    // unk (0), width, height, 0x18, format, texture format, flags, user data, update callback, name, unk (1)
    HOOKKIT_HOOK_HANDLE(allocAndRegister, 0x004B8C80, hookkit::Conv::eCdecl, CTexture*, uint32_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint32_t, uint32_t, void*, void*, const char*, uint32_t);

    HOOKKIT_HOOK_HANDLE(createRenderTarget, 0x004B9200, hookkit::Conv::eCdecl, CTexture*, uint32_t, uint32_t, uint32_t,
        uint32_t, GxTexFlags, void*, void*, const char*, uint32_t);

    HOOKKIT_HOOK(sddtor, 0x004B91D0, hookkit::Conv::eThiscall, CTexture*, CTexture*, char);

    static constexpr hookkit::WildAbi<4> kLoadBlpAbi = {{{hookkit::ArgLoc::inReg(hookkit::Reg::eAx),
        hookkit::ArgLoc::inReg(hookkit::Reg::eCx), hookkit::ArgLoc::onStack(0), hookkit::ArgLoc::onStack(4)}}};
    // extension, src, load flags, fmt
    HOOKKIT_HOOK_WILD(loadBlp, 0x004B8BE0, hookkit::Conv::eUsercall, CTexture*, kLoadBlpAbi, const char*, const char*,
        uint32_t, GxTexFlags);
    // src, extension, load flags, fmt, status
    HOOKKIT_HOOK(loadTga, 0x004B95B0, hookkit::Conv::eCdecl, CTexture*, const char*, uint32_t, GxTexFlags, CStatus*);
};

using HTexture = CTexture*;
static_assert(sizeof(CTexture) == 0x170);
