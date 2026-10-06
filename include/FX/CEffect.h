#pragma once

#include "BaseTypes.h"

class CEffect {
public:
    unk_t _unk[68];
};

static_assert(sizeof(CEffect) == 0x110);
