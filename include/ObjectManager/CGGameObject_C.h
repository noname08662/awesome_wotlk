#pragma once

#include "BaseTypes.h"

#include "ObjectManager/CGObject_C.h"

class CM2Model;

class CGGameObject_C : public CGObject_C {
public:
    GameObjectEntry* descriptors_;

    HOOKKIT_HOOK(getLockRec, 0x0070EF30, hookkit::Conv::eThiscall, LockRec*, CGGameObject_C*);
    HOOKKIT_HOOK(checkForPassiveHighlight, 0x00711210, hookkit::Conv::eThiscall, void, CGGameObject_C*);
    HOOKKIT_HOOK(showLootEffect, 0x0070D080, hookkit::Conv::eThiscall, void, CGGameObject_C*);
    HOOKKIT_HOOK(canUse, 0x0070BA00, hookkit::Conv::eThiscall, bool, CGGameObject_C*);
    HOOKKIT_HOOK(canUseNow, 0x0070BA10, hookkit::Conv::eThiscall, bool, CGGameObject_C*);
    HOOKKIT_HOOK(getInteractDistanceSq, 0x0070BAD0, hookkit::Conv::eThiscall, float, CGGameObject_C*);
};
