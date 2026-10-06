#pragma once

#include "FrameScript/FrameScript.h"

class CSimpleFont;

struct FontState {
    enum PropFlags {
        eFontDirtyFont = 0x0001,
        eFontDirtyJustify = 0x0002,
        eFontDirtyColor = 0x0004,
        eFontDirtyShadow = 0x0008,
        eFontDirtySpacing = 0x0010,
        eFontDirtyAll = 0x001F,

        eFontHasFont = 0x0100,
        eFontHasJustify = 0x0200,
        eFontHasColor = 0x0400,
        eFontHasShadow = 0x0800,
        eFontHasSpacing = 0x1000,
        eFontHasAll = 0x1F00
    };

    enum TextFlags {
        // inherited
        eTextFlagJustifyLeft = 0x0001,
        eTextFlagJustifyCenter = 0x0002,
        eTextFlagJustifyRight = 0x0004,

        eTextFlagJustifyTop = 0x0008,
        eTextFlagJustifyMiddle = 0x0010,
        eTextFlagJustifyBottom = 0x0020,

        eTextFlagUnk0X40 = 0x00040,
        eTextFlagUnk0X1000 = 0x01000,
        eTextFlagUnk0X20000 = 0x20000,

        // local
        eTextFlagUnk0X80 = 0x0080,
        eTextFlagUnk0X100 = 0x0100,
        eTextFlagUnk0X200 = 0x0200,
        eTextFlagNoColorCodes = 0x00400,

        eTextFlagThickoutline = 0x00800,
        eTextFlagUnk0X2000 = 0x02000,
        eTextFlagUnk0X4000 = 0x04000,
        eTextFlagUnk0X8000 = 0x08000,
        eTextFlagUnk0X10000 = 0x10000
    };

    PropFlags update_mask;
    RCString rcstr;
    float size;
    int outline;
    float unk_float;
    TextFlags text_flags;
    Vec4u8 color;
    Vec4u8 shadow_color;
    Vec2f shadow_offset;
};

class SimpleFontObject {
public:
    virtual ~SimpleFontObject();
    virtual int onFontUpdated(FontState* state) = 0;

    CSimpleFont* font_;
    FontState::PropFlags flags_;
    TSLink<SimpleFontObject> link_;
};

class CSimpleFont : public FrameScriptObject, public SimpleFontObject {
public:
    inline static auto& total_fonts_count = *reinterpret_cast<int*>(0x00B499AC);

    int onFontUpdated(FontState* state) override;

    ~CSimpleFont() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override;                                           // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    virtual int notifySubscribers();                     // 5
    virtual bool isMatchingObjectType(const char* str);  // 6
    virtual const char* getObjectTypeStr();              // 7

    FontState state_;
    char* name_;
    unk_t _unk_5C;
    TSExplicitList<SimpleFontObject> fonts_list_;
};

static_assert(sizeof(CSimpleFont) == 0x6C);
