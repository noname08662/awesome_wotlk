#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Map/CMapEntity.h"
#include "ObjectManager/Descriptors.h"
#include "ObjectManager/DescriptorsEnums.h"
#include "ObjectManager/ObjectManagerEnums.h"
#include "World/PlayerNames.h"

class CM2Model;

class CGObject_C {
public:
    template <typename T>
    T& getValue(uint32_t index) const {
        return *reinterpret_cast<T*>(&reinterpret_cast<uintptr_t*>(entry_)[index]);
    }

    template <typename T>
    void setValue(uint32_t index, const T& value) {
        *reinterpret_cast<T*>(&reinterpret_cast<uintptr_t*>(entry_)[index]) = value;
    }

    template <typename T>
    T* getEntry() const {
        return reinterpret_cast<T*>(entry_);
    }

    template <typename T>
    T* as() {
        return reinterpret_cast<T*>(this);
    }

    template <typename T>
    const T* as() const {
        return reinterpret_cast<const T*>(this);
    }

    void setValueBytes(uint32_t index, uint8_t offset, uint8_t value) const {
        if (!data_ || offset >= 4) { return; }
        uint32_t& current = data_[index];
        uint8_t current_byte = static_cast<uint8_t>((current >> (offset * 8)) & 0xFF);
        if (current_byte != value) {
            current &= ~(0xFFu << (offset * 8));
            current |= (static_cast<uint32_t>(value) << (offset * 8));
        }
    }

    virtual ~CGObject_C();                                          // 0
    virtual void disable();                                         // 1
    virtual void reenable();                                        // 2
    virtual void postReenable();                                    // 3
    virtual void handleOutOfRange();                                // 4
    virtual void updateWorldObject();                               // 5
    virtual void shouldFadeout();                                   // 6
    virtual void updateDisplayInfo(bool upd);                       // 7
    virtual void getNamePosition(Vec3f& pos);                       // 8
    virtual void getBag();                                          // 9
    virtual void getBag2();                                         // 10
    virtual Vec3f& getPosition(Vec3f& pos);                         // 11
    virtual Vec3f& getRawPosition(Vec3f& pos);                      // 12
    virtual float getFacing();                                      // 13
    virtual float getRawFacing();                                   // 14
    virtual float getScale();                                       // 15
    virtual uint64_t getTransportGuid();                            // 16
    virtual void getRotation();                                     // 17
    virtual void setFrameOfReference();                             // 18
    virtual bool isQuestGiver();                                    // 19
    virtual void refreshInteractIcon();                             // 20
    virtual void updateInteractIcon();                              // 21
    virtual void updateInteractIconAttach();                        // 22
    virtual void updateInteractIconScale();                         // 23
    virtual bool getModelFileName(const char** model_file_name);    // 24
    virtual void scaleChangeUpdate();                               // 25
    virtual void scaleChangeFinished();                             // 26
    virtual void renderTargetSelection();                           // 27
    virtual void renderPetTargetSelection();                        // 28
    virtual void render();                                          // 29
    virtual void getSelectionHighlightColor();                      // 30
    virtual float getTrueScale();                                   // 31
    virtual void modelLoaded();                                     // 32
    virtual void applyAlpha();                                      // 33
    virtual void preAnimate();                                      // 34
    virtual void animate();                                         // 35
    virtual void shouldRender();                                    // 36
    virtual float getRenderFacing();                                // 37
    virtual void onSpecialMountAnim();                              // 38
    virtual bool isSolidSelectable();                               // 39
    virtual void dummy40();                                         // 40
    virtual bool canHighlight();                                    // 41
    virtual bool canBeTargetted();                                  // 42
    virtual void floatingTooltip();                                 // 43
    virtual void onRightClick();                                    // 44
    virtual bool isHighlightSuppressed();                           // 45
    virtual void onSpellEffectClear();                              // 46
    virtual void getAppropriateSpellVisual();                       // 47
    virtual void connectToLightningThisFrame();                     // 48
    virtual void getMatrix();                                       // 49
    virtual void objectNameVisibilityChanged();                     // 50
    virtual void updateObjectNameString();                          // 51
    virtual void shouldRenderObjectName();                          // 52
    virtual CM2Model* getObjectModel();                             // 53
    virtual const char* getObjectName();                            // 54
    virtual void getPageTextId();                                   // 55
    virtual void cleanUpVehicleBoneAnimsBeforeObjectModelChange();  // 56
    virtual void shouldFadeIn();                                    // 57
    virtual float getBaseAlpha();                                   // 58
    virtual bool isTransport();                                     // 59
    virtual bool isPointInside();                                   // 60
    virtual void addPassenger();                                    // 61
    virtual float getSpeed();                                       // 62
    virtual void playSpellVisualKitPlayAnims();                     // 63
    virtual void playSpellVisualKitHandleWeapons();                 // 64
    virtual void playSpellVisualKitDelayLightningEffects();         // 65

    enum HighlightType : uint8_t {
        eHighlightTypeLockedTarget = 0,
        eHighlightTypeObjectTrack = 1,
        eHighlightTypeInteract = 2,
    };

    enum HighlightMask : uint32_t {
        eHighlightMaskLockedTarget = 1u << (eHighlightTypeLockedTarget + 24),
        eHighlightMaskObjectTrack = 1u << (eHighlightTypeObjectTrack + 24),
        eHighlightMaskInteract = 1u << (eHighlightTypeInteract + 24),
        eHighlightMaskNative = eHighlightMaskLockedTarget | eHighlightMaskObjectTrack,
        eHighlightMaskPassiveLootGlow = 1u << 22,
    };

    uint32_t* data_;
    ObjectEntry* entry_;
    unk_t _unk0C;
    uint32_t flags_;
    TypeId type_id_;
    TSHashObject<void> hash_obj_;
    unk_t _unk30[2];
    TSLink<CGObject_C> link_;
    unk_t _unk40;
    TSExplicitList<void> lists_[6];  // CMirrorHandler
    CM2Model* quest_model_;
    uint32_t quest_giver_status_;
    uint32_t taxi_status_;
    float scale_;
    unk_t _unk9C;
    uint32_t world_obj_dirty_;
    unk_t _unkA4;
    void* _unkA8;
    float height_;
    PLAYERNAMEDESC* player_name_;
    CM2Model* highlight_model_;
    CMapEntity* world_obj_;
    uint32_t highlight_mask_;
    uint32_t fade_time_start_;
    uint32_t fade_duration_;
    uint8_t cur_alpha_;
    uint8_t start_alpha_;
    uint8_t target_alpha_;
    uint8_t _unkCB;
    void* obj_ptr_;

    guid_t getGuid() const { return getValue<guid_t>(eObjectFieldGuid); }

    HOOKKIT_HOOK(showHighlightType, 0x00743C70, hookkit::Conv::eThiscall, int, CGObject_C*, CGObject_C::HighlightType);
    HOOKKIT_HOOK(hideHighlightType, 0x00743BC0, hookkit::Conv::eThiscall, int, CGObject_C*, CGObject_C::HighlightType);
    HOOKKIT_HOOK(getDistanceToPosSq, 0x004F61D0, hookkit::Conv::eThiscall, double, CGObject_C*, const Vec3f*);
};

static_assert(sizeof(CGObject_C) == 0xD0);
