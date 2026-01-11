#include "GameClient.h"
#include "D3D.h"
#include "Fonts.h"
#include <Detours/detours.h>
#include <map>
#include <iostream>
#include <set>

#undef min
#undef max

#include <msdfgen/msdfgen.h>
#include <msdfgen/msdfgen-ext.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include FT_GLYPH_H


// ----  if you want overkill quality, try raising these
#define SDF_SAMPLER_SLOT 13
#define ATLAS_SIZE 1024 // 512-2048
#define ATLAS_GUTTER 12 // usually spread + 2-4
#define SDF_RENDER_SIZE 64.0 // 48-128
#define SDF_SPREAD 8.0 // 6-12
#define D3DFMT D3DFMT_A8R8G8B8 // D3DFMT_A8R8G8B8-D3DFMT_A16B16G16R16
// ----


#define g_FontPixelShader (*reinterpret_cast<D3D::ShaderData**>(0x00C7D2CC))
#define g_FontVertexShader (*reinterpret_cast<D3D::ShaderData**>(0x00C7D2D0))

static IDirect3DPixelShader9* s_cachedPS = nullptr;
static IDirect3DVertexShader9* s_cachedVS = nullptr;

static FT_Library g_realFtLibrary = nullptr;

static const double FLATTEN_EPS = 0.5;
static const double EPS = 1e-9;
static const double MAX_COORD = 1e9;
static const int MIN_CONTOUR_SIZE = 3;
static const int MAX_CURVE_SAMPLES = 10;

static int MODE = 0;
static bool INITIALIZED = false;
static bool ALLOW_UNSAFE_FONTS = false; // due to how distance fields are calculated, some fonts with self-intersecting contours (e.g. diediedie) will break

static Console::CVar* s_cvar_MSDFMode;

static int CVarHandler_MSDFMode(Console::CVar* cvar, const char*, const char* value, void*)
{
    const int val = std::atoi(value);
    if (val < 0 || val > 2)
        return 0;
    return 1;
}


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
    void* m_rasterSizePadded;
    uint32_t m_pageInfo;
    uint32_t m_flags;
    float m_scaleX;
    float m_scaleY;
    void* m_glyphs;
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
    uint32_t pad1[2];                   // +0x1C .. +0x23
    CGxString* firstString;				// +0x24
    uint32_t linkedListOffset;          // +0x28
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


struct GlyphMetrics
{
    float u0, v0, u1, v1;
    int width, height;
    int bitmapLeft, bitmapTop;
    uint32_t atlasPageIndex;
};

struct AtlasPage {
    IDirect3DTexture9* texture = nullptr;
    int nextX = ATLAS_GUTTER;
    int nextY = ATLAS_GUTTER;
    int rowHeight = 0;
    std::set<uint32_t> containedGlyphs;

    ~AtlasPage() {
        if (texture) {
            texture->Release();
            texture = nullptr;
        }
    }
};

struct FontHandle
{
    msdfgen::FontHandle* msdfFont;
    FT_Face ftFace;
    const FT_Byte* fontData;
    FT_Long fontDataSize;
    bool isValid;

    std::vector<std::unique_ptr<AtlasPage>> atlasPages;
    std::map<uint32_t, GlyphMetrics> glyphCache;

    FontHandle() : msdfFont(nullptr), ftFace(nullptr), fontData(nullptr),
        fontDataSize(0), isValid(false) {
    }
    ~FontHandle() {
        if (msdfFont) {
            msdfgen::destroyFont(msdfFont);
            msdfFont = nullptr;
        }
        atlasPages.clear();
    }
};


static inline bool isnan_inf(double v) {
    return std::isnan(v) || std::isinf(v);
}

static inline bool isValidCoord(double v) {
    return !isnan_inf(v) && std::abs(v) <= MAX_COORD;
}

struct Vec {
    double x, y;

    Vec() : x(0), y(0) {}
    Vec(double x_, double y_) : x(x_), y(y_) {}

    inline double length() const { return std::hypot(x, y); }
    inline double lengthSq() const { return x * x + y * y; }
};

static double distPointToLine(const Vec& p, const Vec& a, const Vec& b) {
    const double vx = b.x - a.x;
    const double vy = b.y - a.y;
    const double wx = p.x - a.x;
    const double wy = p.y - a.y;
    const double c2 = vx * vx + vy * vy;

    if (c2 <= EPS) return std::hypot(wx, wy);

    const double t = std::clamp((wx * vx + wy * vy) / c2, 0.0, 1.0);
    const double px = a.x + t * vx;
    const double py = a.y + t * vy;

    return std::hypot(p.x - px, p.y - py);
}

