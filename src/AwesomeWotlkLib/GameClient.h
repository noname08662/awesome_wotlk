#pragma once
#include "Types.h"

#include <ft2build.h>
#include <d3d9.h>
#include FT_FREETYPE_H

// most of the header is either copied from somewhere else or LLM-assisted estimation
// so don't trust it blindly

// ObjectMgr
namespace ObjectMgr {
    template <typename T>
    using GetFuncPtr = T* (*)(guid_t, ETypeMask);

    inline int EnumObjects_internal(int(*func)(guid_t, void*), void* udata) {
        return (reinterpret_cast<int(*)(int(*)(guid_t, void*), void*)>(0x004D4B30))(func, udata);
    }

    template<typename F>
    bool EnumObjects(F&& func) {
    	struct Wrapper {
    		static int __cdecl callback(uint64_t guid, void* udata) {
    			auto& f = *static_cast<std::remove_reference_t<F>*>(udata);
    			return f(guid) ? 1 : 0;
    		}
    	};
        return EnumObjects_internal(&Wrapper::callback, &func) != 0;
    }
    template <typename T>
    T* Get(guid_t guid, ETypeMask flags) { return (reinterpret_cast<GetFuncPtr<T>>(0x004D4DB0))(guid, flags); }

    inline guid_t HexString2Guid(const char* str) { return (reinterpret_cast<guid_t(*)(const char*)>(0x0074D120))(str); }
    inline guid_t GetGuidByUnitID(const char* unitId) { return (reinterpret_cast<guid_t(*)(const char*)>(0x0060C1C0))(unitId); }

    inline void Guid2HexString(guid_t guid, char* buf) { return (reinterpret_cast<void(*)(guid_t, char*)>(0x0074D0D0))(guid, buf); }
    inline guid_t String2Guid(const char* str) {
        if (!str) return 0;
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) return HexString2Guid(str);
        return GetGuidByUnitID(str);
    }

    inline int UnitRightClickByGuid(guid_t guid) { return (reinterpret_cast<int(*)(guid_t)>(0x005277B0))(guid); }
    inline int UnitLeftClickByGuid(guid_t guid) { return (reinterpret_cast<int(*)(guid_t)>(0x005274F0))(guid); }

    inline void SetMouseoverByGuid(guid_t guid, guid_t prev) { return (reinterpret_cast<void(*)(guid_t, guid_t)>(0x0051F790))(guid, prev); }

    inline guid_t GetTargetGuid() { return *reinterpret_cast<guid_t*>(0x00BD07B0); }
    inline guid_t GetPlayerGuid() { return reinterpret_cast<guid_t(*)()>(0x004D3790)(); }
}

// CGame
namespace CGame {
    inline void __stdcall SetLastError(int code) {
        return (reinterpret_cast<void(__stdcall*)(int)>(0x00771870))(code);
    }
}

// CGInputControl
namespace CGInputControl {
    inline uint32_t* GetActive() {
        return *reinterpret_cast<uint32_t**>(0xC24954);
    }
}

// RCString
namespace RCString {
    inline uint32_t __stdcall hash(const char* str) {
        return (reinterpret_cast<uint32_t(__stdcall*)(const char*)>(0x0076F640))(str);
    }
}

// ClientDB
namespace ClientDB {
    using ClientDb_GetLocalizedRow = int(__thiscall*)(void* pThis, uint32_t index, void* rowBuffer);
    inline const auto GetLocalizedRow = reinterpret_cast<ClientDb_GetLocalizedRow>(0x004CFD20);

    using ClientDb_GetRow = int(__thiscall*)(void* pThis, int index);
    inline const auto GetRow = reinterpret_cast<ClientDb_GetRow>(0x004BB1C0);

    inline uintptr_t GetDbcTable(uint32_t dbIndex) {
        for (uintptr_t tableBase = 0x006337D0; *reinterpret_cast<uint8_t*>(tableBase) != 0xC3; tableBase += 0x11) {
            if (*reinterpret_cast<uint32_t*>(tableBase + 1) == dbIndex) return *reinterpret_cast<uintptr_t*>(tableBase + 0xB) + 0x18;
        }
        return 0;
    }
}

// GameUI
namespace CGGameUI {
    inline auto g_lockedTarget = reinterpret_cast<guid_t*>(0x00BD07B0);

    using Target_t = void(__cdecl*)(guid_t);
    inline auto TargetFn = reinterpret_cast<Target_t>(0x00524BF0);

    using ClearTarget_t = void(__cdecl*)(guid_t, int);
    inline auto ClearTargetFn = reinterpret_cast<ClearTarget_t>(0x005241B0);

    using WorldCallback_t = void(__fastcall*)();
    inline auto EnterWorldFn = reinterpret_cast<WorldCallback_t>(0x00528010);
    inline auto LeaveWorldFn = reinterpret_cast<WorldCallback_t>(0x00528C30);

    inline auto GetGuidByKeywordFn = reinterpret_cast<DummyCallback_t>(0x0060AFAA);

    using GetKeywordsByGuid_t = char** (*)(guid_t* guid, size_t* size);
    inline auto GetKeywordsByGuidFn = reinterpret_cast<GetKeywordsByGuid_t>(0x0060BB70);

    using GetItemIDByName_t = uint32_t(__cdecl*)(const char* name);
    inline auto GetItemIDByNameFn = reinterpret_cast<GetItemIDByName_t>(0x00709DE0);

    using HandleTerrainClick_t = void(__cdecl*)(TerrainClickEvent*);
    inline auto HandleTerrainClickFn = reinterpret_cast<HandleTerrainClick_t>(0x00527830);

    using SecureCmdOptionParse_t = int(__cdecl*)(lua_State* L);
    inline auto SecureCmdOptionParseFn = reinterpret_cast<SecureCmdOptionParse_t>(0x00564AE0);

    using CursorReleaseSpellTargeting_t = void(__cdecl*)();
    inline auto CursorReleaseSpellTargetingFn = reinterpret_cast<CursorReleaseSpellTargeting_t>(0x007FCC30);

    using DestroyPlatePool_t = int(__cdecl*)();
    inline auto DestroyPlatePoolFn = reinterpret_cast<DestroyPlatePool_t>(0x009DE370);

    using WipeActivePlates_t = guid_t* (__cdecl*)();
    inline auto WipeActivePlatesFn = reinterpret_cast<WipeActivePlates_t>(0x00727130);

    using OsGetAsyncTimeMs_t = int(__cdecl*)();
    inline auto OsGetAsyncTimeMsFn = reinterpret_cast<OsGetAsyncTimeMs_t>(0x0086AE20);

    using NDCToDDC_t = float(__cdecl*)(float ndcX, float ndcY, float *ddcX, float *ddcY);
    inline auto NDCToDDCFn = reinterpret_cast<NDCToDDC_t>(0x0047BFF0);

    using DDCToNDC_t = float(__cdecl*)(float ddcX, float ddcY, float *ndcX, float *ndcY);
    inline auto DDCToNDCFn = reinterpret_cast<DDCToNDC_t>(0x0047C020);

    using TraceLine_t = uint8_t(__cdecl*)(C3Vector*, C3Vector*, C3Vector*, float*, uint32_t, uint32_t);
    inline auto TraceLineFn = reinterpret_cast<TraceLine_t>(0x007A3B70);

    inline bool TraceLine(const C3Vector& start, const C3Vector& end, uint32_t hitFlags,
        C3Vector& intersectionPoint, float& completedBeforeIntersection) {
        completedBeforeIntersection = 1.0f;
        intersectionPoint = { .X=0.0f, .Y=0.0f, .Z=0.0f };

        uint8_t result = TraceLineFn(
            const_cast<C3Vector*>(&start),
            const_cast<C3Vector*>(&end),
            &intersectionPoint,
            &completedBeforeIntersection,
            hitFlags,
            0
        );
        if (result != 0 && result != 1) return false;

        completedBeforeIntersection *= 100.0f;
        return static_cast<bool>(result);
    }
}

// CGlueMgr
namespace CGlueMgr {
    inline auto LoadGlueXMLFn = reinterpret_cast<DummyCallback_t>(0x004DA9AC);
    inline auto LoadCharactersFn = reinterpret_cast<DummyCallback_t>(0x004E47E5);
}

// FrameScript
namespace FrameScript {
    struct Event {
        uint32_t m_hash;
        unk_t unk_04[4];
        const char* m_name;
        unk_t unk_18[12];
        uint32_t m_field48;
        uint32_t m_field4C;
        uint32_t m_field50;
    };

    struct EventList {
        size_t m_reserve;
        size_t m_size;
        Event** m_buf;
    };

    struct UnkContainer;

    using FireOnUpdate_t = int(*)(int, int, int, int);
    inline auto FireOnUpdateFn = reinterpret_cast<FireOnUpdate_t>(0x00495810);

    using FillEvents_t = void (*)(const char** list, size_t count);
    inline auto FillEventsFn = reinterpret_cast<FillEvents_t>(0x0081B5F0);

