#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Lib/Storm.h"
#include "Map/CMapBaseObj.h"
#include "Map/MapObjData.h"
#include "Map/WorldScene.h"
#include "Math/Primitives.h"

class CMapObj;

// a WMO root's portal polygon
struct MapObjPortal {
    uint16_t first_vertex;
    uint16_t vertex_count;
    Plane plane;
};

static_assert(sizeof(MapObjPortal) == 0x14);

struct MapObjPortalRef {
    static constexpr uint16_t kNoGroup = 0xFFFF;

    uint16_t portal_index;
    uint16_t target_group;  // kNoGroup: skipped
    int16_t side;           // negative flips the portal plane
    uint16_t _pad06;
};

static_assert(sizeof(MapObjPortalRef) == 0x8);

// a portal's projected rect, cached per portal index
struct PortalRectEntry {
    enum Flags : uint16_t {
        eEmpty = 0x1,          // clipped away or occluded
        eStraddles = 0x2,      // the camera stands in the portal, the rect is the full screen
        eExteriorAdded = 0x4,  // AddExteriorPortal ran for it this stamp
        eEntryAdded = 0x8,     // AddEntryPortal ran for it this stamp
        eInteriorOnly = 0x10,  // neither group is exterior, the occluder test is skipped
    };

    uint16_t flags;
    uint16_t _pad02;
    Rectf rect;  // ndc
    uint32_t stamp;
    uint32_t _unk18;
};

static_assert(sizeof(PortalRectEntry) == 0x1C);

// a root's per-group record (MOGI)
struct MapObjGroupInfo {
    GroupFlags flags;
    AaBox bounds;
    uint32_t name_offset;  // MOGN name offset, -1 for none
};

static_assert(sizeof(MapObjGroupInfo) == 0x20);

// the top edge of one of a WMO group's antiportal triangles, in group space
struct MapObjAntiportalEdge {
    uint32_t _unk00;
    Vec3f a;
    Vec3f b;
    TSLink<MapObjAntiportalEdge> link;
};

static_assert(sizeof(MapObjAntiportalEdge) == 0x24);

// a WMO group file, shared by every placement of its root
class CMapObjGroup {
public:
    enum LoadFlags : uint32_t {
        eLoaded = 0x1,
        eColorsAttenuated = 0x2,  // also set with SMOHeader::eNoPortalAttenuation
        eAllBatchesOpaque = 0x4,  // every batch's material is eGxBlendOpaque
        eHasShader6Batch = 0x8,   // a batch's material has shader 6, 48-byte vertices over 36
    };

    uint32_t _unk000;
    // made on first draw, dropped once gpu_idle_time_ passes 5 s; the CPU arrays below stay
    void* gpu_vertex_buf_;
    void* gpu_buf08_;      // a second vertex stream, only with shaders off + the unified path + MOCV
    void* gpu_index_buf_;  // its +0x18 is the CGxBuf
    uint8_t _unk010[0x8];
    float gpu_idle_time_;
    void* liquid_vert_array_;  // liquid_verts_x_ * liquid_verts_y_
    uint8_t _unk020[0x10];
    GroupFlags flags_;  // MOGP
    AaBox bbox_;        // group space
    uint32_t _unk04C;
    uint32_t portal_ref_start_;  // into the root's portal_refs_
    uint32_t portal_ref_count_;
    uint8_t fog_ids_[4];  // MFOG
    // the batches are ordered transition, interior, exterior; a group without MOCV draws [0, ext_batch_count_), the
    // others [0, batch_count_)
    uint16_t trans_batch_count_;
    uint16_t int_batch_count_;
    uint16_t ext_batch_count_;
    uint16_t _unk062;
    CAaBsp bsp_;  // eGroupHasBsp
    uint8_t _unk0C8[0x10];
    const char* name_;  // into the root's group_names_
    // the chunk arrays, pointers into file_buf_; counts are element counts
    SMOPoly* polys_;           // MOPY, one per triangle
    uint16_t* indices_;        // MOVI
    uint16_t* strip_indices_;  // MORI (eGroupHasStrips)
    Vec3f* vertices_;          // MOVT, group space
    Vec3f* normals_;           // MONR
    Vec2f* tex_coords_;        // MOTV
    Vec2f* tex_coords2_;       // the second MOTV (eGroupHasTexCoords2)
    SMOBatch* batches_;        // MOBA
    void* strip_batches_;      // MORB, 8 bytes per batch
    uint16_t* light_refs_;     // MOLR
    uint16_t* doodad_refs_;    // MODR
    ColorBGRA<>* colors_;      // MOCV
    ColorBGRA<>* colors2_;     // the second MOCV
    uint32_t _unk110;
    uint32_t liquid_verts_x_;  // MLIQ (eGroupHasLiquid)
    uint32_t liquid_verts_y_;
    uint32_t liquid_tiles_x_;
    uint32_t liquid_tiles_y_;
    Vec3f liquid_corner_;
    uint32_t liquid_material_;
    void* liquid_verts_;     // 8 bytes each
    uint8_t* liquid_tiles_;  // low nibble 0xF: no liquid
    uint8_t _unk13C[0x8];
    uint32_t liquid_type_;
    uint8_t _unk148[0x8];
    uint32_t poly_count_;
    uint32_t index_count_;
    uint32_t strip_index_count_;
    uint32_t vertex_count_;
    uint32_t normal_count_;
    uint32_t tex_coord_count_;
    uint32_t tex_coord2_count_;
    uint32_t batch_count_;
    uint32_t light_ref_count_;
    uint32_t doodad_ref_count_;
    uint32_t color_count_;
    uint32_t color2_count_;
    uint32_t group_id_;  // MOGP uniqueID (WMOAreaTable)
    void* file_buf_;     // the group file, MOGP data at +0x14
    uint32_t _unk188;
    CMapObj* root_;
    uint32_t _unk190;
    void* pending_read_;  // the group file's async read, null once it landed
    LoadFlags load_flags_;
    // the index and vertex range of the leading opaque batches, the whole group's with eAllBatchesOpaque
    uint32_t batch_first_index_;
    uint32_t batch_last_index_;
    uint16_t batch_min_vertex_;
    uint16_t batch_max_vertex_;
    TSExplicitList<MapObjAntiportalEdge> antiportal_edges_;
    TSLink<CMapObjGroup> loaded_link_;  // in the root's loaded_groups_

