#pragma once
#include "D3D.h"
#include "GameClient.h"
#include "MSDFUtils.h"

#include <cstdint>
#include <mutex>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include FT_OUTLINE_H

// these structs are NOT to be trusted, rough LLM estimation
template<typename T>
struct TSGrowableArray
{
    uint32_t m_capacity;
    uint32_t m_count;
    T* m_data;
    uint32_t m_granularity;
};

struct CFontVertex
{
    C3Vector pos;
    float u, v;
};

struct CLayoutFrame;
struct CFramePoint
{
    float x, y;
    CLayoutFrame* layoutFrame;
    uint32_t flags;
};

struct CLayoutChildNode
{
    CLayoutChildNode* prev;
    CLayoutChildNode* next;
    CLayoutFrame* child;
    uint32_t ukn;
};

struct CLayoutFrame_vtbl
{
    void(__thiscall* Destroy)(void* pThis);
    void(__thiscall* LoadXML)(void* pThis);
    CLayoutFrame* (__thiscall* GetLayoutParent)(void* pThis);
    void(__thiscall* PropagateProtectFlagToParent)(void* pThis);
    bool(__thiscall* AreChildrenProtected)(void* pThis, int* result);
    void(__thiscall* SetLayoutScale)(void* pThis);
    void(__thiscall* SetLayoutDepth)(void* pThis);
    void(__thiscall* SetWidth)(void* pThis, uint32_t width);
    void(__thiscall* SetHeight)(void* pThis, uint32_t height);
    void(__thiscall* SetSize)(void* pThis);
    double(__thiscall* GetWidth)(void* pThis);
    double(__thiscall* GetHeight)(void* pThis);
    void(__thiscall* GetSize)(void* pThis);
    void(__thiscall* GetClampRectInsets)(void* pThis);
    void(__thiscall* ukn4)(void* pThis);
    int(__thiscall* CanBeAnchorFor)(void* pThis, void* other);
    void(__thiscall* ukn6)(void* pThis);
    void(__thiscall* ukn7)(void* pThis);
    void(__thiscall* OnFrameSizeChanged)(void* pThis);
};

struct CLayoutFrame
{
    CLayoutFrame_vtbl* __vftable;       // 0x00
    uint32_t ukn1;                      // 0x04
    uint32_t ukn2;                      // 0x08
    CFramePoint* framePoints[9];        // 0x0C
    uint32_t ukn3;                      // 0x30
    CLayoutChildNode* childUkn;         // 0x34
    CLayoutChildNode* children;         // 0x38
    uint32_t ukn4;                      // 0x3C
    uint32_t flags;                     // 0x40
    float left;                         // 0x44
    float right;                        // 0x48
    float top;                          // 0x4C
    float bottom;                       // 0x50
    float width;                        // 0x54
    float height;                       // 0x58
    float scale;                        // 0x5C
    float f8;                           // 0x60
    float minResizeY;                   // 0x64
    float minResizeX;                   // 0x68
    float maxResizeY;                   // 0x6C
    float maxResizeX;                   // 0x70
};

struct CGlyphMetrics
{
    void* m_pixelData;
    uint32_t m_bufferSize;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_widthPadded;
    float m_advanceX;
    float m_horiBearingX;
    uint32_t unk_24;
    uint32_t m_bearingY;
    uint32_t m_verAdv;
    float v1, u0, v0, u1;
}; // sizeof = 0x38

struct CGlyphCacheEntry
{
    uint32_t m_codepoint;
    void* m_ptr1;
    void* m_ptr2;
    void* m_ptr3;
    void* m_ptr4;
    char unk_14[20];
    uint32_t m_texturePageIndex;
    CFramePoint* m_atlasCellIndex;
    uint32_t m_cellIndexMin;
    uint32_t m_cellIndexMax;
    CGlyphMetrics m_metrics;
}; // sizeof = 0x70

struct CFontGeomBatch
{
    CFontGeomBatch* m_prev;
    CFontGeomBatch* m_next;
    TSGrowableArray<CFontVertex> m_verts;
    TSGrowableArray<uint16_t> m_indices;
};

struct CGlyphTexCell
{
    uint32_t codepoint;
    uint16_t u0, v0;
    uint16_t u1, v1;
    uint16_t width;
    uint16_t height;
    uint32_t flags;
};

