#pragma once

#include <hookkit/hook.h>

#include <cstdint>

#include "Widget/CSimpleFrame.h"

struct CRenderBatch;

class CCooldown : public CSimpleFrame {
public:
    HOOKKIT_HOOK(onFrameRenderLayer, 0x005EBD20, hookkit::Conv::eThiscall, void, CCooldown*, CRenderBatch*, uint32_t);
};
