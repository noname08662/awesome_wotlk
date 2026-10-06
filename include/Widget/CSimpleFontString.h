#pragma once

#include <cstdint>

#include "Lib/Storm.h"
#include "Widget/CSimpleFont.h"
#include "Widget/CSimpleRegion.h"

struct SIMPLEFONT {
    inline static auto& TSHT = *reinterpret_cast<TSHashTable<SIMPLEFONT>*>(0x00B499B4);

    TSHashObject<SIMPLEFONT> hash_obj;
    CSimpleFont* font;
};

static_assert(sizeof(SIMPLEFONT) == 0x1C);

class CSimpleFontString : public CSimpleRegion, public SimpleFontObject {
public:
    int onFontUpdated(FontState* state) override;

    ~CSimpleFontString() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override;                                           // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    bool isMatchingObjectType(const char* str) override;  // 6
    const char* getObjectTypeStr() override;              // 7
    void update() override;                               // 10
    void setPosition(Vec2f* offset) override;             // 17
    void onColorChanged(int is_color_changed) override;   // 21
    void nullsub() override;                              // 22
    void draw(CRenderBatch* batch) override;              // 23
    void updateGeometry() override;                       // 24
    void onFrameChanged() override;                       // 25

    // CLayoutFrame
    void loadXML(XMLNode* node, CStatus* status) override;      // 1
    bool setLayoutScale(float scale, char force_upd) override;  // 5
    bool setLayoutDepth(float depth, char force_upd) override;  // 6
    void setWidth(float width) override;                        // 7
    void setHeight(float height) override;                      // 8
    void setSize(float width, float height) override;           // 9
    float getWidth() override;                                  // 10
    float getHeight() override;                                 // 11
    int32_t stubReturnInt32One() override;                      // 17
    void onFrameSizeChanged(Rectf rect) override;               // 18

    struct HTEXTBLOCK : CHandle {
        CGxString* gx_string;
    };

    FONTHASHOBJ* simple_font_;
    float size_;
    uint16_t text_flags_;
    uint16_t text_len_;
    char* text_;
    float line_spacing_;
    HTEXTBLOCK* block_;
    float width_;
    float height_;
    Vec4u8 shadow_color_;
    Vec2f shadow_offset_;
    Vec2f pos_nudge_;
    uint32_t gradient_flags_;
    uint32_t inheritance_mask_;
    FontState::TextFlags font_flags_;
    uint32_t max_lines_;
    Vec3f anchor_;
    TSExplicitList<CSimpleRegion> regions_;
};

static_assert(sizeof(CSimpleFontString) == 0x144);