struct CFontCache
{
    uint32_t m_bucketCount;
    uint32_t m_entryCount;
    CGlyphTexCell** m_buckets;
    uint32_t m_mask;
};

struct CFontTextureCache
{
    uint32_t m_rasterSizePadded;
    uint32_t m_pageInfo;
    uint32_t m_flags;
    float m_scaleX;
    float m_scaleY;
    CFramePoint* m_glyphs;
};

struct CFontResource
{
    char unk[36];
    FT_Face fontFace;
};

struct CFontFaceMapEntry
{
    uint32_t unk_00;
    FT_Face* fontFace;
    void* m_cache2;
};

struct CFontFaceWrapper
{
    CFontFaceMapEntry m_faceMap[16];
};

struct VertQuad { CFontVertex v[4]; };
struct IndexGroup { uint16_t idx[6]; };

struct CFontObject
{
    void* m_meta;                             // 0x00
    CFontObject* m_next;                      // 0x04
    uint32_t unk_08;                          // 0x08
    char* m_fontName;                         // 0x0C
    char* m_fontFamily;                       // 0x10
    void* m_resourceHandle;                   // 0x14
    uint32_t m_flags;                         // 0x18
    CFontCache* m_glyphCacheByCodepoint;      // 0x1C
    CFontCache* m_kerningCache;               // 0x20
    uint32_t m_fontStyle;                     // 0x24
    uint32_t m_pixelHeight;                   // 0x28
    uint32_t m_pixelWidth;                    // 0x2C
    CFontFaceWrapper* m_faceWrapper;          // 0x30
    uint32_t unk_34;                          // 0x34
    uint32_t m_glyphHashMapMask;              // 0x38
    void* m_resourceHandle2;                  // 0x3C
    void* m_scriptObject;                     // 0x40
    void* m_kerningData;                      // 0x44
    CFontTextureCache* m_textureCache;        // 0x48
    uint32_t m_texCacheResizeCounter;         // 0x4C
    uint32_t m_fontId;                        // 0x50
    uint32_t m_fontUnitHeight;                // 0x54
    void* m_cache4;                           // 0x58
    uint32_t m_atlasTextureSize;              // 0x5C
    uint32_t m_cacheHashMask;                 // 0x60
    uint32_t unk_64;                          // 0x64
    void* m_atlasManager;                     // 0x68
    void* m_fontMetrics;                      // 0x6C
    CFontResource* m_fontResource;            // 0x70
    char m_fontPath[260];                     // 0x74
    CFontTextureCache m_atlasPages[8];        // 0x178
    float m_kerningScale;                     // 0x238
    float unk_23C;                            // 0x23C
    float m_fontHeight;                       // 0x240
    uint32_t unk_244;                         // 0x244
    uint32_t m_effectivePixelHeight;          // 0x248
    uint32_t m_rasterTargetSize;              // 0x24C
};

struct CGxString
{
    CGxString* m_ll_next;                       // 0x00
    CGxString* m_ll_prev;                       // 0x04
    char unk_08[20];                            // 0x08
    float m_fontSizeMult;                       // 0x1C
    C3Vector m_anchorPos;                       // 0x20
    uint32_t m_textColor;                       // 0x2C
    uint32_t m_shadowColor;                     // 0x30
    Vec2D<float> m_shadowOffset;                // 0x34
    float m_widthBBox;                          // 0x40
    float m_heightBBox;                         // 0x44
    CFontObject* m_fontObj;                     // 0x48
    char* m_text;                               // 0x4C
    uint32_t m_text_capacity;                   // 0x50
    uint32_t m_vertAlign;                       // 0x54
    uint32_t m_horzAlign;                       // 0x58
    float m_lineSpacing;                        // 0x5C
    uint32_t m_flags;                           // 0x60
    uint32_t m_bitfield;                        // 0x64
    uint32_t m_isDirty;                         // 0x68
    int32_t m_gradientStartChar;                // 0x6C
    int32_t m_gradientLength;                   // 0x70
    C3Vector m_finalPos;                        // 0x74
    TSGrowableArray<void*> m_hyperlinks;        // 0x80
    TSGrowableArray<void*> m_embeddedTextures;  // 0x90
    char unk_A0[4];                             // 0xA0
    uint32_t m_hyperlinkClickCount;             // 0xA4
    TSGrowableArray<void*> m_gradientInfo;      // 0xA8
    CFontGeomBatch* m_geomBuffers[8];           // 0xB8
    uint32_t m_timeSinceUpdate;                 // 0xD8
    char unk_E0[20];                            // 0xDC
};


