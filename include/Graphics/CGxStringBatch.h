#pragma once

#include <cstddef>
#include <cstdint>

#include "Graphics/CGxFont.h"
#include "Lib/Storm.h"

class CGxStringBatch {
public:
    inline static auto& TSL = *reinterpret_cast<TSExplicitList<CGxStringBatch>*>(0x00AD9970);

    enum BatchFlags : uint32_t {
        eNone = 0x0,
        eWorldSpace = 0x1,
        eAutoClear = 0x2,
    };

    struct BATCHEDRENDERFONTDESC {
        TSHashObject<BATCHEDRENDERFONTDESC, CGxFont*> hash_obj;
        CGxFont* font;
        TSExplicitList<CGxString> strings_list;

        HOOKKIT_HOOK(processBatch, 0x006C4AD0, hookkit::Conv::eThiscall, void, BATCHEDRENDERFONTDESC*);
    };

    static_assert(sizeof(BATCHEDRENDERFONTDESC) == 0x28);
    static_assert(offsetof(BATCHEDRENDERFONTDESC, font) == 0x18);
    static_assert(offsetof(BATCHEDRENDERFONTDESC, strings_list) == 0x1C);

    TSLink<CGxStringBatch> free_list_;
    BatchFlags flags_;
    TSHashTable<BATCHEDRENDERFONTDESC, CGxFont*> hash_table_;

    HOOKKIT_HOOK(renderBatch, 0x006C53A0, hookkit::Conv::eThiscall, void, CGxStringBatch*);
};

static_assert(sizeof(CGxStringBatch) == 0x34);