    // a group file landed: maps its chunks, sets load_flags_ and links it into the root's loaded_groups_
    HOOKKIT_HOOK(submitLoad, 0x007D82E0, hookkit::Conv::eThiscall, uintptr_t, CMapObjGroup*);
    // group; returns it to the WMAPOBJGROUP pool
    HOOKKIT_HOOK_HANDLE(free, 0x007C0030, hookkit::Conv::eCdecl, void, CMapObjGroup*);
};

static_assert(offsetof(CMapObjGroup, gpu_idle_time_) == 0x18);
static_assert(offsetof(CMapObjGroup, flags_) == 0x30);
static_assert(offsetof(CMapObjGroup, bbox_) == 0x34);
static_assert(offsetof(CMapObjGroup, portal_ref_start_) == 0x50);
static_assert(offsetof(CMapObjGroup, trans_batch_count_) == 0x5C);
static_assert(offsetof(CMapObjGroup, bsp_) == 0x64);
static_assert(offsetof(CMapObjGroup, name_) == 0xD8);
static_assert(offsetof(CMapObjGroup, polys_) == 0xDC);
static_assert(offsetof(CMapObjGroup, vertices_) == 0xE8);
static_assert(offsetof(CMapObjGroup, batches_) == 0xF8);
static_assert(offsetof(CMapObjGroup, colors_) == 0x108);
static_assert(offsetof(CMapObjGroup, liquid_type_) == 0x144);
static_assert(offsetof(CMapObjGroup, poly_count_) == 0x150);
static_assert(offsetof(CMapObjGroup, batch_count_) == 0x16C);
static_assert(offsetof(CMapObjGroup, group_id_) == 0x180);
static_assert(offsetof(CMapObjGroup, root_) == 0x18C);
static_assert(offsetof(CMapObjGroup, pending_read_) == 0x194);
static_assert(offsetof(CMapObjGroup, load_flags_) == 0x198);
static_assert(offsetof(CMapObjGroup, antiportal_edges_) == 0x1A8);
static_assert(offsetof(CMapObjGroup, loaded_link_) == 0x1B4);
static_assert(sizeof(CMapObjGroup) == 0x1BC);

// WMO root; freed once ref_count_ has sat at 0 for 10 s
class CMapObj {
public:
    // the portal walk's tolerances
    static constexpr float kPortalPlaneEps = 0.0099999998f;
    static constexpr float kThinPortalRect = 0.001f;  // the walk skips a clipped portal rect thinner than this (ndc)

