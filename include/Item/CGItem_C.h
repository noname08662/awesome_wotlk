#pragma once

#include <cstdint>

#include "ObjectManager/CGObject_C.h"

#pragma pack(push, 4)

class CGItem_C : CGObject_C {
public:
    enum InvType : uint32_t {
        eNonEquip = 0,
        eHead = 1,
        eNeck = 2,
        eShoulder = 3,
        eBody = 4,
        eChest = 5,
        eWaist = 6,
        eLegs = 7,
        eFeet = 8,
        eWrist = 9,
        eHand = 10,
        eFinger = 11,
        eTrinket = 12,
        eWeapon = 13,
        eShield = 14,
        eRanged = 15,
        eCloak = 16,
        eWeapon2H = 17,
        eBag = 18,
        eTabard = 19,
        eRobe = 20,
        eWeaponMainHand = 21,
        eWeaponOffHand = 22,
        eHoldable = 23,
        eAmmo = 24,
        eThrown = 25,
        eRangedRight = 26,
        eQuiver = 27,
        eRelic = 28,
        eInvTypeCount = 29
    };

    int(__thiscall* get_info_by_block_id_)(CGItem_C*, int id);

    guid_t guid_;
    TSExplicitList<void> lists_[58];
    uint32_t state_;
    uint8_t item_class_;
    uint8_t item_sub_class_;
    int8_t sound_override_subclass_;
    uint8_t material_;
    uint8_t inventory_type_;
    uint8_t sheathe_type_;
    char _pad[2];
    uint32_t item_text_id_;
    uint32_t enchantments_[12];
    uint32_t enchantment_count_;
    uint32_t loaded_state_;

    // out
    HOOKKIT_HOOK_HANDLE(initLinkCtx, 0x0050F590, hookkit::Conv::eThiscall, void, void*);
    HOOKKIT_HOOK_HANDLE(getItemIdFromLink, 0x0050F630, hookkit::Conv::eThiscall, uint32_t, void*, const char*);
    HOOKKIT_HOOK(getItemInfoByName, 0x00709DE0, hookkit::Conv::eCdecl, uint32_t, const char*);
    HOOKKIT_HOOK(getInvArtById, 0x0070A910, hookkit::Conv::eCdecl, const char*, uint32_t);

    static uint32_t __fastcall getItemId(const char* link) {
        uint32_t link_context[54];
        initLinkCtx{}(link_context);
        return getItemIdFromLink{}(link_context, link);
    }

    inline static auto& kIdToStr = *reinterpret_cast<const char* const (*)[eInvTypeCount]>(0x00AC7FD8);
};

#pragma pack(pop)

static_assert(sizeof(CGItem_C) == 0x3DC);