static void flattenQuadratic(const Vec& p0, const Vec& p1, const Vec& p2,
    std::vector<Vec>& out, double tol, int depth = 0) {
    if (depth > 20) {
        out.push_back(p2);
        return;
    }

    const Vec m(0.25 * p0.x + 0.5 * p1.x + 0.25 * p2.x,
        0.25 * p0.y + 0.5 * p1.y + 0.25 * p2.y);

    if (distPointToLine(m, p0, p2) <= tol) {
        out.push_back(p2);
        return;
    }

    const Vec p01((p0.x + p1.x) * 0.5, (p0.y + p1.y) * 0.5);
    const Vec p12((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
    const Vec p012((p01.x + p12.x) * 0.5, (p01.y + p12.y) * 0.5);

    flattenQuadratic(p0, p01, p012, out, tol, depth + 1);
    flattenQuadratic(p012, p12, p2, out, tol, depth + 1);
}

static void flattenCubic(const Vec& p0, const Vec& p1, const Vec& p2, const Vec& p3,
    std::vector<Vec>& out, double tol, int depth = 0) {
    if (depth > 20) {
        out.push_back(p3);
        return;
    }

    const Vec m(0.125 * (p0.x + 3.0 * p1.x + 3.0 * p2.x + p3.x),
        0.125 * (p0.y + 3.0 * p1.y + 3.0 * p2.y + p3.y));

    if (distPointToLine(m, p0, p3) <= tol) {
        out.push_back(p3);
        return;
    }

    const Vec p01((p0.x + p1.x) * 0.5, (p0.y + p1.y) * 0.5);
    const Vec p12((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
    const Vec p23((p2.x + p3.x) * 0.5, (p2.y + p3.y) * 0.5);
    const Vec p012((p01.x + p12.x) * 0.5, (p01.y + p12.y) * 0.5);
    const Vec p123((p12.x + p23.x) * 0.5, (p12.y + p23.y) * 0.5);
    const Vec p0123((p012.x + p123.x) * 0.5, (p012.y + p123.y) * 0.5);

    flattenCubic(p0, p01, p012, p0123, out, tol, depth + 1);
    flattenCubic(p0123, p123, p23, p3, out, tol, depth + 1);
}

static int orient(const Vec& a, const Vec& b, const Vec& c) {
    const double v = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    return (v > EPS) - (v < -EPS);
}

static bool onSegment(const Vec& a, const Vec& b, const Vec& p) {
    if (orient(a, b, p) != 0) return false;
    return std::min(a.x, b.x) - EPS <= p.x && p.x <= std::max(a.x, b.x) + EPS &&
        std::min(a.y, b.y) - EPS <= p.y && p.y <= std::max(a.y, b.y) + EPS;
}

static bool segsIntersectProper(const Vec& a1, const Vec& a2, const Vec& b1, const Vec& b2) {
    const int o1 = orient(a1, a2, b1);
    const int o2 = orient(a1, a2, b2);
    const int o3 = orient(b1, b2, a1);
    const int o4 = orient(b1, b2, a2);

    if (o1 != o2 && o3 != o4) return true;

    if (o1 == 0 && onSegment(a1, a2, b1)) return true;
    if (o2 == 0 && onSegment(a1, a2, b2)) return true;
    if (o3 == 0 && onSegment(b1, b2, a1)) return true;
    if (o4 == 0 && onSegment(b1, b2, a2)) return true;

    return false;
}

static bool hasSelfIntersections(const std::vector<Vec>& pts) {
    const size_t n = pts.size();
    if (n < 4) return false;

    for (size_t i = 0; i < n; ++i) {
        const Vec& a1 = pts[i];
        const Vec& a2 = pts[(i + 1) % n];

        if (Vec(a2.x - a1.x, a2.y - a1.y).lengthSq() <= EPS * EPS) continue;

        for (size_t j = i + 2; j < n; ++j) {
            if (i == 0 && j == n - 1) continue;

            const Vec& b1 = pts[j];
            const Vec& b2 = pts[(j + 1) % n];

            if (Vec(b2.x - b1.x, b2.y - b1.y).lengthSq() <= EPS * EPS) continue;

            if (segsIntersectProper(a1, a2, b1, b2)) {
                const bool shareEndpoint =
                    (Vec(a1.x - b1.x, a1.y - b1.y).lengthSq() <= EPS * EPS) ||
                    (Vec(a1.x - b2.x, a1.y - b2.y).lengthSq() <= EPS * EPS) ||
                    (Vec(a2.x - b1.x, a2.y - b1.y).lengthSq() <= EPS * EPS) ||
                    (Vec(a2.x - b2.x, a2.y - b2.y).lengthSq() <= EPS * EPS);
                if (!shareEndpoint) return true;
            }
        }
    }
    return false;
}

struct DecomposeCtx {
    std::vector<std::vector<Vec>> contours;
    std::vector<Vec> current;
    Vec lastMove;
    double tol;
    DecomposeCtx() : tol(FLATTEN_EPS) {}
};

extern "C" {
    static int move_to_func(const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        if (!ctx->current.empty()) {
            ctx->contours.push_back(std::move(ctx->current));
            ctx->current.clear();
        }
        ctx->lastMove = Vec(static_cast<double>(to->x), static_cast<double>(to->y));
        ctx->current.push_back(ctx->lastMove);
        return 0;
    }

    static int line_to_func(const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        ctx->current.emplace_back(static_cast<double>(to->x), static_cast<double>(to->y));
        return 0;
    }

    static int conic_to_func(const FT_Vector* control, const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        if (ctx->current.empty()) return -1;

        const Vec& p0 = ctx->current.back();
        const Vec p1(static_cast<double>(control->x), static_cast<double>(control->y));
        const Vec p2(static_cast<double>(to->x), static_cast<double>(to->y));

        flattenQuadratic(p0, p1, p2, ctx->current, ctx->tol);
        return 0;
    }

    static int cubic_to_func(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        if (ctx->current.empty()) return -1;

        const Vec& p0 = ctx->current.back();
        const Vec p1(static_cast<double>(c1->x), static_cast<double>(c1->y));
        const Vec p2(static_cast<double>(c2->x), static_cast<double>(c2->y));
        const Vec p3(static_cast<double>(to->x), static_cast<double>(to->y));

        flattenCubic(p0, p1, p2, p3, ctx->current, ctx->tol);
        return 0;
    }
}

static bool validateOutline(const FT_Outline& outline, double tol) {
    DecomposeCtx ctx;
    ctx.tol = tol;

    FT_Outline_Funcs funcs = {};
    funcs.move_to = move_to_func;
    funcs.line_to = line_to_func;
    funcs.conic_to = conic_to_func;
    funcs.cubic_to = cubic_to_func;
    funcs.shift = 0;
    funcs.delta = 0;

    if (FT_Outline_Decompose(const_cast<FT_Outline*>(&outline), &funcs, &ctx) != 0) {
        return false;
    }
    if (!ctx.current.empty()) {
        ctx.contours.push_back(std::move(ctx.current));
    }
    for (const auto& cont : ctx.contours) {
        if (cont.size() < MIN_CONTOUR_SIZE) return false;
        for (const auto& p : cont) {
            if (!isValidCoord(p.x) || !isValidCoord(p.y)) return false;
        }
        bool hasNonDegenerateEdge = false;
        for (size_t i = 0; i < cont.size(); ++i) {
            const Vec& a = cont[i];
            const Vec& b = cont[(i + 1) % cont.size()];
            if (Vec(b.x - a.x, b.y - a.y).lengthSq() > EPS * EPS) {
                hasNonDegenerateEdge = true;
                break;
            }
        }
        if (!hasNonDegenerateEdge) return false;
        if (hasSelfIntersections(cont)) return false;
    }
    return true;
}

static std::vector<Vec> flattenMsdfContour(const msdfgen::Contour& contour, double tol) {
    std::vector<Vec> result;

    for (int i = 0; i < contour.edges.size(); ++i) {
        const msdfgen::EdgeSegment* edge = contour.edges[i];
        if (!edge) continue;

        const msdfgen::Point2 start = edge->point(0);

        if (result.empty()) {
            result.emplace_back(start.x, start.y);
        }

        std::vector<Vec> edgePoints;
        edgePoints.emplace_back(start.x, start.y);

        for (int j = 1; j <= MAX_CURVE_SAMPLES; ++j) {
            const double t = static_cast<double>(j) / MAX_CURVE_SAMPLES;
            const msdfgen::Point2 pt = edge->point(t);
            edgePoints.emplace_back(pt.x, pt.y);
        }

        bool isLinear = true;
        const Vec v0 = edgePoints.front();
        const Vec vEnd = edgePoints.back();

        for (size_t j = 1; j < edgePoints.size() - 1; ++j) {
            if (distPointToLine(edgePoints[j], v0, vEnd) > tol) {
                isLinear = false;
                break;
            }
        }

        if (isLinear) {
            result.push_back(edgePoints.back());
        }
        else {
            for (size_t j = 1; j < edgePoints.size(); ++j) {
                result.push_back(edgePoints[j]);
            }
        }
    }
    return result;
}

static bool validateResolvedShape(const msdfgen::Shape& shape, double tol) {
    if (shape.contours.empty()) return false;

    for (const msdfgen::Contour& contour : shape.contours) {
        if (contour.edges.empty()) return false;

        std::vector<Vec> pts = flattenMsdfContour(contour, tol);
        if (pts.size() < MIN_CONTOUR_SIZE) return false;

        for (const auto& p : pts) {
            if (!isValidCoord(p.x) || !isValidCoord(p.y)) {
                return false;
            }
        }

        bool hasNonDegenerateEdge = false;
        for (size_t i = 0; i < pts.size(); ++i) {
            const Vec& a = pts[i];
            const Vec& b = pts[(i + 1) % pts.size()];
            if (Vec(b.x - a.x, b.y - a.y).lengthSq() > EPS * EPS) {
                hasNonDegenerateEdge = true;
                break;
            }
        }
        if (!hasNonDegenerateEdge) return false;
        if (hasSelfIntersections(pts)) return false;
    }
    return true;
}

static bool IsGlyphValid(msdfgen::FontHandle* font, uint32_t codepoint, double tol = FLATTEN_EPS) {
    if (!font) {
        return false;
    }
    msdfgen::Shape shape;
    if (!msdfgen::loadGlyph(shape, font, codepoint))  return false;
    if (shape.contours.empty()) return true;
    msdfgen::resolveShapeGeometry(shape);
    return validateResolvedShape(shape, tol);
}

static bool IsFontMSDFCompatible(msdfgen::FontHandle* font) {
    if (!font) return false;

    for (uint32_t cp = 32; cp < 127; ++cp) {
        if (!IsGlyphValid(font, cp)) {
            return false;
        }
    }
    return true;
}


static std::map<FT_Face, std::unique_ptr<FontHandle>> g_fontHandles;
static msdfgen::FreetypeHandle* g_msdfFreetype = nullptr;

typedef int(__cdecl* FreeType_Init_t)(void* memory, FT_Library* alibrary);
typedef int(__cdecl* FreeType_NewMemoryFace_t)(FT_Library library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face* aface);
typedef int(__cdecl* FreeType_Done_Face_t)(FT_Face face);
typedef int(__cdecl* FreeType_SetPixelSizes_t)(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
typedef FT_UInt(__cdecl* FreeType_GetCharIndex_t)(FT_Face face, FT_ULong charcode);
typedef int(__cdecl* FreeType_LoadGlyph_t)(FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags);
typedef int(__cdecl* FreeType_GetKerning_t)(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning);
typedef int(__cdecl* FreeType_Done_FreeType_t)(FT_Library library);
typedef int(__cdecl* FreeType_NewFace_t)(int* library, int face_descriptor);

FreeType_Init_t FreeType_Init_orig = (FreeType_Init_t)0x00991320;
FreeType_NewMemoryFace_t FreeType_NewMemoryFace_orig = (FreeType_NewMemoryFace_t)0x00993370;
FreeType_Done_Face_t FreeType_Done_Face_orig = (FreeType_Done_Face_t)0x00992610;
FreeType_SetPixelSizes_t FreeType_SetPixelSizes_orig = (FreeType_SetPixelSizes_t)0x00992780;
FreeType_GetCharIndex_t FreeType_GetCharIndex_orig = (FreeType_GetCharIndex_t)0x009911A0;
FreeType_LoadGlyph_t FreeType_LoadGlyph_orig = (FreeType_LoadGlyph_t)0x00992DA0;
FreeType_GetKerning_t FreeType_GetKerning_orig = (FreeType_GetKerning_t)0x00991050;
FreeType_Done_FreeType_t FreeType_Done_FreeType_orig = (FreeType_Done_FreeType_t)0x00992CB0;
FreeType_NewFace_t FreeType_NewFace_orig = (FreeType_NewFace_t)0x009931A0;


static inline bool __cdecl isFontValid(FT_Face fontFace) {
    auto it = g_fontHandles.find(fontFace);
    return (it != g_fontHandles.end() && it->second->isValid);
}

static msdfgen::FontHandle* CreateMSDFGenFont(const FT_Byte* file_base, FT_Long file_size, FT_Long face_index) {
    if (!g_msdfFreetype) return nullptr;

    char tempPath[MAX_PATH];
    char tempFile[MAX_PATH];

    GetTempPathA(sizeof(tempPath), tempPath);
    sprintf_s(tempFile, sizeof(tempFile), "%smsdfgen_font_%p.tmp", tempPath, file_base);

    HANDLE hFile = CreateFileA(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    DWORD bytesWritten;
    BOOL writeResult = WriteFile(hFile, file_base, file_size, &bytesWritten, NULL);
    CloseHandle(hFile);

    if (!writeResult || bytesWritten != file_size) {
        DeleteFileA(tempFile);
        return nullptr;
    }
    msdfgen::FontHandle* handle = msdfgen::loadFont(g_msdfFreetype, tempFile);

    DeleteFileA(tempFile);

    return handle;
}

static bool CreateAtlasPage(FontHandle* fontHandle) {
    auto page = std::make_unique<AtlasPage>();
    if (!D3D::CreateTexture(&page->texture, ATLAS_SIZE, ATLAS_SIZE, 1, 0, D3DFMT, D3DPOOL_MANAGED, false)) {
        return false;
    }
    fontHandle->atlasPages.push_back(std::move(page));
    return true;
}


static int __cdecl FreeType_NewMemoryFace_hk(FT_Library library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face* aface) {
    if (!g_realFtLibrary && FT_Init_FreeType(&g_realFtLibrary) != 0) return -1;

    const int result = FT_New_Memory_Face(library, file_base, file_size, face_index, aface);
    if (result != 0 || !aface || !*aface) return result;

    FT_Face real_face = *aface;

    auto fontHandle = std::make_unique<FontHandle>();
    fontHandle->ftFace = real_face;
    fontHandle->fontData = file_base;
    fontHandle->fontDataSize = file_size;
    fontHandle->msdfFont = CreateMSDFGenFont(file_base, file_size, face_index);

    if (fontHandle->msdfFont) {
        fontHandle->isValid = (ALLOW_UNSAFE_FONTS) || IsFontMSDFCompatible(fontHandle->msdfFont);
    }

    g_fontHandles[real_face] = std::move(fontHandle);
    return result;
}

static int __cdecl FreeType_SetPixelSizes_hk(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height) {
    return FT_Set_Pixel_Sizes(face, pixel_width, pixel_height);
}

static int __cdecl FreeType_LoadGlyph_hk(FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags) {
    return FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING); // original flags CTD
}

static FT_UInt __cdecl FreeType_GetCharIndex_hk(FT_Face face, FT_ULong charcode) {
    return FT_Get_Char_Index(face, charcode);
}

static int __cdecl FreeType_GetKerning_hk(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning) {
    return FT_Get_Kerning(face, left_glyph, right_glyph, kern_mode, akerning);
}

static int __cdecl FreeType_Done_Face_hk(FT_Face face) {
    auto it = g_fontHandles.find(face);
    if (it != g_fontHandles.end()) {
        g_fontHandles.erase(it);
    }
    return FT_Done_Face(face);
}

static int __cdecl FreeType_Done_FreeType_hk(FT_Library library) {
    g_fontHandles.clear();

    if (g_msdfFreetype) {
        msdfgen::deinitializeFreetype(g_msdfFreetype);
        g_msdfFreetype = nullptr;
    }

    if (g_realFtLibrary) {
        const FT_Error error = FT_Done_FreeType(g_realFtLibrary);
        g_realFtLibrary = nullptr;
        return error;
    }

    return 0;
}

static int __cdecl FreeType_NewFace_hk(int* library, int face_descriptor_ptr) {
    return 1; // ../Fonts/* init at startup, falls back to FreeType_NewMemoryFace_hk
}


static void GenerateMSDF(std::vector<unsigned char>& out_msdf, msdfgen::FontHandle* font, uint32_t codepoint, int width, int height, FT_Face face) {
    if (!font || width <= 0 || height <= 0) {
        out_msdf.clear();
        return;
    }

    msdfgen::Shape shape;
    if (!msdfgen::loadGlyph(shape, font, codepoint)) {
        out_msdf.clear();
        return;
    }

    if (shape.contours.empty()) {
        out_msdf.resize(width * height * 4, 0);
        return;
    }

    msdfgen::resolveShapeGeometry(shape);
    msdfgen::edgeColoringInkTrap(shape, 3.0, 0);

    const msdfgen::Shape::Bounds bounds = shape.getBounds();
    const double shapeW = bounds.r - bounds.l;
    const double shapeH = bounds.t - bounds.b;

    if (shapeW <= 0.0 || shapeH <= 0.0) {
        out_msdf.resize(width * height * 4, 128);
        return;
    }

    const double usableW = static_cast<double>(width) - 2.0 * SDF_SPREAD;
    const double usableH = static_cast<double>(height) - 2.0 * SDF_SPREAD;
    const double scale = std::min(usableW / shapeW, usableH / shapeH);

    if (scale <= 0.0) {
        out_msdf.resize(width * height * 4, 128);
        return;
    }

    const msdfgen::Projection projection(
        msdfgen::Vector2(scale, scale),
        msdfgen::Vector2(SDF_SPREAD / scale - bounds.l, SDF_SPREAD / scale - bounds.b)
    );

    msdfgen::MSDFGeneratorConfig config;
    config.overlapSupport = true;

    // msdf for the contour ranged in shape units
    const msdfgen::Range msdfRange(SDF_SPREAD / scale);
    const msdfgen::SDFTransformation msdfTransform(projection, msdfRange);

    // greater range single-channel sdf for proper outlines
    const msdfgen::Range sdfRange(SDF_SPREAD / scale * 5.0);
    const msdfgen::SDFTransformation sdfTransform(projection, sdfRange);

    std::vector<float> pixels(width * height * 3, 0.0f);
    msdfgen::BitmapRef<float, 3> bitmap(pixels.data(), width, height);

    msdfgen::generateMSDF(bitmap, shape, projection, msdfRange, config);
    msdfgen::distanceSignCorrection(bitmap, shape, msdfTransform, msdfgen::FillRule::FILL_NONZERO);

    std::vector<float> sdfPixels(width * height, 0.0f);
    msdfgen::BitmapRef<float, 1> sdfBitmap(sdfPixels.data(), width, height);

    msdfgen::generateSDF(sdfBitmap, shape, projection, sdfRange);
    msdfgen::distanceSignCorrection(sdfBitmap, shape, sdfTransform, msdfgen::FillRule::FILL_NONZERO);

    out_msdf.resize(width * height * 4);
    for (int i = 0; i < width * height; ++i) {
        out_msdf[i * 4 + 0] = static_cast<unsigned char>(std::clamp(pixels[i * 3 + 0] * 255.0f, 0.0f, 255.0f));
        out_msdf[i * 4 + 1] = static_cast<unsigned char>(std::clamp(pixels[i * 3 + 1] * 255.0f, 0.0f, 255.0f));
        out_msdf[i * 4 + 2] = static_cast<unsigned char>(std::clamp(pixels[i * 3 + 2] * 255.0f, 0.0f, 255.0f));
        out_msdf[i * 4 + 3] = static_cast<unsigned char>(std::clamp(sdfPixels[i] * 255.0f, 0.0f, 255.0f));
    }
}

static const GlyphMetrics* CacheGlyphMSDF(FT_Face face, uint32_t codepoint) {
    if (!face) return nullptr;

    auto it = g_fontHandles.find(face);
    if (it == g_fontHandles.end() || !it->second->isValid) return nullptr;

    FontHandle* fontHandle = it->second.get();
    auto cacheIt = fontHandle->glyphCache.find(codepoint);
    if (cacheIt != fontHandle->glyphCache.end()) {
        return &cacheIt->second;
    }
    if (fontHandle->atlasPages.empty())
        if (!CreateAtlasPage(fontHandle)) return nullptr;

    if (FT_Set_Pixel_Sizes(face, SDF_RENDER_SIZE, SDF_RENDER_SIZE) != 0) return nullptr;
    const FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0) {
        return nullptr;
    }

    FT_BBox obox;
    FT_Outline_Get_BBox(&face->glyph->outline, &obox);
    int xMin = obox.xMin >> 6;
    int yMin = obox.yMin >> 6;
    int xMax = (obox.xMax + 63) >> 6;
    int yMax = (obox.yMax + 63) >> 6;
    int outline_width = std::max(0, xMax - xMin);
    int outline_height = std::max(0, yMax - yMin);
    int sdfWidth = (outline_width > 0) ? outline_width + 2 * SDF_SPREAD : SDF_SPREAD;
    int sdfHeight = (outline_height > 0) ? outline_height + 2 * SDF_SPREAD : SDF_SPREAD;

    int pageIndex = -1;
    AtlasPage* targetPage = nullptr;

    for (int i = 0; i < fontHandle->atlasPages.size(); ++i) {
        AtlasPage* page = fontHandle->atlasPages[i].get();
        if (page->nextX + sdfWidth + ATLAS_GUTTER <= ATLAS_SIZE &&
            page->nextY + sdfHeight + ATLAS_GUTTER <= ATLAS_SIZE) {
            pageIndex = i;
            targetPage = page;
            break;
        }
        int nextY = page->nextY + page->rowHeight + ATLAS_GUTTER;
        if (nextY + sdfHeight + ATLAS_GUTTER <= ATLAS_SIZE) {
            page->nextX = ATLAS_GUTTER;
            page->nextY = nextY;
            page->rowHeight = 0;
            pageIndex = i;
            targetPage = page;
            break;
        }
    }

    if (pageIndex == -1) {
        if (!CreateAtlasPage(fontHandle)) return nullptr;
        pageIndex = fontHandle->atlasPages.size() - 1;
        targetPage = fontHandle->atlasPages.back().get();
    }
    if (pageIndex > 8) return nullptr;

    std::vector<unsigned char> msdf_data;
    if (outline_width > 0 && outline_height > 0) {
        GenerateMSDF(msdf_data, fontHandle->msdfFont, codepoint, sdfWidth, sdfHeight, face);
    }

    if (!msdf_data.empty()) {
        D3DLOCKED_RECT lockedRect;
        if (SUCCEEDED(targetPage->texture->LockRect(0, &lockedRect, nullptr, 0))) {
            unsigned char* dest = static_cast<unsigned char*>(lockedRect.pBits) +
                targetPage->nextY * lockedRect.Pitch + targetPage->nextX * 4;
            const unsigned char* src = msdf_data.data();
            for (int y = 0; y < sdfHeight; ++y) {
                memcpy(dest, src, sdfWidth * 4);
                dest += lockedRect.Pitch;
                src += sdfWidth * 4;
            }
            targetPage->texture->UnlockRect(0);
        }
    }

    GlyphMetrics metrics = {};

    metrics.u0 = static_cast<float>(static_cast<double>(targetPage->nextX) / ATLAS_SIZE);
    metrics.v0 = static_cast<float>(static_cast<double>(targetPage->nextY) / ATLAS_SIZE);
    metrics.u1 = static_cast<float>(static_cast<double>(targetPage->nextX + sdfWidth) / ATLAS_SIZE);
    metrics.v1 = static_cast<float>(static_cast<double>(targetPage->nextY + sdfHeight) / ATLAS_SIZE);
    metrics.width = sdfWidth;
    metrics.height = sdfHeight;
    metrics.bitmapTop = yMax;
    metrics.bitmapLeft = face->glyph->bitmap_left;
    metrics.atlasPageIndex = pageIndex;

    fontHandle->glyphCache[codepoint] = metrics;

    targetPage->nextX += sdfWidth + ATLAS_GUTTER;
    targetPage->rowHeight = std::max(targetPage->rowHeight, sdfHeight);

    return &fontHandle->glyphCache.at(codepoint);
}


typedef double(__cdecl* GetFontEffectiveWidth_t)(int a1, float a2);
static GetFontEffectiveWidth_t GetFontEffectiveWidth_orig = (GetFontEffectiveWidth_t)0x006C0B60;

typedef double(__cdecl* GetFontEffectiveHeight_t)(int a1, float a2);
static GetFontEffectiveHeight_t GetFontEffectiveHeight_orig = (GetFontEffectiveHeight_t)0x006C0B20;


typedef int(__thiscall* CGxString__WriteGeometry_t)(CGxString* pThis, int destPtr, int index, int vertIndex, int vertCount);
static CGxString__WriteGeometry_t CGxString__WriteGeometry_orig = (CGxString__WriteGeometry_t)0x006C5E90;

static int __fastcall CGxString__WriteGeometry_hk(CGxString* pThis, void* edx, int destPtr, int index, int vertIndex, int vertCount) {
    const int result = CGxString__WriteGeometry_orig(pThis, destPtr, index, vertIndex, vertCount);

    auto it = g_fontHandles.find(pThis->m_fontObj->m_fontResource->fontFace);
    if (it == g_fontHandles.end() || !it->second->isValid) {
        IDirect3DDevice9* device = D3D::GetDevice();
        if (device) {
            const float resetControl[4] = { 0, 0, 0, 0 };
            device->SetPixelShaderConstantF(SDF_SAMPLER_SLOT, resetControl, 1);
        }
        return result;
    }
    FontHandle* fontHandle = it->second.get();
    if (fontHandle->atlasPages.empty() || static_cast<size_t>(index) >= fontHandle->atlasPages.size()) return result;
    if (!fontHandle->atlasPages[index] || !fontHandle->atlasPages[index]->texture) return result;

    IDirect3DDevice9* device = D3D::GetDevice();
    if (!device) return result;

    const uint32_t flags = pThis->m_fontObj->m_atlasPages[0].m_flags;
    const bool is3d = pThis->m_flags & 0x80;
    const float controlFlagSDF[4] = {
        is3d ? pThis->m_fontObj->m_rasterTargetSize : GetFontEffectiveHeight_orig(is3d, pThis->m_fontSizeMult),
        is3d ? 0.0f : ((flags & 8) ? 2.0f : ((flags & 1) ? 1.0f : 0.0f)),
        SDF_SPREAD, ATLAS_SIZE
    };

    device->SetPixelShaderConstantF(SDF_SAMPLER_SLOT, controlFlagSDF, 1);
    device->SetTexture(SDF_SAMPLER_SLOT, fontHandle->atlasPages[index]->texture);

    return result;
}


typedef void(__thiscall* IGxuFontRenderBatch_t)(IGxuFontObj* pThis);
static IGxuFontRenderBatch_t IGxuFontRenderBatch_orig = reinterpret_cast<IGxuFontRenderBatch_t>(0x006C53A0);

static void __fastcall IGxuFontRenderBatch_hk(IGxuFontObj* pThis) {
    IDirect3DDevice9* device = D3D::GetDevice();
    if (!device) {
        IGxuFontRenderBatch_orig(pThis);
        return;
    }

    // rebind every time for safety
    device->SetSamplerState(SDF_SAMPLER_SLOT, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(SDF_SAMPLER_SLOT, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetSamplerState(SDF_SAMPLER_SLOT, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(SDF_SAMPLER_SLOT, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(SDF_SAMPLER_SLOT, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

    IGxuFontRenderBatch_orig(pThis);

    // reset since font PS also handles UI elements
    const float resetControl[4] = { 0, 0, 0, 0 };
    device->SetPixelShaderConstantF(SDF_SAMPLER_SLOT, resetControl, 1);
}


typedef char(__cdecl* IGxuFontGlyphRenderGlyph_t)(FT_Face fontFace, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGlyphMetrics* resultBuffer, uint32_t outline_flag, uint32_t pad);
static IGxuFontGlyphRenderGlyph_t IGxuFontGlyphRenderGlyph_orig = (IGxuFontGlyphRenderGlyph_t)0x006C8CC0;

static char __cdecl IGxuFontGlyphRenderGlyph_hk(FT_Face fontFace, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGlyphMetrics* resultBuffer, uint32_t outline_flag, uint32_t pad) {
    const char result = IGxuFontGlyphRenderGlyph_orig(fontFace, fontSize, codepoint, pageInfo, resultBuffer, outline_flag, pad);
    if (isFontValid(fontFace)) {
        // verAdv is distance from the top of the quad to the top of the glyph, bearingY is the quad's vert correction (descenders)
        resultBuffer->m_bearingY -= resultBuffer->m_verAdv;
    }
    return result;
}


typedef CGlyphCacheEntry* (__thiscall* CGxString__GetOrCreateGlyphEntry_t)(CFontObject* fontObj, uint32_t codepoint);
static CGxString__GetOrCreateGlyphEntry_t CGxString__GetOrCreateGlyphEntry_orig = reinterpret_cast<CGxString__GetOrCreateGlyphEntry_t>(0x006C3FC0);

static CGlyphCacheEntry* __fastcall CGxString__GetOrCreateGlyphEntry_hk(CFontObject* fontObj, void* edx, uint32_t codepoint) {
    FT_Face face = fontObj->m_fontResource->fontFace;

    const GlyphMetrics* gm = CacheGlyphMSDF(face, codepoint);
    if (!gm || !isFontValid(face)) {
        fontObj->m_atlasPages[0].m_flags &= ~0x80000000;
        fontObj->m_atlasPages[0].m_flags |= (9 << 28);
        return CGxString__GetOrCreateGlyphEntry_orig(fontObj, codepoint);
    }

    // store target page idx
    fontObj->m_atlasPages[0].m_flags &= ~0xF0000000;
    fontObj->m_atlasPages[0].m_flags |= (gm->atlasPageIndex << 28);

    CGlyphCacheEntry* result = CGxString__GetOrCreateGlyphEntry_orig(fontObj, codepoint);
    if (result) {
        result->m_metrics.u0 = 1.0f + codepoint; // store codepoint
        // prevent atlas cursor from moving to ensuring it never pages before the msdf atlas does
        result->m_cellIndexMin = 0;
        result->m_cellIndexMax = 0;
        result->m_texturePageIndex = gm->atlasPageIndex; // overwrite with msdf atlas page index to ensure correct batching
    }
    return result;
}


typedef double(__thiscall* CGxString__GetBearingX_t)(CFontObject* fontObj, CGlyphCacheEntry* entry, float flag, float scale);
static CGxString__GetBearingX_t CGxString__GetBearingX_orig = reinterpret_cast<CGxString__GetBearingX_t>(0x006C24F0);

typedef int(__thiscall* CGxString__InitializeTextLine_t)(CGxString* pThis, char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7);
static CGxString__InitializeTextLine_t CGxString__InitializeTextLine_orig = reinterpret_cast<CGxString__InitializeTextLine_t>(0x006C6CD0);

static int __fastcall CGxString__InitializeTextLine_hk(CGxString* pThis, void* edx, char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7) {
    const int result = CGxString__InitializeTextLine_orig(pThis, text, textLength, a4, startPos, a6, a7);

    CFontObject* fontObj = pThis->m_fontObj;
    FT_Face fontFace = fontObj->m_fontResource->fontFace;
    if (!isFontValid(fontFace)) return result;

    const uint32_t flags = fontObj->m_atlasPages[0].m_flags;
    const bool is3d = pThis->m_flags & 0x80; // native 3d obj - nameplate text, etc.
    const double fontSizeMult = pThis->m_fontSizeMult;
    const double fontOffs = !is3d ? ((flags & 8) ? 4.5 : ((flags & 1) ? 2.5 : 0.0)) : 0.0;
    const double baselineOffs = (fontOffs > 0.0) ? 1.0 : 0.0;
    const double scale = (is3d ? fontSizeMult : GetFontEffectiveHeight_orig(is3d, fontSizeMult) * 0.98) / SDF_RENDER_SIZE; // 0.98 compensation
    const double pad = SDF_SPREAD * scale;

    for (int i = 0; i < 8; ++i) {
        CFontGeomBatch* batch = pThis->m_geomBuffers[i];
        if (!batch) continue;

        TSGrowableArray<CFontVertex>& verts = batch->m_verts;
        if (verts.m_count < 4) continue;

        for (int q = 0; q < verts.m_count; q += 4) {
            if (verts.m_data[q].u > 1) {
                const uint32_t codepoint = static_cast<uint32_t>(verts.m_data[q].u - 1.0f);

                const GlyphMetrics* gm = CacheGlyphMSDF(fontFace, codepoint);
                if (!gm) continue;

                CGlyphCacheEntry* entry = CGxString__GetOrCreateGlyphEntry_orig(fontObj, codepoint);
                if (!entry) continue;

                CFontVertex* vert0 = &verts.m_data[q + 0];
                CFontVertex* vert1 = &verts.m_data[q + 1];
                CFontVertex* vert2 = &verts.m_data[q + 2];
                CFontVertex* vert3 = &verts.m_data[q + 3];

                const double leftOffs = CGxString__GetBearingX_orig(fontObj, entry, is3d, fontSizeMult);
                const double bitmapLeft = is3d ? leftOffs : gm->bitmapLeft * scale - leftOffs;

                // no clue where this  + 1.0  comes from, but it works, I guess?..
                const double newLeft = static_cast<double>(vert0->pos.X) + (bitmapLeft != leftOffs ? bitmapLeft + 1.0 : 0.0) - pad + fontOffs * 0.5;
                const double newRight = newLeft + (gm->width * scale);

                const double newTop = static_cast<double>(vert1->pos.Y) + (gm->bitmapTop * scale) + pad - baselineOffs;
                const double newBottom = newTop - (gm->height * scale);

                vert0->pos.X = static_cast<float>(newLeft);  vert0->pos.Y = static_cast<float>(newBottom);
                vert1->pos.X = static_cast<float>(newLeft);  vert1->pos.Y = static_cast<float>(newTop);
                vert2->pos.X = static_cast<float>(newRight); vert2->pos.Y = static_cast<float>(newBottom);
                vert3->pos.X = static_cast<float>(newRight); vert3->pos.Y = static_cast<float>(newTop);

                const float u0 = gm->u0;
                const float u1 = gm->u1;
                const float v0 = gm->v0;
                const float v1 = gm->v1;

                vert0->u = u0; vert0->v = v0;
                vert1->u = u0; vert1->v = v1;
                vert2->u = u1; vert2->v = v0;
                vert3->u = u1; vert3->v = v1;
            }
        }
    }
    return result;
}


typedef CGlyphCacheEntry* (__thiscall* IGxuFontInitGlyphMetrics_t)(CFramePoint* pAtlasPage, CGlyphMetrics* resultBuffer);
static IGxuFontInitGlyphMetrics_t IGxuFontInitGlyphMetrics_orig = reinterpret_cast<IGxuFontInitGlyphMetrics_t>(0x006C9D90);

static void(*IGxuFontInitGlyphMetrics_site_orig)() = (decltype(IGxuFontInitGlyphMetrics_site_orig))0x006C4051;
static constexpr DWORD_PTR IGxuFontInitGlyphMetrics_site_jmpback = 0x006C4056;

__declspec(naked) void IGxuFontInitGlyphMetrics_site_hk()
{
    __asm {
        pushfd;
        mov esi, [edi + 180h];  // fontObj->m_atlasPages[0].m_flags
        shr esi, 28;
        and esi, 0Fh;
        cmp esi, 9;
        jz pages_match;
        cmp ebx, esi; // compare the current engine page against the current msdf page, keep the engine up to speed
        je pages_match;
        xor eax, eax; // manually page the engine side when msdf atlas fills up
        popfd;
        jmp IGxuFontInitGlyphMetrics_site_jmpback;
    pages_match:
        popfd;
        call IGxuFontInitGlyphMetrics_orig;
        jmp IGxuFontInitGlyphMetrics_site_jmpback;
    }
}


static void(*CGxString__GetGlyphYMetrics_orig)() = (decltype(CGxString__GetGlyphYMetrics_orig))0x006C8C71;
static constexpr DWORD_PTR CGxString__GetGlyphYMetrics_jmpback = 0x006C8C77;

__declspec(naked) static void CGxString_GetGlyphYMetrics_hk() // skip the orig baseline calc
{
    __asm {
        mov edx, [ecx + 54h];
        pushad;
        push ecx;
        call isFontValid;
        add esp, 4;
        test al, al;
        popad;
        jz font_unsafe;
        xor ecx, ecx;
        jmp CGxString__GetGlyphYMetrics_jmpback;
    font_unsafe:
        mov ecx, [edx + 68h];
        jmp CGxString__GetGlyphYMetrics_jmpback;
    }
}


static int __cdecl FreeType_Init_hk(void* memory, FT_Library* alibrary) {
    if (!INITIALIZED) {
        const int mode = std::atoi(s_cvar_MSDFMode->vStr);
        Console::CVar* locale_cvar = Console::GetCVar("locale");
        if (locale_cvar && locale_cvar->vStr) {
            const char* locale = locale_cvar->vStr;
            if (mode < 1 || strcmp(locale, "zhCN") == 0 || strcmp(locale, "zhTW") == 0 || strcmp(locale, "koKR") == 0) {
                return FreeType_Init_orig(memory, alibrary);
            }
        }
        else {
            return FreeType_Init_orig(memory, alibrary);
        }
        ALLOW_UNSAFE_FONTS = mode > 1;

        DetourTransactionBegin();
        DetourAttach(&(LPVOID&)FreeType_NewMemoryFace_orig, FreeType_NewMemoryFace_hk);
        DetourAttach(&(LPVOID&)FreeType_NewFace_orig, FreeType_NewFace_hk);
        DetourAttach(&(LPVOID&)FreeType_Done_Face_orig, FreeType_Done_Face_hk);
        DetourAttach(&(LPVOID&)FreeType_SetPixelSizes_orig, FreeType_SetPixelSizes_hk);
        DetourAttach(&(LPVOID&)FreeType_GetCharIndex_orig, FreeType_GetCharIndex_hk);
        DetourAttach(&(LPVOID&)FreeType_LoadGlyph_orig, FreeType_LoadGlyph_hk);
        DetourAttach(&(LPVOID&)FreeType_GetKerning_orig, FreeType_GetKerning_hk);
        DetourAttach(&(LPVOID&)FreeType_Done_FreeType_orig, FreeType_Done_FreeType_hk);

        DetourAttach(&(LPVOID&)IGxuFontRenderBatch_orig, IGxuFontRenderBatch_hk);
        DetourAttach(&(LPVOID&)IGxuFontGlyphRenderGlyph_orig, IGxuFontGlyphRenderGlyph_hk);
        DetourAttach(&(LPVOID&)IGxuFontInitGlyphMetrics_site_orig, IGxuFontInitGlyphMetrics_site_hk);

        DetourAttach(&(LPVOID&)CGxString__WriteGeometry_orig, CGxString__WriteGeometry_hk);
        DetourAttach(&(LPVOID&)CGxString__GetGlyphYMetrics_orig, CGxString_GetGlyphYMetrics_hk);
        DetourAttach(&(LPVOID&)CGxString__GetOrCreateGlyphEntry_orig, CGxString__GetOrCreateGlyphEntry_hk);
        DetourAttach(&(LPVOID&)CGxString__InitializeTextLine_orig, CGxString__InitializeTextLine_hk);
        DetourTransactionCommit();

        INITIALIZED = true;
        MODE = mode;
    }
    else if (MODE < 1) {
        return FreeType_Init_orig(memory, alibrary);
    }

    const FT_Error error = FT_Init_FreeType(&g_realFtLibrary);
    if (error) return error;

    if (alibrary) *alibrary = static_cast<FT_Library>(g_realFtLibrary);

    g_msdfFreetype = msdfgen::initializeFreetype();
    return 0;
}


void Fonts::initialize()
{
    s_cvar_MSDFMode = Console::RegisterCVar("MSDFMode", NULL, (Console::CVarFlags)1, "1", CVarHandler_MSDFMode, 0, 0, 0, 0);;
    DetourAttach(&(LPVOID&)FreeType_Init_orig, FreeType_Init_hk);

    D3D::RegisterOnDestroy([]() {
        if (s_cachedVS) { s_cachedVS->Release(); s_cachedVS = nullptr; }
        if (s_cachedPS) { s_cachedPS->Release(); s_cachedPS = nullptr; }

        for (auto& [face, handle] : g_fontHandles) {
            if (!handle) continue;
            handle->atlasPages.clear();
            handle->glyphCache.clear();
        }
        });

    D3D::RegisterPixelShaderInit([](D3D::ShaderData* shaderData) -> IDirect3DPixelShader9* {
        if (shaderData != g_FontPixelShader && g_FontPixelShader != nullptr) return nullptr;

        if (s_cachedPS) {
            if (shaderData->pixel_shader) {
                reinterpret_cast<IDirect3DPixelShader9*>(shaderData->pixel_shader)->Release();
            }
            shaderData->pixel_shader = s_cachedPS;
            s_cachedPS->AddRef();
            shaderData->compilation_flags = 1;
            return s_cachedPS;
        }

        const char* psCode = R"(
                sampler2D gameTexture : register(s0);
                sampler2D sdfAtlas    : register(s13);

                float4 control : register(c13); // font size, outline mode, spread, atlas size

                struct PS_IN {
                    float4 col : COLOR0;
                    float2 uv0 : TEXCOORD0;
                };

                float median(float r, float g, float b) {
                    return max(min(r, g), min(max(r, g), b));
                }

                float4 main(PS_IN IN) : COLOR {
                    if (control.x < 1.0f) return tex2D(gameTexture, IN.uv0) * IN.col;

                    float2 uv = IN.uv0;

                    float outlineHint = control.y;
                    float fontSize = control.x;
                    float outlinePx = 0.0f;

                    if (outlineHint >= 1.5f) {
                        outlinePx = max(2.75f, pow(fontSize, 0.150f) * 1.6f);
                    } else if (outlineHint >= 0.5f) {
                        outlinePx = max(1.50f, pow(fontSize, 0.075f) * 1.5f);
                    }
                    float4 sample = tex2D(sdfAtlas, uv);

                    float sd = median(sample.r, sample.g, sample.b);
                    float screenPxRange = (control.z / max(max(fwidth(uv.x), fwidth(uv.y)) * control.a, 1e-6)) * (1.0f - min(0.3f, fontSize * 0.0035f)); // smoother edges for larger text
                    float opacity = saturate((sd - 0.5f) * screenPxRange + 0.5f);

                    if (outlinePx > 0.0f) {
                        return float4(
                            lerp(float3(0.0f, 0.0f, 0.0f), IN.col.rgb, opacity),
                            max(opacity, saturate((sample.a - 0.5f) * screenPxRange * 5.0f + outlinePx)) * IN.col.a
                        );
                    }
                    return float4(IN.col.rgb, opacity * IN.col.a);
                }
            )";
        s_cachedPS = D3D::CompilePixelShader(psCode);

        if (s_cachedPS) {
            if (shaderData->pixel_shader) {
                reinterpret_cast<IDirect3DPixelShader9*>(shaderData->pixel_shader)->Release();
            }
            shaderData->pixel_shader = s_cachedPS;
            s_cachedPS->AddRef();
            shaderData->compilation_flags = 1;
        }
        return s_cachedPS;
        });
}
