#pragma once

#include <cstdint>

#include "Graphics/GraphicsEnums.h"
#include "Math/Primitives.h"

class CTexture;

enum GroupFlags : uint32_t {
    eGroupHasBsp = 0x1,
    eGroupHasVertexColors = 0x4,
    eGroupExterior = 0x8,
    eGroupExteriorLit = 0x40,
    eGroupUnreachable = 0x80,
    eGroupHasLights = 0x200,
    eGroupHasMpbChunks = 0x400,
    eGroupHasDoodads = 0x800,
    eGroupHasLiquid = 0x1000,
    eGroupInterior = 0x2000,
    eGroupNoPortals = 0x10000,
    eGroupHasStrips = 0x20000,
    eGroupSkybox = 0x40000,
    eGroupOceanLiquid = 0x80000,
    eGroupHasVertexColors2 = 0x1000000,
    eGroupHasTexCoords2 = 0x2000000,
};

// the group flag sets the engine tests together
inline constexpr uint32_t kGroupOutdoor = eGroupExterior | eGroupNoPortals;  // bucketed and culled outdoors
// lit as exterior (eExteriorLit on the def group), and the end of a walk's interior path
inline constexpr uint32_t kGroupLitAsExterior = eGroupExterior | eGroupExteriorLit;
inline constexpr uint32_t kGroupExteriorPortal = 0x50148;  // the camera walk adds an exterior portal into these
inline constexpr uint32_t kGroupNoEntryPortal = 0x140;     // the exterior walk adds no entry portal into these
inline constexpr uint32_t kGroupExteriorSeed = 0x40140;    // a camera group seeds a full exterior rect for these

struct SMOHeader {
    enum Flags : uint16_t {
        eNoPortalAttenuation = 0x1,
        eUnifiedRenderPath = 0x2,
        eLiquidTypeDbcId = 0x4,
        eNoVertexColorAlphaFix = 0x8,
    };

    uint32_t texture_count;
    uint32_t group_count;
    uint32_t portal_count;
    uint32_t light_count;
    uint32_t doodad_name_count;
    uint32_t doodad_def_count;
    uint32_t doodad_set_count;
    ColorBGRA<> ambient_color;
    uint32_t wmo_id;
    AaBox bounds;
    uint16_t flags;
    uint16_t _unk3E;
};

static_assert(sizeof(SMOHeader) == 0x40);

struct SMOMaterial {
    enum Flags : uint32_t {
        eUnlit = 0x1,
        eUnfogged = 0x2,
        eTwoSided = 0x4,
        eExtLight = 0x8,
        eSidn = 0x10,
        eWindow = 0x20,
        eClampS = 0x40,
        eClampT = 0x80,
    };

    uint32_t flags;
    uint32_t shader;
    GxBlend blend_mode;
    uint32_t texture1_ofs;
    ColorBGRA<> sidn_color;
    ColorBGRA<> frame_sidn_color;
    uint32_t texture2_ofs;
    ColorBGRA<> diff_color;
    uint32_t ground_type;
    uint32_t texture3_ofs;
    uint32_t color2;
    uint32_t flags2;
    uint32_t _unk30[2];
    CTexture* texture1;
    CTexture* texture2;
};

static_assert(sizeof(SMOMaterial) == 0x40);

struct SMOPoly {
    enum Flags : uint8_t {
        eUnk01 = 0x1,
        eNoCamCollide = 0x2,
        eDetail = 0x4,
        eCollision = 0x8,
        eHint = 0x10,
        eRender = 0x20,
        eCullObjects = 0x40,
        eQueryVisited = 0x80,
    };

    uint8_t flags;
    uint8_t material_id;
};

static_assert(sizeof(SMOPoly) == 0x2);

struct SMOBatch {
    Vec3i16 bounds_min;
    Vec3i16 bounds_max;
    uint32_t start_index;
    uint16_t index_count;
    uint16_t min_index;
    uint16_t max_index;
    uint8_t flags;
    uint8_t material_id;
};

static_assert(sizeof(SMOBatch) == 0x18);

struct CAaBspNode {
    enum Flags : uint16_t {
        eAxisX = 0x0,
        eAxisY = 0x1,
        eAxisZ = 0x2,
        eAxisMask = 0x3,
        eLeaf = 0x4,
    };

    uint16_t flags;
    int16_t neg_child;
    int16_t pos_child;
    uint16_t face_count;
    uint32_t face_start;
    float plane_dist;
};

static_assert(sizeof(CAaBspNode) == 0x10);

class CAaBsp {
public:
    CAaBspNode* nodes_;
    CAaBspNode* _unk04;
    uint16_t* face_refs_;
    uint32_t node_count_;
    uint32_t face_ref_count_;
    uint8_t _unk14[0x34];
    uint32_t _unk48;
    AaBox bounds_;
};

static_assert(sizeof(CAaBsp) == 0x64);
