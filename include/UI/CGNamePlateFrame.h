#pragma once

#include <cstdint>

#include "Heap/CDataAllocator.h"
#include "Math/Primitives.h"
#include "ObjectManager/CGUnit_C.h"
#include "UI/UIEnums.h"
#include "Widget/CGSimpleHealthBar.h"
#include "Widget/CSimpleFontString.h"
#include "Widget/CSimpleFrame.h"
#include "Widget/CSimpleStatusBar.h"

class CGNamePlateFrame : public CSimpleFrame {
public:
    using NamePlateFlags = uint32_t;

    static constexpr float kDefaultHeight = 0.025f;
    static constexpr float kDefaultWidth = 0.1f;

    inline static auto& CDA = *reinterpret_cast<CDataAllocator<CGNamePlateFrame>*>(0x00DCEC44);
    inline static auto& focus = *reinterpret_cast<CGNamePlateFrame**>(0x00CA1204);
    inline static auto& max_distance_sq = *reinterpret_cast<float*>(0x00ADAA7C);

    // a CDA block that holds a constructed plate rather than a free-list link
    static bool isConstructed(const CGNamePlateFrame* block) {
        return *reinterpret_cast<const uintptr_t*>(block) == 0x00AA36F8;
    }

    struct ActiveLink {
        static TSLink<CGNamePlateFrame>& link(CGNamePlateFrame* plate) { return plate->link_active_; }

        static const TSLink<CGNamePlateFrame>& link(const CGNamePlateFrame* plate) { return plate->link_active_; }

        static CGNamePlateFrame* node(TSLink<CGNamePlateFrame>* lnk) {
            return reinterpret_cast<CGNamePlateFrame*>(
                reinterpret_cast<uintptr_t>(lnk) - offsetof(CGNamePlateFrame, link_active_));
        }
    };

    inline static auto& TSL_active = *reinterpret_cast<TSList<CGNamePlateFrame, ActiveLink>*>(0x00ADAA80);
    inline static auto& TSL_free = *reinterpret_cast<TSList<CGNamePlateFrame, ActiveLink>*>(0x00ADAA8C);

    TSLink<CGNamePlateFrame> link_active_;

    uint32_t _alignment;
    guid_t owner_guid_;

    CSimpleTexture* focus_;
    CSimpleTexture* skull_icon_;
    CSimpleTexture* raid_mark_icon_;
    CSimpleTexture* border_;
    CSimpleTexture* cast_shield_;
    CSimpleTexture* targeting_flash_;
    CSimpleTexture* elite_icon_;
    CSimpleTexture* spell_icon_;

    CSimpleFontString* name_fs_;
    CSimpleFontString* level_fs_;

    CGSimpleHealthBar* hpbar_;
    CSimpleStatusBar* castbar_;

    Vec2f ndc_proj_;
    float depth_z_;

    uint32_t plate_color_;

    uint32_t feedback_duration_;
    uint8_t is_casting_;
    uint8_t is_channeling_;
    char _pad[2];
    float cast_hold_timer_;
    float cast_fade_timer_;

    HOOKKIT_HOOK(ctor, 0x0098F790, hookkit::Conv::eThiscall, CGNamePlateFrame*, CGNamePlateFrame*, CSimpleFrame*);
    HOOKKIT_HOOK(init, 0x0098F390, hookkit::Conv::eThiscall, void, CGNamePlateFrame*, CGUnit_C*);

    HOOKKIT_HOOK(onFocusLost, 0x0098E980, hookkit::Conv::eThiscall, CGNamePlateFrame*, CGNamePlateFrame*);
    HOOKKIT_HOOK(onFocusGained, 0x0098E910, hookkit::Conv::eThiscall, CGNamePlateFrame*, CGNamePlateFrame*);

    HOOKKIT_HOOK_HANDLE(wipeCDA, 0x009DE370, hookkit::Conv::eCdecl, int);
    HOOKKIT_HOOK_HANDLE(wipeActive, 0x00727130, hookkit::Conv::eCdecl, CGNamePlateFrame*);
};