    uint8_t _unk000[0x1C];
    char name_[260];  // the file path
    // the chunk arrays, pointers into file_buf_
    SMOHeader* header_;             // MOHD
    const char* texture_names_;     // MOTX, texture_names_size_ bytes
    const char* group_names_;       // MOGN, group_names_size_ bytes
    void* skybox_;                  // MOSB, null when empty
    MapObjGroupInfo* group_infos_;  // MOGI, group_count_
    Vec3f* portal_vertices_;        // MOPV
    MapObjPortal* portals_;         // MOPT
    MapObjPortalRef* portal_refs_;  // MOPR
    Vec3f* visible_verts_;          // MOVV
    void* visible_blocks_;          // MOVB
    void* lights_;                  // MOLT, 0x30 each
    void* doodad_sets_;             // MODS, 0x20 each
    const char* doodad_names_;      // MODN, doodad_names_size_ bytes
    void* doodad_defs_;             // MODD, 0x28 each
    void* fogs_;                    // MFOG, 0x30 each
    void* convex_planes_;           // MCVP (optional), 0x10 each
    SMOMaterial* materials_;        // MOMT
    uint32_t texture_names_size_;
    uint32_t group_names_size_;
    uint32_t group_count_;
    uint32_t portal_vertex_count_;
    uint32_t portal_count_;
    uint32_t portal_ref_count_;
    uint32_t visible_vert_count_;
    uint32_t visible_block_count_;
    uint32_t light_count_;
    uint32_t doodad_set_count_;
    uint32_t doodad_names_size_;
    uint32_t doodad_def_count_;
    uint32_t fog_count_;
    uint32_t convex_plane_count_;
    uint32_t material_count_;
    ColorBGRA<> ambient_color_;  // from header_
    uint32_t _unk1A4;
    AaBox bbox_;  // from header_
    float _unk1C0;
    TSLink<CMapObj> pending_link_;  // in the pending-roots list (0x00ADFC4C)
    void* file_buf_;
    uint32_t file_size_;
    uint32_t ref_count_;
    float unref_time_;  // seconds at ref_count_ 0
    void* async_read_;  // CAsyncObject: the root file's read
    uint32_t loaded_;
    uint16_t walked_group_count_;  // groups without kGroupOutdoor minus 1, 0xFFFF for none
    uint16_t _unk1E6;
    TSExplicitList<CMapObjGroup> loaded_groups_;  // through CMapObjGroup::loaded_link_
    uint32_t group_slot_count_;
    CMapObjGroup* groups_[512];

    // getGroupForced with force 0
    CMapObjGroup* loadedGroup(uint32_t index) const {
        if (loaded_ == 0) { return nullptr; }
        CMapObjGroup* group = groups_[index];
        return (group->load_flags_ & CMapObjGroup::eLoaded) != 0 ? group : nullptr;
    }

    uint32_t groupFlags(uint32_t index) const { return loaded_ != 0 ? group_infos_[index].flags : 0; }

    std::span<const Vec3f> portalVerts(const MapObjPortal& portal) const {
        return {portal_vertices_ + portal.first_vertex, portal.vertex_count};
    }

    // local to world, world to local, camera pos, camera target; also sets the device world matrix
    HOOKKIT_HOOK(setupTraversalXforms, 0x007A6E00, hookkit::Conv::eThiscall, void, CMapObj*, const Mat4f*, const Mat4f*,
        const Vec3f*, const Vec3f*);
    // portal, entry
    HOOKKIT_HOOK(
        computePortalRect, 0x007A9090, hookkit::Conv::eThiscall, void, CMapObj*, const MapObjPortal*, PortalRectEntry*);
    // local to world, world to local, camera pos, camera target, camera groups
    HOOKKIT_HOOK(traverseFromCamera, 0x007AD1F0, hookkit::Conv::eThiscall, void, CMapObj*, const Mat4f*, const Mat4f*,
        const Vec3f*, const Vec3f*, const TSGrowableArray<uint32_t>*);
    // local to world, world to local, camera pos, camera target, parent rect (ndc), group
    HOOKKIT_HOOK(traverseFromExterior, 0x007AD350, hookkit::Conv::eThiscall, void, CMapObj*, const Mat4f*, const Mat4f*,
        const Vec3f*, const Vec3f*, const Rectf*, uint32_t);
    // group
    HOOKKIT_HOOK(visitGroup, 0x007A6B60, hookkit::Conv::eThiscall, void, CMapObj*, uint32_t);
    // group
    HOOKKIT_HOOK(getGroupFlags, 0x007AE7B0, hookkit::Conv::eThiscall, uint32_t, CMapObj*, uint32_t);
    // group, force; null while the root isn't loaded, and while the group isn't unless force is set
    HOOKKIT_HOOK(getGroupForced, 0x007AEA80, hookkit::Conv::eThiscall, CMapObjGroup*, CMapObj*, uint32_t, int);
    // root
    HOOKKIT_HOOK_HANDLE(free, 0x007BFF70, hookkit::Conv::eCdecl, void, CMapObj*);
};

