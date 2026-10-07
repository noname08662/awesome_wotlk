#pragma once

#include <array>

#include "Graphics/CGxShader.h"
#include "Widget/CSimpleRegion.h"

class CSimpleTexture : public CSimpleRegion {
public:
    struct CTexCoords {
        Vec2f ul;
        Vec2f ll;
        Vec2f ur;
        Vec2f lr;
    };

    enum TextureMask : uint32_t {
        eTexMaskNone = 0x0,
        eTexMaskAsyncLoad = 0x1,
        eTexMaskTexcoordsDirty = 0x2,
        eTexMaskHorizTile = 0x4,
        eTexMaskVertTile = 0x8,
        eTexMaskTileBoth = 0xC,
    };

    ~CSimpleTexture() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override;                                           // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    bool isMatchingObjectType(const char* str) override;                                         // 6
    const char* getObjectTypeStr() override;                                                     // 7
    void update() override;                                                                      // 10
    void setPosition(Vec2f* offset) override;                                                    // 17
    int setRotation(PivotMode pivot_mode, const Vec2f* pivot_offset, float radians) override;    // 18
    int setScale(PivotMode pivot_mode, const Vec2f* local_offset, const Vec2f* scale) override;  // 19
    void draw(CRenderBatch* batch) override;                                                     // 23
    void updateGeometry() override;                                                              // 24
    void onFrameChanged() override;                                                              // 25

    // CLayoutFrame
    void loadXML(XMLNode* node, CStatus* status) override;  // 1
    float getWidth() override;                              // 10
    float getHeight() override;                             // 11
    int32_t stubReturnInt32One() override;                  // 17
    void onFrameSizeChanged(Rectf rect) override;           // 18

    CTexture* ctex_;
    GxBlend blend_mode_;
    CGxShader* shader_;
    Vec3f transformed_pos_[4];
    Vec3f transformed_pos_scratch_[4];
    CTexCoords coords_;
    TextureMask mask_;

    // preserve ecx
    static constexpr hookkit::WildAbi<3> kCalcQuadAbi = {
        {{hookkit::ArgLoc::inReg(hookkit::Reg::eCx), hookkit::ArgLoc::onStack(0), hookkit::ArgLoc::onStack(4)}}};
    HOOKKIT_HOOK_WILD_HANDLE(calcQuadVertices, 0x00483220, hookkit::Conv::eUserpurge, Vec3f*, kCalcQuadAbi,
        CSimpleTexture*, const Rectf*, Vec3f*);

    HOOKKIT_HOOK(updateGeometry, 0x004834C0, hookkit::Conv::eThiscall, Vec3f*, CSimpleTexture*);
};

static_assert(sizeof(CSimpleTexture) == 0x164);
