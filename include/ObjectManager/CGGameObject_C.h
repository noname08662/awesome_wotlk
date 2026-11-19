#pragma once

#include "BaseTypes.h"

#include "DB/DBRecords.h"
#include "M2/CM2Model.h"
#include "ObjectManager/CGObject_C.h"

class CGGameObject_C : public CGObject_C {
public:
    GameObjectEntry* descriptors_;

    enum GoFlags : uint32_t {
        eGoFlagInUse = 0x00000001,
        eGoFlagLocked = 0x00000002,
        eGoFlagNotSelectable = 0x00000010,
    };

    HOOKKIT_HOOK(getLockRec, 0x0070EF30, hookkit::Conv::eThiscall, LockRec*, CGGameObject_C*);
    HOOKKIT_HOOK(checkForPassiveHighlight, 0x00711210, hookkit::Conv::eThiscall, void, CGGameObject_C*);
    HOOKKIT_HOOK(showLootEffect, 0x0070D080, hookkit::Conv::eThiscall, void, CGGameObject_C*);
    HOOKKIT_HOOK(canUse, 0x0070BA00, hookkit::Conv::eThiscall, bool, CGGameObject_C*);
    HOOKKIT_HOOK(canUseNow, 0x0070BA10, hookkit::Conv::eThiscall, bool, CGGameObject_C*);
    HOOKKIT_HOOK(getInteractDistanceSq, 0x0070BAD0, hookkit::Conv::eThiscall, float, CGGameObject_C*);

    bool isGatheringNode() {
        const LockRec* lock = getLockRec();
        return lock != nullptr && lock->isGatheringLock();
    }

    // usable right now and of a type the interact key acts on
    bool canInteract() {
        CM2Model* model = getObjectModel();
        if (model == nullptr || model->isLoaded(0, 0) == 0) { return false; }
        switch (descriptors_->subtype) {
            case eGameobjectTypeDoor:
            case eGameobjectTypeButton:
            case eGameobjectTypeQuestgiver:
            case eGameobjectTypeChest:
            case eGameobjectTypeBinder:
            case eGameobjectTypeChair:
            case eGameobjectTypeSpellFocus:
            case eGameobjectTypeGoober:
            case eGameobjectTypeFishingnode:
            case eGameobjectTypeMailbox:
            case eGameobjectTypeMeetingstone:
            case eGameobjectTypeGuildBank:
                return canUse() && canUseNow();
            default:
                return false;
        }
    }

    // battleground flags: WSG flag stands and dropped flags, EotS tower flags, AV banners
    bool isFlag() const {
        return descriptors_->subtype == eGameobjectTypeFlagstand || descriptors_->subtype == eGameobjectTypeFlagdrop;
    }

    // A flag is never "useful" in the engine's sense: it has no lock record and its use state
    // is what changes when the flag is picked up, so canUse()/canUseNow() reject every flag
    // and the object never becomes an interaction candidate. Capturing or returning one is a
    // plain CMSG_GAMEOBJ_USE, which onRightClick() already sends, so the flag is handed to the
    // server instead: it decides between pick up, return, or nothing at all.
    bool canInteractFlag() {
        CM2Model* model = getObjectModel();
        if (model == nullptr || model->isLoaded(0, 0) == 0) { return false; }
        return (descriptors_->flags & eGoFlagNotSelectable) == 0;
    }
};
