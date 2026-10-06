#pragma once

#include "ObjectManager/CGUnit_C.h"

class CGObject_C;

class CGPlayer_C : public CGUnit_C {
public:
    HOOKKIT_HOOK(canTrackObj, 0x006DCA90, hookkit::Conv::eThiscall, bool, CGPlayer_C*, CGObject_C*);
};
