#pragma once

#include "Graphics/CGxStringBatch.h"
#include "Graphics/GraphicsEnums.h"
#include "Math/Primitives.h"
#include "Widget/CSimpleTexture.h"

struct CSimpleBatchedMesh {
    CTexture* texture;
    void* unk_params;
    GxBlend blend_mode;
    CGxShader* shader;
    uint32_t vertex_count;
    Vec3f* vertices;
    const CSimpleTexture::CTexCoords* uvs;
    const Vec4u8* colors;
    uint32_t color_count;
    const uint16_t* indices;
    uint32_t index_count;
    int32_t is_cached;
    float uv_scale;
    float uv_offset_u;
    float uv_offset_v;
};

static_assert(sizeof(CSimpleBatchedMesh) == 0x3C);

struct CRenderBatch {
    struct RENDERCALLBACKNODE {
        TSLink<RENDERCALLBACKNODE> link;
        void(__cdecl* callback)(void*);
        CSimpleFrame* parent;
    };

    static_assert(sizeof(RENDERCALLBACKNODE) == 0x10);

    virtual ~CRenderBatch();

    uint32_t cb_count;
    TSGrowableArray<CSimpleBatchedMesh> batched_meshes;
    CGxStringBatch* gx_batch;
    TSExplicitList<RENDERCALLBACKNODE> cb_list;
    TSLink<CRenderBatch> link;

    HOOKKIT_HOOK_HANDLE(drawBatch, 0x00484B00, hookkit::Conv::eCdecl, void, CRenderBatch*);
    HOOKKIT_HOOK(
        newCbNode, 0x004858E0, hookkit::Conv::eThiscall, void*, CRenderBatch*, void(__cdecl*)(void*), CSimpleFrame*);
};

static_assert(sizeof(CRenderBatch) == 0x30);
