#pragma once

#include "Graphics/CGxFont.h"
#include "Graphics/CGxStringBatch.h"
#include "Heap/CDataAllocator.h"
#include "Texture/CTexture.h"

struct PLAYERNAMEDESCS {
    inline static auto& descs = *reinterpret_cast<PLAYERNAMEDESCS*>(0x00D380A8);

    CGxStringBatch* batch;
    CGxFont* font;
    CTexture* ctex;
    CDataAllocator<PLAYERNAMEDESCS> cda;
};

struct WORLDTEXTSTRING {
    virtual ~WORLDTEXTSTRING();

    unk_t _unk04;
    uint32_t type;
    float _field_0C;
    float _field_10;
    Vec3f pos;
    uint32_t color;
    uint32_t m_timestamp;
    CGxString* gx_str;
    unk_t _unk2C[4];
    char text[64];
    void* _type5_ptr;
};

static_assert(sizeof(WORLDTEXTSTRING) == 0x80);

struct PLAYERNAMEDESC {
    enum State {
        eNeedsRegen = 0x1,
        eNeedsColorUpdate = 0x2,
    };

    TSLink<PLAYERNAMEDESC> link;
    CGxString* gx_str;
    unk_t _alignment;
    guid_t guid;
    State state;
    unk_t _pad;
    WORLDTEXTSTRING* texts[4];
    float _field_30;
};