static_assert(offsetof(CMapObj, name_) == 0x1C);
static_assert(offsetof(CMapObj, header_) == 0x120);
static_assert(offsetof(CMapObj, skybox_) == 0x12C);
static_assert(offsetof(CMapObj, portal_vertices_) == 0x134);
static_assert(offsetof(CMapObj, portal_refs_) == 0x13C);
static_assert(offsetof(CMapObj, materials_) == 0x160);
static_assert(offsetof(CMapObj, group_count_) == 0x16C);
static_assert(offsetof(CMapObj, material_count_) == 0x19C);
static_assert(offsetof(CMapObj, bbox_) == 0x1A8);
static_assert(offsetof(CMapObj, file_buf_) == 0x1CC);
static_assert(offsetof(CMapObj, ref_count_) == 0x1D4);
static_assert(offsetof(CMapObj, loaded_) == 0x1E0);
static_assert(offsetof(CMapObj, walked_group_count_) == 0x1E4);
static_assert(offsetof(CMapObj, loaded_groups_) == 0x1E8);
static_assert(offsetof(CMapObj, groups_) == 0x1F8);
static_assert(sizeof(CMapObj) == 0x9F8);
static_assert(std::bit_cast<uint32_t>(CMapObj::kPortalPlaneEps) == 0x3C23D70A);

// one placed WMO group
class CMapObjDefGroup : public CMapBaseObj {
public:
    AaBox bounds_;
    AaSphere sphere_;
    float view_depth_;  // view_plane . the bounds corner nearest along the view
    uint32_t group_index_;
    uint8_t _unk54[0x14];
    void* liquid_proc_;                   // CLiquidProc
    TSExplicitList<CWFrustum> frustums_;  // one per portal path that reached it this frame, from frustum_pool
    TSExplicitList<CMapBaseObjLink<>> doodads_;
    TSExplicitList<CMapEntityLink> entities_;
    TSExplicitList<CMapBaseObjLink<>> lights_;
    uint8_t _unk9C[0x0C];
    TSLink<CMapObjDefGroup> bucket_link_;   // in a distance bucket's wmo_groups or unbucketed_exterior_groups_
    TSLink<CMapObjDefGroup> visited_link_;  // in visited_portal_groups_; next != 0: visited this frame
    TSLink<CMapObjDefGroup> liquid_link_;   // in visited_liquid_groups_

    CMapObjDef* def() const;
};

static_assert(offsetof(CMapObjDefGroup, bounds_) == 0x24);
static_assert(offsetof(CMapObjDefGroup, sphere_) == 0x3C);
static_assert(offsetof(CMapObjDefGroup, view_depth_) == 0x4C);
static_assert(offsetof(CMapObjDefGroup, group_index_) == 0x50);
static_assert(offsetof(CMapObjDefGroup, frustums_) == 0x6C);
static_assert(offsetof(CMapObjDefGroup, doodads_) == 0x78);
static_assert(offsetof(CMapObjDefGroup, entities_) == 0x84);
static_assert(offsetof(CMapObjDefGroup, lights_) == 0x90);
static_assert(offsetof(CMapObjDefGroup, bucket_link_) == 0xA8);
static_assert(offsetof(CMapObjDefGroup, visited_link_) == 0xB0);
static_assert(offsetof(CMapObjDefGroup, liquid_link_) == 0xB8);
static_assert(sizeof(CMapObjDefGroup) == 0xC0);

// one placed WMO
class CMapObjDef : public CMapBaseObj {
public:
    // light_flags_ as read for a def
    enum DefFlags : uint32_t {
        eDefBucketGroups = 0x80,   // its loaded exterior groups are bucketed for the outdoor cull
        eDefSkipBucket = 0x20,     // ... unless this is set
        eDefObjectPlaced = 0x400,  // transports, elevators; exterior groups unbucketed
    };

    // every placed WMO
    inline static auto& TSHT = *reinterpret_cast<TSHashTable<CMapObjDef>*>(0x00D25434);

    uint8_t _unk24[0x4C];
    Mat4f transform_;
    Mat4f inv_transform_;
    uint8_t _unkF0[0x04];
    CMapObj* root_;
    uint8_t _unkF8[0x1C];
    TSExplicitList<CMapObjDefGroupLink> group_links_;

    union {
        CMapObjDefGroup* groups_inline_[4];         // while groups_mode_ != -1
        TSGrowableArray<CMapObjDefGroup*> groups_;  // while groups_mode_ == -1
    };

