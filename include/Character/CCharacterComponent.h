#pragma once

#include <cstdint>

#include "DB/DBRecords.h"
#include "Graphics/GraphicsEnums.h"
#include "Lib/Storm.h"
#include "M2/CM2Model.h"
#include "Texture/CTexture.h"

class CCharacterComponent {
public:
    struct CharSectionsLookup {
        struct Entry {
            int color_count;
            CharSectionsRec** rec;  // array of CharSectionsRec* of length [color_count]
        };

        int variation_count;
        Entry* entry;  // array of variations of length [variation_count]
    };

    static_assert(sizeof(CharSectionsLookup) == 0x8);

    struct RegionTexContext {
        Vec2i pos;
        Vec2i dims;
    };

    static_assert(sizeof(RegionTexContext) == 0x10);

    inline static int& base_skin_updating = *reinterpret_cast<int*>(0x00B6B884);
    inline static GxTexFormat& tex_fmt = *reinterpret_cast<GxTexFormat*>(0x00B6B85C);
    inline static uint32_t& comp = *reinterpret_cast<uint32_t*>(0x00B6B880);
    inline static uint32_t& max_tex_size = *reinterpret_cast<uint32_t*>(0x00B6B5FC);
    inline static uint32_t (*&facial_hair_count)[2] = *reinterpret_cast<uint32_t (**)[2]>(
        0x00B6B860);  // [raceId][genderId]
    inline static CharSectionsLookup& char_sections_lookup = *reinterpret_cast<CharSectionsLookup*>(0x00B6B864);
    inline static RegionTexContext (&region_contexts)[10] = *reinterpret_cast<RegionTexContext (*)[10]>(0x00B6B888);
    inline static RegionTexContext (&base_region_contexts)[10] = *reinterpret_cast<RegionTexContext (*)[10]>(
        0x00B6B928);

    // initialized @ 004F1A20
    using AsyncUpdateRegion = uint32_t(__thiscall*)(CCharacterComponent*);
    inline static AsyncUpdateRegion (&update_region_async)[10] = *reinterpret_cast<AsyncUpdateRegion (*)[10]>(
        0x00B6B4B8);

    using UpdateRegion = uint32_t(__thiscall*)(
        CCharacterComponent*, int equipment_slot, ItemDisplayInfoRec* item_display_info, char apply_tex);
    inline static UpdateRegion (&update_region)[10] = *reinterpret_cast<UpdateRegion (*)[10]>(0x00B6B80C);

    using PrepRegion = uint32_t(__thiscall*)(CCharacterComponent*);
    inline static PrepRegion (&prep_region)[10] = *reinterpret_cast<PrepRegion (*)[10]>(0x00B6B834);

    struct CharData {
        enum TexFlags : uint32_t {
            eFlagCustomBaseTexture = 0x1,
            eFlagForceD3DformatArgb = 0x2,
        };

        RaceId race_id;
        CharGender gender_id;
        ClassId class_id;
        uint32_t hair_color;
        uint32_t skin_id;
        uint32_t face_id;
        uint32_t facial_hair_style;
        uint32_t hair_style;
        CM2Model* model;
        TexFlags flags;
        char custom_texture_path[260];
        uint32_t geosets[19];  // CharGeosetGroup
    };

    static_assert(sizeof(CharData) == 0x178);

    struct RegionState {
        TCACHEENTRY* layer_textures[7];
        uint32_t layer_item_display_ids[7];
        uint32_t layer_mask;
    };

    static_assert(sizeof(RegionState) == 0x3C);

    enum TextureDirtyFlags : uint32_t {
        eCharTexDirtyAllRegions = 0x1,
        eCharTexBaseSkinDirty = 0x2,
        eCharTexDirtyGeometry = 0x4,
        eCharTexCacheValid = 0x8,
        eCharTexBarefoot = 0x10,
        eCharTexHideRobeGeoset = 0x20,
        eCharTexShowTabardGeoset = 0x40,
    };

    enum RegionDirtyFlags : uint32_t {
        eCharRegionDirtyArmUpper = 0x1,
        eCharRegionDirtyArmLower = 0x2,
        eCharRegionDirtyHand = 0x4,
        eCharRegionDirtyTorsoUpper = 0x8,
        eCharRegionDirtyTorsoLower = 0x10,
        eCharRegionDirtyLegUpper = 0x20,
        eCharRegionDirtyLegLower = 0x40,
        eCharRegionDirtyFoot = 0x80,
        eCharRegionDirtyScalpUpper = 0x100,
        eCharRegionDirtyScalpLower = 0x200,
    };

    class CComponentMipBits {
    public:
        inline static const int32_t (&kPixelFormatToMipBitsCache)[44] = *reinterpret_cast<const int32_t (*)[44]>(
            0x009F1074);

        using CacheGrid = TSExplicitList<CComponentMipBits>[6][6][2];  // argb8888 / dxt
        inline static CacheGrid& TSL_array = *reinterpret_cast<CacheGrid*>(0x00B49E88);

        struct {
            uint8_t** levels = nullptr;

            uint8_t* operator[](size_t level) const { return levels[level]; }

            template <typename PixelType>
            PixelType* getLevelPixels(size_t level) const {
                return reinterpret_cast<PixelType*>(levels[level]);
            }
        } mip_levels_;

        CPixelFormat format_;
        TSLink<CComponentMipBits> link_;
    };

    static_assert(sizeof(CComponentMipBits) == 0x10);

    class CComponentRequest {
    public:
        enum class RequestState : uint32_t {
            eInactive = 0,
            ePending = 1u,
            eDone = 2u,
        };
        RequestState state_;
        GxTexFormat format_;
        CComponentMipBits* mip_bits_;
        TCACHEENTRY* region_entries_[10];
        uint32_t entry_count_;
        TCACHEENTRY* entries_[40];
        uint8_t regions_[40];
        TSLink<CComponentRequest> link_;
    };

    static_assert(sizeof(CComponentRequest) == 0x108);

    TSLink<CCharacterComponent> link_;
    TextureDirtyFlags texture_dirty_;
    RegionDirtyFlags region_dirty_;
    uint32_t heap_list_id_;  // 0x00B4AFC0
    GxTexFormat format_;
    CharData char_data_;
    CTexture* base_skin_composite_;
    TCACHEENTRY* base_textures_[15];
    RegionState region_states_[10];
    uint32_t equipment_display_ids_[12];
    uint32_t weapon_display_ids_[3];
    uint32_t weapon_and_visual_ids_[50];
    CComponentRequest* request_;

    // fmt, resolution level, threading, compress to dxt1
    HOOKKIT_HOOK(init, 0x004F1A20, hookkit::Conv::eCdecl, char, GxTexFormat, uint32_t, int, int);
    HOOKKIT_HOOK(alloc, 0x004F0980, hookkit::Conv::eCdecl, CCharacterComponent*);
    HOOKKIT_HOOK(free, 0x004F16C0, hookkit::Conv::eCdecl, void, CCharacterComponent*);
    HOOKKIT_HOOK(releaseRegionTextures, 0x004E7650, hookkit::Conv::eFastcall, void);
    HOOKKIT_HOOK(updateCharacterBaseSkin, 0x004F14A0, hookkit::Conv::eThiscall, void, CCharacterComponent*);
    HOOKKIT_HOOK(getSectionsRec, 0x004E76D0, hookkit::Conv::eThiscall, CharSectionsRec*, CCharacterComponent*, uint32_t,
        uint32_t, int, void*);
};

static_assert(sizeof(CCharacterComponent) == 0x530);
