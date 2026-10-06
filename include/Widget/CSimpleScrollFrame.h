#pragma once

#include <hookkit/hook.h>

#include <cstdint>

#include "BaseTypes.h"

#include "Widget/CSimpleFrame.h"

class CSimpleScrollFrame : public CSimpleFrame {
public:
    uint32_t notified_;
    CSimpleFrame* scroll_child_;
    unk_t _unk2A4[10];

    HOOKKIT_HOOK(setScrollChild, 0x0096B3A0, hookkit::Conv::eThiscall, void, CSimpleScrollFrame*, CSimpleFrame*);
    HOOKKIT_HOOK_HANDLE(renderScrollChild, 0x0096B610, hookkit::Conv::eCdecl, void, CSimpleScrollFrame*);
};

static_assert(sizeof(CSimpleScrollFrame) == 0x2CC);