    inline UnkContainer* GetUnkContainer() { return reinterpret_cast<UnkContainer*>(0x00D3F7A8); }
    inline Event* __fastcall FindEvent(UnkContainer* This, void* edx, const char* eventName) { return (reinterpret_cast<Event * (__fastcall*)(UnkContainer*, void*, const char*)>(0x004BC410))(This, edx, eventName); }
    inline EventList* GetEventList() { return reinterpret_cast<EventList*>(0x00D3F7D0); }
    inline void FireEvent_inner(int eventId, lua_State* L, int nargs) { return (reinterpret_cast<void(*)(int, lua_State*, int)>(0x0081AA00))(eventId, L, nargs); };
    inline void vFireEvent(int eventId, const char* format, va_list args) { return (reinterpret_cast<void(*)(int, const char*, va_list)>(0x0081AC90))(eventId, format, args); }
    inline char* GetText(const char* key, int pluralIdx, int gender) { return (reinterpret_cast<char*(*)(const char*, int, int)>(0x00819D40))(key, pluralIdx, gender); }

    inline int GetEventIdByName(const char* eventName) {
        EventList* eventList = GetEventList();
        if (!eventList || eventList->m_size == 0) return -1;

        uint32_t hashValue = RCString::hash(eventName);
        for (size_t i = 0; i < eventList->m_size; i++) {
            Event* event = eventList->m_buf[i];
            if (event && event->m_hash == hashValue && (std::strcmp(event->m_name, eventName) == 0)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    inline void FireEvent(const char* eventName, const char* format, ...) {
        int eventId = GetEventIdByName(eventName);
        if (eventId == -1) return;

        va_list args;
        va_start(args, format);
        vFireEvent(eventId, format, args);
        va_end(args);
    }
}

// NetClient
namespace NetClient {
    using Login_t = void(*)(const char*, const char*);
    inline void Login(const char* login, const char* password) { return (reinterpret_cast<Login_t>(0x004D8A30))(login, password); }
}

// LoginUI
namespace LoginUI {
#pragma pack(push, 1)
    struct CharData {
        guid_t m_guid;
        char m_name[48];
        int m_map;
        int m_zone;
        int m_guildId;
        VecXYZ m_pos;
        int m_displayInfoId[23];
        int m_inventoryType[23];
        int m_enchantVisual[23];
        int m_petDisplayId;
        int m_petLevel;
        int m_petFamily;
        int m_flags;
        int m_charCustomizeFlags;
        char m_race;
        char m_class;
        char m_gender;
        char m_skin;
        char m_face;
        char m_hairStyle;
        char m_hairColor;
        char m_facialColor;
        char m_level;
        char m_firstLogin;
        char pad[6];
    };
#pragma pack(pop)
    static_assert(sizeof(CharData) == 0x188);

    struct CharVectorEntry {
        CharData m_data;
        // ...
    };

    struct CharVector {
        int m_reserved;
        int m_size;
        CharVectorEntry* m_buf;
        int m_fieldC;
    };

    inline CharVector* GetChars() { return reinterpret_cast<CharVector*>(0x00B6B238); }

    inline void SelectCharacter(int idx) {
        *reinterpret_cast<int*>(0x00AC436C) = idx;
        (reinterpret_cast<void(*)()>(0x004E3CD0))();
    }

    inline void EnterWorld(int idx) {
        *reinterpret_cast<int*>(0x00B499A4) = *reinterpret_cast<int*>(0x00B1D618);
        *reinterpret_cast<int*>(0x00AC436C) = idx;
        (reinterpret_cast<void(*)()>(0x004D9BD0))();
    }
}

class CSimpleTop;
class CSimpleFrame;
class CGNamePlate;
class CSimpleCamera;
class CGWorldFrame;

// CGObject_C
class CGObject_C {
public:
    template <typename T>
    T& GetValue(uint32_t index) const { return *reinterpret_cast<T*>(&reinterpret_cast<uintptr_t*>(m_entry)[index]); }

    template <typename T>
    T* GetEntry() const { return reinterpret_cast<T*>(m_entry); }

    template <typename T>
    T* As() { return reinterpret_cast<T*>(this); }

    template <typename T>
    const T* As() const { return reinterpret_cast<const T*>(this); }

    void SetValueBytes(uint32_t index, uint8_t offset, uint8_t value) const {
        if (!m_data || offset >= 4) return;
        uint32_t& current = m_data[index];
        uint8_t currentByte = static_cast<uint8_t>((current >> (offset * 8)) & 0xFF);
        if (currentByte != value) {
            current &= ~(0xFFu << (offset * 8));
            current |= (static_cast<uint32_t>(value) << (offset * 8));
        }
    }

    virtual ~CGObject_C(); // 0
    virtual void Disable(); // 1
    virtual void Reenable(); // 2
    virtual void PostReenable(); // 3
    virtual void HandleOutOfRange(); // 4
    virtual void UpdateWorldObject(); // 5
    virtual void ShouldFadeout(); // 6
    virtual void UpdateDisplayInfo(); // 7
    virtual void GetNamePosition(C3Vector& pos); // 8
    virtual void GetBag(); // 9
    virtual void GetBag2(); // 10
    virtual C3Vector& GetPosition(C3Vector& pos); // 11
    virtual C3Vector& GetRawPosition(C3Vector& pos); // 12
    virtual float GetFacing(); // 13
    virtual float GetRawFacing(); // 14
    virtual float GetScale(); // 15
    virtual uint64_t GetTransportGUID(); // 16
    virtual void GetRotation(); // 17
    virtual void SetFrameOfReference(); // 18
    virtual bool IsQuestGiver(); // 19
    virtual void RefreshInteractIcon(); // 20
    virtual void UpdateInteractIcon(); // 21
    virtual void UpdateInteractIconAttach(); // 22
    virtual void UpdateInteractIconScale(); // 23
    virtual bool GetModelFileName(char const** modelFileName); // 24
    virtual void ScaleChangeUpdate(); // 25
    virtual void ScaleChangeFinished(); // 26
    virtual void RenderTargetSelection(); // 27
    virtual void RenderPetTargetSelection(); // 28
    virtual void Render(); // 29
    virtual void GetSelectionHighlightColor(); // 30
    virtual float GetTrueScale(); // 31
    virtual void ModelLoaded(); // 32
    virtual void ApplyAlpha(); // 33
    virtual void PreAnimate(); // 34
    virtual void Animate(); // 35
    virtual void ShouldRender(); // 36
    virtual float GetRenderFacing(); // 37
    virtual void OnSpecialMountAnim(); // 38
    virtual bool IsSolidSelectable(); // 39
    virtual void Dummy40(); // 40
    virtual bool CanHighlight(); // 41
    virtual bool CanBeTargetted(); // 42
    virtual void FloatingTooltip(); // 43
    virtual void OnRightClick(); // 44
    virtual bool IsHighlightSuppressed(); // 45
    virtual void OnSpellEffectClear(); // 46
    virtual void GetAppropriateSpellVisual(); // 47
    virtual void ConnectToLightningThisFrame(); // 48
    virtual void GetMatrix(); // 49
    virtual void ObjectNameVisibilityChanged(); // 50
    virtual void UpdateObjectNameString(); // 51
    virtual void ShouldRenderObjectName(); // 52
    virtual void GetObjectModel(); // 53
    virtual const char* GetObjectName(); // 54
    virtual void GetPageTextID(); // 55
    virtual void CleanUpVehicleBoneAnimsBeforeObjectModelChange(); // 56
    virtual void ShouldFadeIn(); // 57
    virtual float GetBaseAlpha(); // 58
    virtual bool IsTransport(); // 59
    virtual bool IsPointInside(); // 60
    virtual void AddPassenger(); // 61
    virtual float GetSpeed(); // 62
    virtual void PlaySpellVisualKit_PlayAnims(); // 63
    virtual void PlaySpellVisualKit_HandleWeapons(); // 64
    virtual void PlaySpellVisualKit_DelayLightningEffects(); // 65

    uint32_t* m_data;
    ObjectEntry* m_entry;
    uint32_t m_fieldC;
    uint32_t m_field10;
    ETypeID m_typeID;
    uint32_t m_spline[29];
    CM2Model* m_childModel;
    unk_t unk_90;
    float m_trueScale;
    float m_scaleX;
    float m_renderScale;
    unk_t unk_A0;
    unk_t unk_A4;
    uint32_t m_highlightTypes;
    float m_objectHeight;
    uint32_t m_flags;
    CM2Model* m_model;
    void* m_worldObject;
    uint32_t m_highlightMask;

    float GetDistance(CGObject_C* secObj) {
        if (!secObj) return 0.0f;

        C3Vector a{}, b{};
        secObj->GetPosition(a);
        this->GetPosition(b);

        float dx = b.X - a.X;
        float dy = b.Y - a.Y;
        float dz = b.Z - a.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
};

// CGUnit_C
class CGUnit_C : public CGObject_C {
public:
    unk_t unk_C0[4];                        // 0xC0
    FactionTemplateRec* m_factionTemplate;  // 0xD0
    unk_t unk_D4[53];                       // 0xD4
    C44Matrix m_transform;                  // 0x1A8
    C3Vector m_selectionCenter;             // 0x1E8
    float m_selectionRadius;                // 0x1F4
    unk_t unk_1F8[117];                     // 0x1F8
    BattlegroundData* m_bgData;             // 0x3CC
    unk_t unk_3D0[289];                     // 0x3D0
    float m_unitHeight;                     // 0x854
    unk_t unk_858[67];                      // 0x858
    CreatureCache* m_creaturecache;         // 0x964
    unk_t unk_968[78];                      // 0x968
    float m_cameraFacing;                   // 0xAA0
    unk_t unk_AA4[101];                     // 0xAA4
    CGNamePlate* m_nameplate;               // 0xC38

    using GetCreatureRank_t = ECreatureRank(__thiscall*)(const CGObject_C*);
    inline static const auto GetCreatureRankFn = reinterpret_cast<GetCreatureRank_t>(0x00718A00);

    using CanAssist_t = bool(__thiscall*)(const CGUnit_C*, const CGUnit_C*, bool ignoreFlags);
    inline static const auto CanAssistFn = reinterpret_cast<CanAssist_t>(0x007293D0);

    using UnitReaction_t = EUnitReaction(__thiscall*)(const CGUnit_C*, const CGUnit_C*);
    inline static const auto UnitReactionFn = reinterpret_cast<UnitReaction_t>(0x007251C0);

    using CanAttack_t = bool(__thiscall*)(const CGUnit_C*, const CGUnit_C*);
    inline static const auto CanAttackFn = reinterpret_cast<CanAttack_t>(0x00729A70);

    using HideNamePlate_t = CGNamePlate*(__thiscall*)(CGUnit_C*);
    inline static auto HideNamePlateFn = reinterpret_cast<HideNamePlate_t>(0x00725840);

    using UpdateReaction_t = int(__thiscall*)(CGUnit_C*, int updateAll);
    inline static auto UpdateReactionFn = reinterpret_cast<UpdateReaction_t>(0x0071F8F0);

    using SetNamePlateFocus_t = void(__cdecl*)(C3Vector* pos);
    inline static auto SetNamePlateFocusFn = reinterpret_cast<SetNamePlateFocus_t>(0x007271D0);

    using GetName_t = const char*(__thiscall*)(CGUnit_C*, void*, int);
    inline static auto GetNameFn = reinterpret_cast<GetName_t>(0x0072A000);

    using GetMissileTargetPosition_t = int(__thiscall*)(CGUnit_C*, C3Vector*);
    inline static auto GetMissileTargetPositionFn = reinterpret_cast<GetMissileTargetPosition_t>(0x0071A720);

    using IsVisible_t = bool(__thiscall*)(CGUnit_C*, CGWorldFrame*, C3Vector*);
    inline static auto IsVisibleFn = reinterpret_cast<IsVisible_t>(0x00715720);

    guid_t GetGUID() const { return GetValue<guid_t>(OBJECT_FIELD_GUID); }
    const char* GetName(void* ptr, int flag) { return GetNameFn(this, ptr, flag); }
    ECreatureRank GetCreatureRank() const { return GetCreatureRankFn(this); }
    bool CanAttack(const CGUnit_C* unit) const { return CanAttackFn(this, unit); }
    bool CanAssist(const CGUnit_C* unit, bool ignoreFlags) const { return CanAssistFn(this, unit, ignoreFlags); }
    EUnitReaction UnitReaction(const CGUnit_C* unit) const { return UnitReactionFn(this, unit); }
    int UpdateReaction(int updateAll) { return UpdateReactionFn(this, updateAll); }
    int GetMissileTargetPosition(C3Vector* out) { return GetMissileTargetPositionFn(this, out); }
    CGNamePlate* HideNamePlate() { return HideNamePlateFn(this); }
    bool IsVisible(CGWorldFrame* wf, C3Vector* out) { return IsVisibleFn(this, wf, out); }
    bool IsFriendly(const CGUnit_C* player = ObjectMgr::Get<CGUnit_C>(ObjectMgr::GetPlayerGuid(), TYPEMASK_PLAYER)) const {
        if (!player) return false; int reaction = player->UnitReaction(this);
        return (reaction >= 5) || (reaction == 4 && !player->CanAttack(this));
    }
    static void SetNamePlateFocus(C3Vector* pos) { return SetNamePlateFocusFn(pos); }
};

// CGPlayer_C
class CGPlayer_C : public CGUnit_C {
public:
    using NotifyCombatChange_t = void(__thiscall*)(CGUnit_C*, int, int);
    inline static auto NotifyCombatChangeFn = reinterpret_cast<NotifyCombatChange_t>(0x0055E550);

    void NotifyCombatChange(int offs, int val) { return NotifyCombatChangeFn(this, offs, val); }
};

// CGGameObject_C
class CGGameObject_C : public CGObject_C {
public:
    bool CanUseNow() const { return CanUseNowFn(this); }

private:
    using CanUseNow_t = bool(__thiscall*)(const CGGameObject_C*);
    inline static const auto CanUseNowFn = reinterpret_cast<CanUseNow_t>(0x0070BA10);
};


// Frame Interface
class FrameScript_Object {
public:
    virtual ~FrameScript_Object();
    virtual void unk_pure1();
    virtual void unk_pure2();
    virtual char* GetScriptByName(char* name, char** out);
    virtual void unk_pure3();

    int m_field_04;
    int m_field_08;
    int m_field_0C;
    int m_field_10;

    using RegisterScriptObject_t = void(__thiscall*)(FrameScript_Object*, const char*);
    inline static auto RegisterScriptObjectFn = reinterpret_cast<RegisterScriptObject_t>(0x00819880);

    void __fastcall RegisterScriptObject(const char* type) { RegisterScriptObjectFn(this, type); }
};

class CLayoutFrame {
public:
    struct CFramePoint {
        Vec2D<float> m_pos;
        CLayoutFrame* m_layout;
        uint32_t m_flags;
    };

    struct CLayoutChildNode {
        CLayoutChildNode* m_prev;
        CLayoutChildNode* m_next;
        CLayoutFrame* m_child;
        unk_t unk_0C;
    };

	enum EHitboxAnchor : uint8_t {
		HB_TOP = 0,
		HB_CENTER = 1,
		HB_BOTTOM = 2,
	};

    virtual ~CLayoutFrame();
    virtual void LoadXML(void* node, void* status);
    virtual void* GetLayoutParent();
    virtual void unk_null();
    virtual int GetPageTextID(int index);
    virtual void SetLayoutScale(float scale, bool force);
    virtual void SetLayoutDepth(float depth, bool force);
    virtual void SetWidth(float width);
    virtual void SetHeight(float height);
    virtual void SetSize(float w, float h);
    virtual float GetWidth();
    virtual float GetHeight();
    virtual void GetSize(float* w, float* h, bool skipUpdate);
    virtual void GetClampRectInsets(float* l, float* r, float* t, float* b);
    virtual int stub_zero();
    virtual bool CanBeAnchorFor(CLayoutFrame* other);
    virtual void sub151();
    virtual int stub_one();
    virtual void OnFrameSizeChanged(int size);

    unk_t unk_04;                   // 0x04
    unk_t unk_08;                   // 0x08
    CFramePoint* m_framePoints[9];  // 0x0C - Anchors (Top, Bottom, Center, etc.)
    unk_t unk_30;                   // 0x30
    CLayoutChildNode* m_childunk;   // 0x34
    CLayoutChildNode* m_children;   // 0x38
    unk_t unk_3C;                   // 0x3C
    uint32_t m_flags;               // 0x40
    float m_bottom;                 // 0x44
    float m_left;                   // 0x48
    float m_top;                    // 0x4C
    float m_right;                  // 0x50
    float m_width;                  // 0x54
    float m_height;                 // 0x58
    float m_scale;                  // 0x5C
    float m_depth;                  // 0x60
    float m_minResizeY;             // 0x64
    float m_minResizeX;             // 0x68
    float m_maxResizeY;             // 0x6C
    float m_maxResizeX;             // 0x70

    using IsAtTargetPos_t = bool(__thiscall*)(CLayoutFrame*, C3Vector*);
    inline static auto IsAtTargetPosFn = reinterpret_cast<IsAtTargetPos_t>(0x489270);

    using SetPoint_t = void(__thiscall*)(CLayoutFrame*, int, CLayoutFrame*, int, float, float, int);
    inline static auto SetPointFn = reinterpret_cast<SetPoint_t>(0x0048A260);

    void SetPoint(int point, CLayoutFrame* relativeTo, int relativePoint, float xOffset, float yOffset, int doResize) {
        SetPointFn(this, point, relativeTo, relativePoint, xOffset, yOffset, doResize);
    }
    static int ResizePending() { return reinterpret_cast<int(__cdecl*)()>(0x004898B0)(); }

	bool IsAtTargetPos(const C3Vector* pos, Vec2D<float> percs, EHitboxAnchor anchor) const {
    	float yMin, yMax;
    	switch (anchor) {
    	case HB_TOP:
    		yMax = this->m_top;
    		yMin = this->m_top - this->m_height * percs.y;
    		break;
    	case HB_BOTTOM:
    		yMin = this->m_bottom;
    		yMax = this->m_bottom + this->m_height * percs.y;
    		break;
    	case HB_CENTER:
    	default:
    		const float midY = (this->m_bottom + this->m_top) * 0.5f;
    		const float halfHeight = this->m_height * percs.y * 0.5f;
    		yMin = midY - halfHeight;
    		yMax = midY + halfHeight;
    		break;
    	}
    	const float midX = (this->m_left + this->m_right) * 0.5f;
    	const float halfWidth = (this->m_width * 0.5f) * percs.x;
    	return (pos->X >= (midX - halfWidth) && pos->X <= (midX + halfWidth)) && (pos->Y >= yMin && pos->Y <= yMax);
    }
};
static_assert(sizeof(CLayoutFrame) == 0x74);

class CSimpleTop : public CLayoutFrame {
public:
    unk_t unk_74[1145];         // 0x74
};

class RCString_Object {
public:
    virtual int GetName();
    virtual void unk_pure1();
    virtual int* GetScriptByName(char* mame, const char** out);
    virtual bool IsA(int a);
    virtual void unk_pure2();
    virtual bool IsA_0(char* name);
    virtual char* GetObjectTypeName(int a);
    unk_t unk_04;
};

class CScriptObject : public FrameScript_Object {
public:
    RCString_Object m_name;
    int m_nameId;
};

class CScriptRegion : public CScriptObject, public CLayoutFrame {
public:
    CSimpleFrame* parent;           // 0x94
    unk_t unk_98[2];                // 0x98
};

// CSimpleFrame
class CSimpleFrame : public CScriptRegion {
public:
    struct CBackdropGenerator {
        unk_t unk_00[27];
    };

    struct CSimpleFrameNode {
        CSimpleFrameNode* prev;
        CSimpleFrameNode* next;
        CSimpleFrame* frame;
    };

    CSimpleTop* m_simpleTop;            // 0xA0
    unk_t unk_A4;                       // 0xA4
    CScriptRegion* m_titleRegion;       // 0xA8
    unk_t unk_AC;                       // 0xAC
    unk_t unk_B0;                       // 0xB0
    EFrameState m_stateFlags;           // 0xB4
    unk_t unk_B8;                       // 0xB8
    uint8_t m_alpha;                    // 0xBC
    unk_t unk_C0[5];                    // 0xC0
    uint32_t m_frameLevel;              // 0xD4
    uint32_t m_registeredEvents;        // 0xD8
	uint32_t m_isShown;                 // 0xDC
    uint32_t m_isLinked;                // 0xE0
    float m_clampInsets[4];             // 0xE4 (Left, m_top, m_right, m_bottom)
    unk_t unk_F4[69];                   // 0xF4
    CBackdropGenerator* m_bdGen;        // 0x208
    unk_t unk_20C[21];                  // 0x20C
    void* m_childrenList;               // 0x260
    unk_t unk_264[5];                   // 0x264
    void* m_parentListSentinel;         // 0x278
    CSimpleFrameNode m_parentLink;      // 0x27C
    unk_t unk_288[5];                   // 0x288

	template <typename F>
	void EnumerateChildren(F callback) {
		CSimpleFrameNode* current = this->m_parentLink.next;
		while (current && (reinterpret_cast<uintptr_t>(current) & 1) == 0) {
			if (current->frame) callback(current->frame);
			current = current->next;
		}
	}
	
    void SetFrameDepth(float depth, int flag) {
        reinterpret_cast<void(__thiscall*)(CSimpleFrame*, float, int)>(0x0048F5D0)(this, depth, flag);
    }
    void SetFrameLevel(uint32_t level, int levelChildren) {
        reinterpret_cast<void(__thiscall*)(CSimpleFrame*, uint32_t, int)>(0x004910A0)(this, level, levelChildren);
    }
    void SetAlpha(uint8_t alpha) {
        reinterpret_cast<void(__thiscall*)(CSimpleFrame*, uint8_t)>(0x0048EA10)(this, alpha);
    }
    int Hide() { return reinterpret_cast<int(__thiscall*)(CSimpleFrame*)>(0x0048F620)(this); }
    int GetRefTable() { return (reinterpret_cast<int(__thiscall*)(CSimpleFrame*)>(0x00488380))(this); }

    static int GetObjectType() { return *reinterpret_cast<int*>(0x00B49984); }
};
static_assert(sizeof(CSimpleFrame) == 0x29C);

class CSimpleCamera {
public:
    struct M2Scene {
        unk_t unk[82];
    };
    static_assert(sizeof(M2Scene) == 0x148);

    virtual double GetFov();
    virtual C3Vector* GetForwardVector(C3Vector* out);
    virtual C3Vector* GetRightVector(C3Vector* out);
    virtual C3Vector* GetUpVector(C3Vector* out);

    M2Scene* m_m2scene;     // 0x04
    C3Vector m_pos;         // 0x08
    C33Matrix m_matrix;     // 0x14 (fwd, right, up)
    float m_farClip;        // 0x38
    float m_nearClip;       // 0x3C
    float m_fov;            // 0x40
    float m_aspect;         // 0x44

    using constructor_t = char* (__thiscall*)(CSimpleCamera*, float a2, float a3, float fov);
    inline static auto constructorFn = reinterpret_cast<constructor_t>(0x00607C20);

    char* constructor(float a2, float a3, float fov) { return constructorFn(this, a2, a3, fov); }
};
static_assert(sizeof(CSimpleCamera) == 0x48);

class CGCamera : public CSimpleCamera {
public:
    enum ECamFlags3 : uint32_t {
        CAM3_FOLLOW_FACING = 0x00000001,
        CAM3_MODEL_CAMERA = 0x00000004,
        CAM3_VIEW_LOCKED = 0x00000008,
        CAM3_HAS_TARGET = 0x00000010,
        CAM3_DISABLE_INPUT = 0x00000020,
        CAM3_MOUSE_HELD = 0x00000040,
        CAM3_CLICK_TO_MOVE = 0x00000100,
        CAM3_FREELOOK_ACTIVE = 0x00000200,
        CAM3_CLICK_TO_MOVE2 = 0x00000400,
        CAM3_INHERIT_YAW = 0x00001000,
        CAM3_DISABLE_FOLLOW = 0x00004000,
        CAM3_DISABLE_SMOOTHING = 0x00008000,
        CAM3_IS_MOVING = 0x00010000,
        CAM3_CAN_PIVOT = 0x00020000,
        CAM3_CAMERA_BELOW = 0x00100000,
        CAM3_CAMERA_ABOVE = 0x00200000,
        CAM3_FOV_ALT_BLEND = 0x00400000,
        CAM3_TARGET_CHANGED = 0x00800000,
        CAM3_TRANSITION_T2 = 0x01000000,
        CAM3_PITCH_LIMIT_BLEND = 0x02000000,
        CAM3_IS_COLLIDING = 0x04000000,
        CAM3_PITCH_SMOOTH_ACTIVE = 0x08000000,
        CAM3_SMOOTH_OVERRIDE = 0x10000000,
        CAM3_YAW_BLEND_ACTIVE = 0x20000000,
        CAM3_FOV_BLEND_ACTIVE = 0x40000000,
        CAM3_TELESCOPE_ACTIVE = 0x80000000,
    };

    enum ECamMode : uint8_t {
        CAMMODE_AUTO_INTERACT = 0x00000001,
        CAMMODE_HAS_TARGET = 0x00000002,
        CAMMODE_FACING_LOCKED = 0x00000004,
        CAMMODE_ALT_ZOOM = 0x00000020,
        CAMMODE_ZOOM_LOCKED = 0x00000040,
    };

    enum EZoomState : uint8_t {
        CAMZOOM_ZOOM_IN_ACTIVE = 0x00000001,
        CAMZOOM_ZOOM_IN_INTERRUPTED = 0x00000002,
        CAMZOOM_ZOOM_OUT_ACTIVE = 0x00000004,
        CAMZOOM_ZOOM_OUT_INTERRUPTED = 0x00000008,
    };

    struct ViewEntry {
        float pitchLimit;
        float zoomDist;
        float facing;
    };

    struct CameraShake {
        uintptr_t next;
        uintptr_t prev;
        int type;
        int channel;        // axis index 0-2
        float amplitude;
        float param1;
        float duration;     // lifetime
        float phaseOffset;
        float param4;
        C3Vector dir;
        uint32_t startTimeMs;
    };
    static_assert(sizeof(CameraShake) == 0x34);

    CM2Model* m_model;                  // 0x48
    uint32_t m_timestamp;               // 0x4C
    uint32_t m_modelHandle;             // 0x50
    C33Matrix m_modelCamMatrix;         // 0x54
    C3Vector m_targetPos;               // 0x78
    float unk_84;                       // 0x84
    guid_t m_targetGuid;                // 0x88

    uint32_t m_flags;                   // 0x90
    uint32_t m_flags2;                  // 0x94
    ECamFlags3 m_flags3;                // 0x98
    ECamMode m_camModeFlags;            // 0x9C

    guid_t m_relativeToGuid;            // 0xA0
    float m_terrainTiltAngle;           // 0xA8
    uint32_t m_targetRetainCount;       // 0xAC
    uint32_t m_currentViewIndex;        // 0xB0
    uint32_t m_currentViewSlot;         // 0xB4
    float m_currentViewPitchLimit;      // 0xB8
    float m_currentViewFacing;          // 0xBC
    ViewEntry m_viewTable[6];           // 0xC0
    float m_commentatorYaw;             // 0x108
    C3Vector m_modelCamOffset;          // 0x10C - {zoom, pitch, 0}

    float m_zoomDistance;               // 0x118
    float m_facingSmooth;               // 0x11C - animated
    float m_pitchLimitMax;              // 0x120 - animated, collision-clamped
    float m_pitchLimitMin;              // 0x124
    float m_yaw;                        // 0x128 - animated
    float m_pivotAccum;                 // 0x12C
    float m_pitchOffset;                // 0x130 - (actual pitch - m_pitchLimitMax)
    float m_savedFirstPersonPitch;      // 0x134
    float m_fovCorrection;              // 0x138
    float m_fovCorrectionTarget;        // 0x13C
    C3Vector m_shakeBias;               // 0x140
    float m_pitchLimits[3];             // 0x14C - per-axis height bounds

    unk_t unk_158[2];                   // 0x158
    EZoomState m_zoomStateFlags;        // 0x160
    uint32_t m_zoomStartInOutMs[2];     // 0x164
    unk_t unk_16C;                      // 0x16C
    unk_t unk_170[3];                   // 0x170
    uint32_t m_zoomOutStartMs;          // 0x17C
    uint32_t m_zoomInStartMs;           // 0x180
    unk_t unk_184;                      // 0x184
    unk_t unk_188[2];                   // 0x188
    unk_t unk_190;                      // 0x190
    uint32_t m_zoomInEnd;               // 0x194
    uint32_t m_zoomOutEnd;              // 0x198
    unk_t unk_19C[3];                   // 0x19C
    float m_pendingZoomOutDelta;        // 0x1A8
    float m_zoomSpeed[2];               // 0x1AC
    unk_t unk_1B4[4];                   // 0x1B4

    float m_cachedTargetX;              // 0x1C4
    float m_cachedTargetY;              // 0x1C8
    float m_cachedTargetZ;              // 0x1CC
    float m_cachedSomething;            // 0x1D0
    float m_cachedFacing;               // 0x1D4
    uint32_t m_terrainTiltLastUpdateMs; // 0x1D8

    float m_savedFacing;                // 0x1DC - freelook start

    uint32_t m_zoomBlendStartMs;        // 0x1E0
    float m_zoomBlendDuration;          // 0x1E4
    float m_zoomBlendFrom;              // 0x1E8 - source 
    float m_zoomBlendTo;                // 0x1EC - destination
    float m_zoomLimitBlendEnd;          // 0x1F0
    float m_zoomLimitBlendStart;        // 0x1F4

    uint32_t m_smoothOverrideStartMs;   // 0x1F8
    float m_smoothOverrideDuration;     // 0x1FC
    float m_smoothOverrideFrom;         // 0x200
    float m_smoothOverrideTo;           // 0x204
    float m_smoothOverrideEnd;          // 0x208
    float m_smoothOverrideTime;         // 0x20C

    uint32_t m_yawBlendStartMs;         // 0x210
    float m_yawBlendDuration;           // 0x214
    float m_yawBlendFrom;               // 0x218
    float m_yawBlendTo;                 // 0x21C
    float m_yawBlendEnd;                // 0x220 - 1.0 default
    float m_yawBlendStart;              // 0x224

    uint32_t m_pitchLimitBlendStartMs;  // 0x228
    float m_pitchLimitBlendDuration;    // 0x22C
    float m_pitchLimitBlendFrom;        // 0x230
    float m_pitchLimitBlendTo;          // 0x234
    float m_pitchLimitBlendStart;       // 0x238
    float m_pitchLimitBlendTime;        // 0x23C

    uint32_t m_pitchBlendStartMs;       // 0x240
    float m_pitchBlendDuration;         // 0x244
    float m_pitchBlendFrom;             // 0x248
    float m_pitchBlendTo;               // 0x24C
    float m_pitchBlendEnd;              // 0x250
    float m_pitchBlendStart;            // 0x254

    uint32_t m_facingLimitBlendStartMs; // 0x258
    float m_facingLimitBlendDuration;   // 0x25C
    float m_facingLimitFrom;            // 0x260
    float m_facingLimitTo;              // 0x264
    float m_facingLimitBlendEnd;        // 0x268
    float m_facingLimitBlendStart;      // 0x26C

    uint32_t m_fovBlendStartMs;         // 0x270
    float m_fovBlendDuration;           // 0x274
    float m_fovBlendFrom;               // 0x278
    float m_fovBlendTo;                 // 0x27C
    float m_fovBlendEnd;                // 0x280
    float m_fovBlendStart;              // 0x284

    uint32_t m_freelookStartMs;         // 0x288
    float m_freelookDuration;           // 0x28C
    C3Vector m_freelookSnapTarget;      // 0x290
    uint32_t m_autoRotateStartMs;       // 0x29C
    unk_t unk_2A0;
    unk_t unk_2A4;

    uint32_t m_fovAltBlendStartMs;      // 0x2A8
    float m_fovAltBlendDuration;        // 0x2AC
    float m_fovBlendAltFrom;            // 0x2B0
    float m_fovBlendAltTo;              // 0x2B4
    float m_fovBlendAltEnd;             // 0x280
    float m_fovBlendAltStart;           // 0x284

    float m_fovOffsetScale;             // 0x2C0
    uint32_t m_zoomProfileID;           // 0x2C4
    float m_profileZoomDist[2];         // 0x2C8
    unk_t unk_2D0[8];                   // 0x2D0

    unk_t unk_2F0;                      // 0x2F0
    uintptr_t m_m2sceneNode;            // 0x2F4
    uintptr_t m_m2scenePrev;            // 0x2F8
    uint32_t m_blendStyleInit;          // 0x2FC
    uintptr_t m_listNext;               // 0x300
    CameraShake* m_shakeListHead;       // 0x304
    uintptr_t m_shakeListTail;          // 0x308

    float m_savedViewIndex;             // 0x30C
    float m_savedZoomDist;              // 0x310
    float m_savedPitchLimit;            // 0x314
    float m_telescopePitchOffset;       // 0x318
    void* m_vehicleCam;                 // 0x31C

    inline static auto GetActiveCameraFn = reinterpret_cast<CGCamera*(*)()>(0x004F5960);
    static CGCamera* GetActiveCamera() { return GetActiveCameraFn(); }
};

class CGWorldFrame : public CSimpleFrame {
public:
    struct CGWorldFrameUnk {
        unk_t unk_00[1094];
        float m_mouseX;
        float m_mouseY;
    };

    unk_t unk_list1[3];                 // 0x29C
    unk_t unk_list2[3];                 // 0x2A8
    unk_t unk_list3[3];                 // 0x2B4

    uint32_t m_renderState;             // 0x2C0
    unk_t unk_2C4[4];                   // 0x2C4
    uint32_t m_pendingActionID;         // 0x2D4
    int32_t m_defaultActionResult;      // 0x2D8
    unk_t unk_2DC;                      // 0x2DC
    float unk_2E0[2];                   // 0x2E0

    uint32_t m_rollTransitionStartMs;   // 0x2E8
    uint32_t m_rollTransitionEndMs;     // 0x2EC
    float m_rollFrom;                   // 0x2EC
    float m_rollTo;                     // 0x2EC
    float m_cameraRoll;                 // 0x2F8
    unk_t unk_2FC[5];                   // 0x2FC

    float m_screenX_ndc_to_ddc;         // 0x310
    float m_screenY_ndc_to_ddc;         // 0x314

    unk_t m_unknownFlags;               // 0x318
    uint32_t m_flags_31C;               // 0x31C
    unk_t unk_320[4];                   // 0x320

	float m_viewBottom;                 // 0x330
    float m_viewLeft;                   // 0x334
    float m_viewTop;                    // 0x338
    float m_viewRight;                  // 0x33C
    C44Matrix m_viewMatrix;             // 0x340

    CGWorldFrameUnk* m_data;            // 0x380
    uint32_t m_lightSystem[479];        // 0x384
    unk_t unk_B00;                      // 0xB00
    float m_lastUpdateTime;             // 0xB04

    unk_t unk_B08;                      // 0xB08
    unk_t unk_B0C;                      // 0xB0C
    uint32_t m_renderDirtyFlags;        // 0xB10
    float m_sceneTime;                  // 0xB14
    uint32_t m_sceneObjectCache[7079];  // 0xB18

    unk_t unk_79B4[8];                  // 0x79B4
    uint32_t m_dataBlockUnk[275];       // 0x79D4

    CSimpleCamera* camera;              // 0x7E20
    // ...

    using Intersect_t = char(__cdecl*)(C3Vector* start, C3Vector* end, C3Vector* hitPoint, float* distance, uint32_t flag, uint32_t buffer);
    inline static auto IntersectFn = reinterpret_cast<Intersect_t>(0x0077F310);

    using UpdateNamePlatePositions_t = int(__cdecl*)(CGWorldFrame*);
    inline static auto UpdateNamePlatePositionsFn = reinterpret_cast<UpdateNamePlatePositions_t>(0x00725890);

    using UpdateNamePlatePosition_t = char(__cdecl*)(int, CGNamePlate*, CGWorldFrame*, C3Vector*, int);
    inline static auto UpdateNamePlatePositionFn = reinterpret_cast<UpdateNamePlatePosition_t>(0x00615E10);

    using GetScreenCoordinates_t = bool(__thiscall*)(CGWorldFrame*, C3Vector*, C3Vector*, int*);
    inline static auto GetScreenCoordinatesFn = reinterpret_cast<GetScreenCoordinates_t>(0x004F6D20);

    using To2D_t = int(__thiscall*)(CGWorldFrame*, C3Vector*, C3Vector*, uint32_t*);
    inline static auto To2DFn = reinterpret_cast<To2D_t>(0x004F6D20);

    using OnLayerTrackTerrain_t = int(__thiscall*)(CGWorldFrame*, int*);
    inline static auto OnLayerTrackTerrainFn = reinterpret_cast<OnLayerTrackTerrain_t>(0x004F66C0);

    int To2D(C3Vector* pos3d, C3Vector* pos2d, uint32_t* flags) { return To2DFn(this, pos3d, pos2d, flags); }
    int GetScreenCoordinates(C3Vector* pos, C3Vector* out, int* result) { return GetScreenCoordinatesFn(this, pos, out, result); }
    int OnLayerTrackTerrain(int* a2) { return OnLayerTrackTerrainFn(this, a2); }
    int UpdateNamePlatePositions() { return UpdateNamePlatePositionsFn(this); }
    char UpdateNamePlatePosition(int viewportId, CGNamePlate* namePlate, C3Vector* proj, int flag) {
        return UpdateNamePlatePositionFn(viewportId, namePlate, this, proj, flag);
    }

    static void PercToScreenPos(float x, float y, float* resX, float* resY) {
        if (!resX || !resY) return;
        float screenHeightAptitude = *reinterpret_cast<float*>(0x00AC0CBC);
        float someVal = *reinterpret_cast<float*>(0x00AC0CB4);
        if (std::abs(someVal) < 1e-6f) return;
        float scale = (screenHeightAptitude * 1024.0f) / someVal;
        *resX = x * scale; *resY = y * scale;
    }
    static int HandleNameplateLeftClick(guid_t guid) { return reinterpret_cast<int(__cdecl*)(guid_t)>(0x005274F0)(guid); }
    static int HandleNameplateRightClick(guid_t guid) { return reinterpret_cast<int(__cdecl*)(guid_t)>(0x005277B0)(guid); }

    static CGWorldFrame* GetWorldFrame() { return *reinterpret_cast<CGWorldFrame**>(0x00B7436C); }
};

class CSimpleAnim : public FrameScript_Object {
public:
    void* m_vmt;
    unk_t unk_04[2];
};

class CSimpleFrameScript : public CSimpleAnim, public CLayoutFrame {
public:
    unk_t unk_94[3];
};

class CSimpleRender : public CSimpleFrameScript {
public:
    unk_t unk_A0[13];
};

struct CGxFontVertex {
    C3Vector pos;
    float u, v;
};

struct CGxGlyphMetrics {
    void* m_pixelData;
    uint32_t m_bufferSize;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_widthPadded;
    float m_advanceX;
    float m_horiBearingX;
    unk_t unk_24;
    uint32_t m_bearingY;
    uint32_t m_verAdv;
    float v1, u0, v0, u1;
};
static_assert(sizeof(CGxGlyphMetrics) == 0x38);

struct CGxGlyphCacheEntry {
    uint32_t m_codepoint;
    CGxGlyphCacheEntry* m_hashNext;
    CGxGlyphCacheEntry* m_hashPrev;
    CGxGlyphCacheEntry* m_listNext;
    CGxGlyphCacheEntry* m_listPrev;
    unk_t unk_14;
    unk_t unk_18;
    unk_t unk_1C;
    unk_t unk_20;
    unk_t unk_24;
    uint32_t m_texturePageIndex;
    uint32_t m_atlasCellIndex;
    uint32_t m_cellIndexMin;
    uint32_t m_cellIndexMax;
    CGxGlyphMetrics m_metrics;
};
static_assert(sizeof(CGxGlyphCacheEntry) == 0x70);

struct CGxKernCacheEntry {
    // guess
    uint32_t charPairKey;
    CGxKernCacheEntry* m_prev;
    CGxKernCacheEntry* m_next;
    float kerningOffset;
};

struct CGxFontTextureCache {
    uint32_t m_rasterSizePadded;
    uint32_t m_pageInfo;
    uint32_t m_flags;
    float m_scaleX;
    float m_scaleY;
    void* m_glyphs;
};
static_assert(sizeof(CGxFontTextureCache) == 0x18);

struct CGxFontGeomBatch {
    CGxFontGeomBatch* m_prev;
    CGxFontGeomBatch* m_next;
    TSGrowableArray<CGxFontVertex> m_verts;
    TSGrowableArray<uint16_t> m_indices;
};

struct CGlyphTexCell {
    uint32_t codepoint;
    uint16_t u0, v0;
    uint16_t u1, v1;
    uint16_t width;
    uint16_t height;
    uint32_t flags;
};

struct CFontCache {
    uint32_t m_bucketCount;
    uint32_t m_entryCount;
    CGlyphTexCell** m_buckets;
    uint32_t m_mask;
};

class CGxFont {
public:
    CGxFont* m_prev;                                // 0x00
    CGxFont* m_next;                                // 0x04
    uint32_t m_refCount;                            // 0x08
    char* m_fontName;                               // 0x0C
    char* m_fontFamily;                             // 0x10
    TSHashTable<CGxGlyphCacheEntry> m_glyphCache;   // 0x14
    TSHashTable<CGxKernCacheEntry> m_kernCache;     // 0x3C
    TSList<CGxGlyphCacheEntry> m_glyphsList;        // 0x64
    void* m_ftWrapper;                              // 0x70
    char m_fontPath[260];                           // 0x74
    CGxFontTextureCache m_atlasPages[8];            // 0x178
    float m_kerningScale;                           // 0x238
    float m_isLoaded;                               // 0x23C
    float m_fontHeight;                             // 0x240
    uint32_t m_styleFlags;                          // 0x244
    uint32_t m_effectivePixelHeight;                // 0x248
    uint32_t m_rasterTargetSize;                    // 0x24C

    using GetOrCreateGlyphEntry_t = CGxGlyphCacheEntry * (__thiscall*)(CGxFont*, uint32_t codepoint);
    using GetBearingX_t = double(__thiscall*)(CGxFont*, CGxGlyphCacheEntry*, float flag, float scale);

    inline static GetOrCreateGlyphEntry_t GetOrCreateGlyphEntryFn = reinterpret_cast<GetOrCreateGlyphEntry_t>(0x006C3FC0);
    inline static GetBearingX_t GetBearingXFn = reinterpret_cast<GetBearingX_t>(0x006C24F0);

    double GetBearingX(CGxGlyphCacheEntry* entry, float flag, float scale) { return GetBearingXFn(this, entry, flag, scale); }
    CGxGlyphCacheEntry* GetOrCreateGlyphEntry(uint32_t codepoint) { return GetOrCreateGlyphEntryFn(this, codepoint); }
};
static_assert(sizeof(CGxFont) == 0x250);

class CGxString {
public:
    CGxString* m_ll_next;                       // 0x00
    CGxString* m_ll_prev;                       // 0x04
    unk_t unk_08[4];                            // 0x08
    float m_fontSize;                           // 0x18
    float m_fontSizeMult;                       // 0x1C
    C3Vector m_anchorPos;                       // 0x20
    uint32_t m_textColor;                       // 0x2C
    uint32_t m_shadowColor;                     // 0x30
    Vec2D<float> m_shadowOffset;                // 0x34
    float m_widthBBox;                          // 0x3C
    float m_heightBBox;                         // 0x40
    CGxFont* m_fontObj;                         // 0x44
    char* m_text;                               // 0x48
    uint32_t m_text_capacity;                   // 0x4C
    uint32_t m_vertAlign;                       // 0x50
    uint32_t m_horzAlign;                       // 0x54
    float m_lineSpacing;                        // 0x58
    uint32_t m_flags;                           // 0x5C
    uint32_t m_bitfield;                        // 0x60
    uint32_t m_isDirty;                         // 0x64
    int32_t m_gradientStartChar;                // 0x68
    int32_t m_gradientLength;                   // 0x6C
    C3Vector m_finalPos;                        // 0x70
    TSGrowableArray<void*> m_hyperlinks;        // 0x7C
    TSGrowableArray<void*> m_embeddedTextures;  // 0x8C
    unk_t unk_A0;                               // 0x9C
    uint32_t m_hyperlinkClickCount;             // 0xA0
    TSGrowableArray<void*> m_gradientInfo;      // 0xA4
    CGxFontGeomBatch* m_geomBuffers[8];         // 0xB4
    uint32_t m_timeSinceUpdate;                 // 0xD4

    FT_Face GetFontFace() const { return reinterpret_cast<FT_Face(*)(void*)>(0x006C8080)(this->m_fontObj->m_ftWrapper); }
    static FT_Face GetFontFace(void* ptr) { return reinterpret_cast<FT_Face(*)(void*)>(0x006C8080)(ptr); }

    using WriteGeometry_t = void(__thiscall*)(CGxString*, int destPtr, int index, int vertIndex, int vertCount);
    using InitializeTextLine_t = int(__thiscall*)(CGxString*, char* text, int textLength, int* a4, C3Vector* startPos, void*, int);
    using ClearInstanceData_t = int* (__thiscall*)(CGxString*);
    using CheckGeometry_t = bool(__thiscall*)(CGxString*);
    using GetVertCountForPage_t = uint32_t(__thiscall*)(CGxString*, int pageIdx);

    inline static WriteGeometry_t WriteGeometryFn = reinterpret_cast<WriteGeometry_t>(0x006C5E90);
    inline static InitializeTextLine_t InitializeTextLineFn = reinterpret_cast<InitializeTextLine_t>(0x006C6CD0);
    inline static ClearInstanceData_t ClearInstanceDataFn = reinterpret_cast<ClearInstanceData_t>(0x006C6B90);
    inline static CheckGeometry_t CheckGeometryFn = reinterpret_cast<CheckGeometry_t>(0x006C7480);
    inline static GetVertCountForPage_t GetVertCountForPageFn = reinterpret_cast<GetVertCountForPage_t>(0x006C63E0);

    void WriteGeometry(int destPtr, int index, int vertIndex, int vertCount) {  WriteGeometryFn(this, destPtr, index, vertIndex, vertCount); }
    int InitializeTextLine(char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7) { return InitializeTextLineFn(this, text, textLength, a4, startPos, a6, a7); }
    int* ClearInstanceData() { return ClearInstanceDataFn(this); }
    bool CheckGeometry() { return CheckGeometryFn(this); }
    uint32_t GetVertCountForPage(int pageIdx) { return GetVertCountForPageFn(this, pageIdx); }
};
static_assert(sizeof(CGxString) == 0xD8);

struct TextureWrapperInternal {
    uint32_t m_width;           // 0x00
    uint32_t m_height;          // 0x04
    unk_t unk_08[3];            // 0x08
    uint32_t m_width_alt;       // 0x14
    uint32_t m_height_alt;      // 0x18
    unk_t  unk_1C[7];           // 0x1C
    IDirect3DTexture9* d3dTex;  // 0x38
    unk_t unk_3C;               // 0x3C
};
static_assert(sizeof(TextureWrapperInternal) == 0x40);

struct TextureWrapper {
    unk_t unk_00[17];                   // 0x00
    TextureWrapperInternal* internal;   // 0x44
    // ...
};

struct TextureSlot {
    TextureWrapper* mWrapper;  // 0x00
    unk_t unk_04[5];            // 0x04
};
static_assert(sizeof(TextureSlot) == 0x18);

struct TextureData {
    unk_t unk[99];                  // 0x00
    TextureSlot m_slots[8];         // 0x18C
    // ...
};

struct GxuFontBatchNode {
    GxuFontBatchNode* m_next;
    GxuFontBatchNode* m_prev;
    uint32_t m_linkNode;
    GxuFontBatchNode* m_nextInSubList;
    GxuFontBatchNode* m_prevInSubList;
    unk_t unk_14;
    TextureData* m_texture;
    void* m_stringListHead;
    unk_t unk_20;
    CGxString* m_firstString;
    uint32_t m_linkedListOffset;
};
static_assert(sizeof(GxuFontBatchNode) == 0x2C);

class CGxuFont {
public:
    void* vmt;
    unk_t unk_04;
    uint32_t m_flags;
    void(__thiscall** m_nodeCleanup)(CGxuFont*, GxuFontBatchNode*);
    uint32_t m_nodeLinkOffset;
    unk_t unk_14;
    GxuFontBatchNode m_head;

    using RenderBatch_t = void(__thiscall*)(CGxuFont*);
    using GetFontEffectiveWidth_t = double(__cdecl*)(int, float);
    using GetFontEffectiveHeight_t = double(__cdecl*)(int, float);
    using RenderGlyph_t = char(__cdecl*)(FT_Face, uint32_t, uint32_t, uint32_t, CGxGlyphMetrics*, uint32_t, uint32_t);

    inline static RenderBatch_t RenderBatchFn = reinterpret_cast<RenderBatch_t>(0x006C53A0);
    inline static GetFontEffectiveWidth_t GetFontEffectiveWidthFn = reinterpret_cast<GetFontEffectiveWidth_t>(0x006C0B60);
    inline static GetFontEffectiveHeight_t GetFontEffectiveHeightFn = reinterpret_cast<GetFontEffectiveHeight_t>(0x006C0B20);
    inline static RenderGlyph_t RenderGlyphFn = reinterpret_cast<RenderGlyph_t>(0x006C8CC0);

    void RenderBatch() { RenderBatchFn(this); }
    static double GetFontEffectiveWidth(int is3d, float fontSizeMult) { return GetFontEffectiveWidthFn(is3d, fontSizeMult); }
    static double GetFontEffectiveHeight(int is3d, float fontSizeMult) { return GetFontEffectiveHeightFn(is3d, fontSizeMult); }
    static char RenderGlyph(FT_Face face, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGxGlyphMetrics* entry, uint32_t outline_flag, uint32_t pad) {
        return RenderGlyphFn(face, fontSize, codepoint, pageInfo, entry, outline_flag, pad);
    }
};
static_assert(sizeof(CGxuFont) == 0x44);

class CSimpleFontString : public CSimpleRender {
public:
    struct CSimpleRegion {};

    void* vmt;
    unk_t unk_04_2[4];
    CGxuFont* m_font;
    float m_baseSize;
    uint16_t m_textFlags;
    uint16_t m_textLen;
    char* m_text;
    float m_effectiveSize;
    int m_textBlockHandle;
    float m_cachedWidth;
    float m_cachedHeight;
    uint32_t m_shadowColorARGB;
    float m_shadowOffsetX;
    float m_shadowOffsetY;
    float m_posNudgeX;
    float m_posNudgeY;
    uint32_t m_wrapFlags;
    ETextStateFlags m_flags_fs;
    uint32_t m_defaultColor;
    uint32_t m_colorAlphaFlags;
    float m_anchorX;
    float m_anchorY;
    float m_anchorZ;
    uintptr_t m_regionNodeOffset;
    CSimpleRegion* m_regionListNext;
    CSimpleRegion* m_regionListPrev;
};
static_assert(sizeof(CSimpleFontString) == 0x144);

class CGNamePlate : public CSimpleFrame {
public:
    static constexpr float DefaultHeight = 0.025f;
    static constexpr float DefaultWidth = 0.1f;

    struct CSimpleTexture {};

    CGNamePlate* m_sortedPrev;          // 0x29C
    CGNamePlate* m_sortedNext;          // 0x2A0
    unk_t unk_2A4;                      // 0x2A4
    guid_t m_ownerGuid;                 // 0x2A8

    CLayoutFrame* m_focusFrame;         // 0x2B0
    CSimpleTexture* m_bossIcon;         // 0x2B4
    CSimpleTexture* m_raidMarkerIcon;   // 0x2B8
    CSimpleFrame* m_borderFrame;        // 0x2BC
    CSimpleTexture* m_castShield;       // 0x2C0
    CSimpleTexture* m_castBarFlash;     // 0x2C4
    CSimpleTexture* m_raidIcon;         // 0x2C8
    CSimpleFontString* m_nameFS;        // 0x2CC
    CSimpleFontString* m_levelFS;       // 0x2D0
    CSimpleFrame* m_healthBar;          // 0x2D4
    CSimpleFrame* m_castBar;            // 0x2D8
    CSimpleFrame* m_statusBar;          // 0x2DC

    Vec2D<float> m_NDCproj;             // 0x2E0
    float m_depthZ;                     // 0x2E8

    uint32_t m_plateColor;              // 0x2EC
    float m_castStartTime;              // 0x2F0
    float m_castEndTime;                // 0x2F4
    uint32_t m_castFlags;               // 0x2F8

	inline static auto VerticalPlacementOffset = reinterpret_cast<float*>(0x009FC830);

    using Create_t = CGNamePlate* (__thiscall*)(CGNamePlate*, CSimpleFrame*);
    inline static auto CreateFn = reinterpret_cast<Create_t>(0x0098F790);

    using Initialize_t = int(__thiscall*)(CGNamePlate*, CGUnit_C*);
    inline static auto InitializeFn = reinterpret_cast<Initialize_t>(0x0098F390);

    CGNamePlate* Create(CSimpleFrame* p) { return CreateFn(this, p); }
    int HasPlateState(CGUnit_C* unit) { return InitializeFn(this, unit); }

    bool HasPlateState(EFrameState flags) const { return (this->m_stateFlags & flags) != 0; }
    void SetPlateState(EFrameState flag, bool val) {
        if (val) this->m_stateFlags = static_cast<EFrameState>(this->m_stateFlags | flag);
        else this->m_stateFlags = static_cast<EFrameState>(this->m_stateFlags & ~flag);
    }
    void SetPlateId(int id) {
        this->m_stateFlags = static_cast<EFrameState>((this->m_stateFlags & ~NP_ID_MASK) | ((id + 1) << NP_ID_SHIFT));
    }
    int GetPlateId() const {
        return static_cast<int>((this->m_stateFlags & NP_ID_MASK) >> NP_ID_SHIFT) - 1;
    }

    using OnLoseFocus_t = CSimpleFrame*(__thiscall*)(CGNamePlate*);
    inline static auto OnLoseFocusFn = reinterpret_cast<OnLoseFocus_t>(0x0098E980);

    CSimpleFrame* OnLoseFocus() { return OnLoseFocusFn(this); }

    using OnGainFocus_t = CSimpleFrame*(__thiscall*)(CGNamePlate*);
    inline static auto OnGainFocusFn = reinterpret_cast<OnGainFocus_t>(0x0098E910);

    CSimpleFrame* OnGainFocus() { return OnGainFocusFn(this); }
};
static_assert(sizeof(CGNamePlate) == 0x300);

// CVar
class CVar {
public:
    enum CVarFlags : uint16_t {
        CVarFlags_ReadOnly = 0x4,
        CVarFlags_CheckTaint = 0x8,
        CVarFlags_HideFromUser = 0x40,
        CVarFlags_ReadOnlyForUser = 0x100,
    };

    using Handler_t = int(*)(CVar* cvar, const char* prevVal, const char* newVal, void* userData);

    uint32_t m_hash;
    unk_t unk_04[4];
    const char* m_name;
    uint32_t m_field_18;
    CVarFlags m_flags;
    uint32_t m_field_20;
    uint32_t m_field_24;
    const char* m_str;
    unk_t unk_2C[5];
    uint32_t m_bool;
    unk_t unk_44[9];
    Handler_t m_handler;
    void* m_userData;

    inline static auto InitializeFn = reinterpret_cast<DummyCallback_t>(0x007663F0);

    using InternalSet_t = void(__thiscall*)(CVar*, const char*, bool, bool, bool, bool);
    inline static auto InternalSetFn = reinterpret_cast<InternalSet_t>(0x007667B0);

    using Register_t = CVar* (*)(const char*, const char*, unsigned, const char*, CVar::Handler_t, int, int, int, int);
    inline static auto RegisterFn = reinterpret_cast<Register_t>(0x00767FC0);

    using SetValue_t = char(__thiscall*)(CVar*, const char*, int, int, int, int);
    inline static auto SetValueFn = reinterpret_cast<SetValue_t>(0x007668C0);

    char SetValue(const char* value, int a3, int a4, int a5, int a6) { return SetValueFn(this, value, a3, a4, a5, a6); }

    template<typename T>
    int Sync(const char* rawValue, T* globalVar, T minVal, T maxVal, const char* fmt) {
        if (!rawValue || !globalVar) return 0;
        T requested;
        if constexpr (std::is_floating_point_v<T>) requested = static_cast<T>(std::atof(rawValue));
        else requested = static_cast<T>(std::atoi(rawValue));
        T clamped = std::clamp(requested, minVal, maxVal);
        *globalVar = clamped;
        if (requested != clamped) {
            char buf[32];
            if constexpr (std::is_floating_point_v<T>) snprintf(buf, sizeof(buf), fmt, static_cast<double>(clamped));
            else snprintf(buf, sizeof(buf), fmt, clamped);
            InternalSetFn(this, buf, true, false, false, true);
            return 0;
        }
        return 1;
    }

    static CVar* Get(const char* name) { return (reinterpret_cast<CVar* (*)(const char*)>(0x00767460))(name); }
    static CVar* Find(const char* name) { return (reinterpret_cast<CVar* (*)(const char*)>(0x00767440))(name); }
    static CVar* Register(const char* name, const char* desc, unsigned flags, const char* defaultVal, Handler_t callback,
            int a6, int a7, int a8, int a9) {
        return RegisterFn(name, desc, flags, defaultVal, callback, a6, a7, a8, a9);
    }
};
static_assert(sizeof(CVar) == 0x70);

class FreeType {
public:
    using Init_t = int(__cdecl*)(void* memory, FT_Library*);
    using NewMemoryFace_t = int(__cdecl*)(FT_Library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face*);
    using Done_Face_t = int(__cdecl*)(FT_Face);
    using SetPixelSizes_t = int(__cdecl*)(FT_Face, FT_UInt pixel_width, FT_UInt pixel_height);
    using GetCharIndex_t = FT_UInt(__cdecl*)(FT_Face, FT_ULong charcode);
    using LoadGlyph_t = int(__cdecl*)(FT_Face, FT_ULong glyph_index, FT_Int32 load_flags);
    using GetKerning_t = int(__cdecl*)(FT_Face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning);
    using Done_FreeType_t = int(__cdecl*)(FT_Library);
    using NewFace_t = int(__cdecl*)(int* library, int face_descriptor);

    inline static auto InitFn = reinterpret_cast<Init_t>(0x00991320);
    inline static auto NewMemoryFaceFn = reinterpret_cast<NewMemoryFace_t>(0x00993370);
    inline static auto Done_FaceFn = reinterpret_cast<Done_Face_t>(0x00992610);
    inline static auto SetPixelSizesFn = reinterpret_cast<SetPixelSizes_t>(0x00992780);
    inline static auto GetCharIndexFn = reinterpret_cast<GetCharIndex_t>(0x009911A0);
    inline static auto LoadGlyphFn = reinterpret_cast<LoadGlyph_t>(0x00992DA0);
    inline static auto GetKerningFn = reinterpret_cast<GetKerning_t>(0x00991050);
    inline static auto Done_FreeTypeFn = reinterpret_cast<Done_FreeType_t>(0x00992CB0);
    inline static auto NewFaceFn = reinterpret_cast<NewFace_t>(0x009931A0);
};

class CGxDevice {
public:
    using FlushBuffer_t = int(__cdecl*)(int* bufferHandle, int bufferSize);
    using InitFontIndexBuffer_t = int(__cdecl*)();
    using PoolCreate_t = void* (__thiscall*)(IDirect3DDevice9*, int flags, int pad, int size, int usage, const char* name);

    inline static auto FlushBufferFn = reinterpret_cast<FlushBuffer_t>(0x006C48D0);
    inline static auto InitFontIndexBufferFn = reinterpret_cast<InitFontIndexBuffer_t>(0x006C47B0);
    inline static auto PoolCreateFn = reinterpret_cast<PoolCreate_t>(0x006876D0);

    struct ShaderData {
        IUnknown* base_interface;                       // 0x00
        DWORD unknown_vtable_ptr;                       // 0x04
        IDirect3DResource9* resource_interface_1;       // 0x08
        IDirect3DResource9* resource_interface_2;       // 0x0C
        IDirect3DBaseTexture9* texture_interface_1;     // 0x10
        IDirect3DBaseTexture9* texture_interface_2;     // 0x14
        void* additional_resource;                      // 0x18
        DWORD reference_count;                          // 0x1C

        union {                                         // 0x20
            IDirect3DPixelShader9* pixel_shader;
            IDirect3DVertexShader9* vertex_shader;
        };

        DWORD shader_version;                           // 0x24
        DWORD reserved_1;                               // 0x28
        DWORD compilation_flags;                        // 0x2C

        DWORD shader_enabled;                           // 0x30
        DWORD reserved_2;                               // 0x34
        DWORD reserved_3;                               // 0x38
        DWORD texture_dimension_flags;                  // 0x3C
        DWORD constant_buffer_size;                     // 0x40
        DWORD active_samplers;                          // 0x44
        DWORD instruction_slots;                        // 0x48
        DWORD bytecode_length;                          // 0x4C

        void* bytecode_memory;                          // 0x50
        DWORD reserved_4;                               // 0x54
        DWORD creation_timestamp;                       // 0x58
        DWORD bytecode_checksum;                        // 0x5C

        void* texture_stage_state;                      // 0x60
        void* sampler_state_block;                      // 0x64
        DWORD active_texture_stages;                    // 0x68
        DWORD primary_sampler_index;                    // 0x6C
        DWORD blend_stage_enabled;                      // 0x70
        DWORD alpha_test_enabled;                       // 0x74
        DWORD render_state_flags;                       // 0x78
        DWORD fog_enabled;                              // 0x7C

        DWORD lighting_enabled;                         // 0x80
        DWORD vertex_shader_constants;                  // 0x84
        DWORD pixel_shader_constants;                   // 0x88
        DWORD texture_filter_flags;                     // 0x8C
        DWORD mipmap_settings;                          // 0x90
        DWORD reserved_5;                               // 0x94
        DWORD reserved_6;                               // 0x98
        DWORD reserved_7;                               // 0x9C

        DWORD font_antialiasing;                        // 0xA0
        DWORD subpixel_rendering;                       // 0xA4
        DWORD reserved_8;                               // 0xA8

        char coordinate_data[20];                       // 0xAC

        DWORD extended_flags_1;                         // 0xC0
        DWORD reserved_9;                               // 0xC4
        float unknown_float_1;                          // 0xC8
        DWORD combined_hash;                            // 0xCC
        DWORD validation_flag;                          // 0xD0

        WORD max_texture_width;                         // 0xD4
        WORD max_texture_height;                        // 0xD8
        DWORD padding[8];                               // 0xDC
        DWORD final_validation;                         // 0xFC
    };

    using DeviceCreate_t = int(__thiscall*)(void*, IDirect3DDevice9*, int);
    using NotifyOnDeviceRestored_t = int(__thiscall*)(void*);

    using DeviceSetFormat_t = int(__thiscall*)(char*, const void*);
    using IDestroyD3d_t = int(__thiscall*)(int*);
    using IReleaseD3dResources_t = int(__thiscall*)(void*, int);
    using IShaderCreateVertex_t = void(__thiscall*)(int, ShaderData*);
    using IShaderCreatePixel_t = void(__thiscall*)(int, ShaderData*);

    inline static auto DeviceCreateFn = reinterpret_cast<DeviceCreate_t>(0x00682CB0);
    inline static auto NotifyOnDeviceRestoredFn = reinterpret_cast<NotifyOnDeviceRestored_t>(0x006843B0);

    inline static auto DeviceSetFormatFn = reinterpret_cast<DeviceSetFormat_t>(0x006904D0);
    inline static auto IDestroyD3dFn = reinterpret_cast<IDestroyD3d_t>(0x006903B0);
    inline static auto IReleaseD3dResourcesFn = reinterpret_cast<IReleaseD3dResources_t>(0x00690150);
    inline static auto IShaderCreateVertexFn = reinterpret_cast<IShaderCreateVertex_t>(0x006AA0D0);
    inline static auto IShaderCreatePixelFn = reinterpret_cast<IShaderCreatePixel_t>(0x006AA070);
};

class DBItemCache {
public:
    using GetItemInfoBlockById_t = uintptr_t(__thiscall*)(DBItemCache*, uint32_t, guid_t*, int, int, int);
    inline static const auto GetItemInfoBlockByIdFn = reinterpret_cast<GetItemInfoBlockById_t>(0x0067CA30);

    inline static const auto WDB_CACHE_ITEM = reinterpret_cast<DBItemCache*>(0x00C5D828);

    uintptr_t GetItemInfoBlockById(uint32_t id, guid_t* guid, int a4, int a5, int a6) {
        return GetItemInfoBlockByIdFn(this, id, guid, a4, a5, a6);
    }
};