    int32_t groups_mode_;  // the inline count, -1 while groups_ holds them

    bool bucketsGroups() const {
        const auto flags = static_cast<uint32_t>(light_flags_);
        return (flags & eDefBucketGroups) != 0 && (flags & eDefSkipBucket) == 0;
    }

    bool objectPlaced() const { return (static_cast<uint32_t>(light_flags_) & eDefObjectPlaced) != 0; }

    CMapObjDefGroup* group(uint32_t index) const { return groups_mode_ == -1 ? groups_[index] : groups_inline_[index]; }

    // bucket; TSHT's loads and, with bucket, each loaded group's bucket step and occluder edges
    HOOKKIT_HOOK_HANDLE(updateGroupLoads, 0x007B6110, hookkit::Conv::eCdecl, void, int);
};

static_assert(offsetof(CMapObjDef, transform_) == 0x70);
static_assert(offsetof(CMapObjDef, inv_transform_) == 0xB0);
static_assert(offsetof(CMapObjDef, root_) == 0xF4);
static_assert(offsetof(CMapObjDef, group_links_) == 0x114);
static_assert(offsetof(CMapObjDef, groups_inline_) == 0x120);
static_assert(offsetof(CMapObjDef, groups_mode_) == 0x130);

inline CMapObjDef* CMapObjDefGroup::def() const { return static_cast<CMapObjDef*>(list_.head()->owner); }

namespace map_obj {
using GroupVisitor = void(__cdecl*)(uint32_t group_index, CMapObjDef* def);

// the portal walk calls this for every group it reaches, with that path's frustum on top of the stack
inline auto& group_visitor = *reinterpret_cast<GroupVisitor*>(0x00D1BED8);
inline auto& group_visitor_def = *reinterpret_cast<CMapObjDef**>(0x00D1BEDC);
inline auto& traversal_def = *reinterpret_cast<CMapObjDef**>(0x00D1C420);
inline auto& stamped_def = *reinterpret_cast<CMapObjDef**>(0x00D1C41C);
inline auto& portal_rect_stamp = *reinterpret_cast<uint32_t*>(0x00D1C424);  // cached portal rects are keyed on it
using PortalRectCache = PortalRectEntry[2048];
inline auto& portal_rect_cache = *reinterpret_cast<PortalRectCache*>(0x00ADFF54);

// setupTraversalXforms' outputs for the walked WMO
struct TraversalXforms {
    Mat4f local_to_clip;  // local_to_camera * view_proj
    Mat4f view_proj;      // camera-relative: apply to world - camera_pos
    Mat4f local_to_view;  // local_to_camera * the device view matrix
    Mat4f local_to_world;
};

struct TraversalCamera {
    Vec3f camera_local;
    Vec3f target_local;
    Vec3f view_dir_local;  // normalized unless shorter than 1e-2
    float view_plane_d_local;
};

static_assert(sizeof(TraversalXforms) == 0x100);
static_assert(sizeof(TraversalCamera) == 0x28);

inline auto& traversal_xforms = *reinterpret_cast<TraversalXforms*>(0x00ADFE50);
inline auto& traversal_camera = *reinterpret_cast<TraversalCamera*>(0x00D1C42C);
inline auto& xforms_ready = *reinterpret_cast<uint32_t*>(0x00CFBEBC);  // never cleared

inline auto& walk_from_camera = *reinterpret_cast<uint32_t*>(0x00CFBEC0);
// the interior path of the group being visited
inline auto& walk_interior_path = *reinterpret_cast<uint32_t*>(0x00CFBEB8);
inline auto& max_portal_depth = *reinterpret_cast<uint32_t*>(0x00D1BEE4);  // deeper paths are cut
// the entry portals' projected verts
inline auto& entry_portal_verts = *reinterpret_cast<TSGrowableArray<Vec3f>*>(0x00D1BEE8);

// the portal projection's and clipper's function statics
inline auto& project_portal_verts = *reinterpret_cast<Vec3f (*)[12]>(0x00D1C308);
inline auto& clip_portal_bufs = *reinterpret_cast<Vec3f (*)[2][16]>(0x00D1C180);  // ping-pong
inline auto& clip_portal_class = *reinterpret_cast<int32_t (*)[16]>(0x00D1C100);
inline auto& clip_portal_dist = *reinterpret_cast<float (*)[16]>(0x00D1C140);

// visitor, def
HOOKKIT_HOOK_HANDLE(setGroupVisitor, 0x007A6B40, hookkit::Conv::eCdecl, void, GroupVisitor, CMapObjDef*);
}  // namespace map_obj