struct TextureWrapperInternal {
    uint32_t width;            // +0x00
    uint32_t height;           // +0x04
    uint8_t  reserved0[12];    // +0x08 .. +0x13
    uint32_t width_alt;        // +0x14
    uint32_t height_alt;       // +0x18
    uint8_t  reserved1[28];    // +0x1C .. +0x37
    IDirect3DTexture9* d3dTex; // +0x38
    uint8_t  reserved2[4];     // +0x3C .. +0x3F
}; // sizeof >= 64

struct TextureWrapper {
    uint8_t pad[68];                   // +0x00 .. +0x43
    TextureWrapperInternal* internal;  // +0x44
    // ...
};

struct TextureSlot {
    TextureWrapper* wrapper; // +0x00
    uint8_t padding[20];     // +0x04 .. +0x17
}; // sizeof = 24

struct TextureData {
    uint8_t pad_until_slots[396]; // +0x00 .. +0x18B
    TextureSlot slots[8];         // +0x18C
    // ... other stuff may follow
};

struct RenderContext {
    uint32_t pad0[6];                   // +0x00 .. +0x17
    TextureData* textureData;			// +0x18
    uint32_t linkedListOffset;          // +0x1C
    uint32_t pad1;                      // +0x23
    CGxString* firstString;				// +0x24
    // ...
}; // size >= 0x2C

typedef void(__thiscall* IGxuCallbackFn)(void* thisptr, int arg);
struct IGxuFontObj {
    void* vtable;               // +0x00
    uint32_t unk04;             // +0x04
    uint8_t flagsByte;          // +0x08
    uint8_t pad09[3];           // +0x09 .. +0x0B
    IGxuCallbackFn** vfuncTbl;  // +0x0C
    uint32_t nodeLinkOffset;    // +0x10
    uint32_t pad14;             // +0x14
    RenderContext* renderCtx;	// +0x18
    // ...
};


struct CacheKey {
    uint32_t sdfRenderSize = 0;
    uint32_t sdfSpread = 0;
    uint32_t d3dFormat = 0;
    bool operator==(const CacheKey& other) const {
        return sdfRenderSize == other.sdfRenderSize &&
            sdfSpread == other.sdfSpread &&
            d3dFormat == other.d3dFormat;
    }
};

struct GlyphQuad {
    uint32_t codepoint;
    uint32_t quadIndex;
    bool operator<(const GlyphQuad& other) const {
        return codepoint < other.codepoint;
    }
};

struct GlyphMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    int16_t bitmapTop = 0;
    int16_t bitmapLeft = 0;
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    uint16_t atlasPageIndex = 0;
    const uint8_t* pixelData = nullptr;
};

struct GlyphMetricsToStore {
    uint32_t codepoint = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    int16_t bitmapTop = 0;
    int16_t bitmapLeft = 0;
    std::vector<uint8_t> ownedPixelData;
    uint32_t dataSize = 0;
};

struct ThreadLocalBatch {
    std::vector<std::pair<uint32_t, GlyphMetrics>> glyphs;
    std::vector<std::vector<uint8_t>> ownedBuffers;
    std::mutex mutex;
    size_t memoryUsed = 0;
};

struct PreGenRequest {
    FT_Face face = nullptr;
    const FT_Byte* data = nullptr;
    FT_Long size = 0;
    FT_Long faceIndex = 0;
    std::string familyName;
    std::string styleName;
};

struct AtlasPage {
    IDirect3DTexture9* texture = nullptr;
    int nextX = 0, nextY = 0;
    int rowHeight = 0;
    int g = 0;
    std::vector<uint32_t> codepoints;

    AtlasPage(int gutter) : nextX(gutter), nextY(gutter), g(gutter) {}
    ~AtlasPage() { if (texture) texture->Release(); }

    void Clear() {
        nextX = g;
        nextY = g;
        rowHeight = 0;
        codepoints.clear();
    }
};