#pragma once

#include <hookkit/hookkit.h>

#include <cstdint>
#include <span>

#include "Graphics/CGxDevice.h"
#include "Map/CMapChunk.h"
#include "Math/Primitives.h"

struct ProjectedVertex {
    Vec3f pos;
    uint32_t color;
};

static_assert(sizeof(ProjectedVertex) == 0x10);

struct ProjectedBatch {
    const Mat4f* transform;  // vertices to world
    const Vec3f* vertices;
    const Vec3f* normals;  // WMO only
    const float* heights;  // terrain only
    const uint16_t* indices;
    const uint16_t* triangle_ids;
    uint16_t index_count;
    uint16_t triangle_count;
    uint16_t min_vertex;
    uint16_t max_vertex;  // inclusive, folded from every index written
    CMapChunk* owner;     // terrain

    [[nodiscard]]
    bool terrain() const {
        return heights != nullptr;
    }

    [[nodiscard]]
    std::span<const Vec3f> vertexSpan() const {
        return {vertices, terrain() ? CMapChunk::kVertexCount : max_vertex + 1u};
    }

    [[nodiscard]]
    std::span<const float> heightSpan() const {
        return {heights, terrain() ? CMapChunk::kVertexCount : 0u};
    }

    // the draw loop skips the rest
    [[nodiscard]]
    bool drawable() const {
        return 3u * index_count <= 0x10000u && transform != nullptr && vertices != nullptr && indices != nullptr;
    }
};

static_assert(sizeof(ProjectedBatch) == 0x24);

enum ProjectedVtxMode : uint32_t {
    eProjectedVtxReceiveM2 = 0x1,
    eProjectedVtxLit = 0x2,
    eProjectedVtxAllWindings = 0x4  // skips the XY winding filter on WMO triangles
};

namespace projected_texture {
inline constexpr uint32_t kMaxBatches = 32;
inline constexpr float kReticleHalf = 2.0f;  // the reticle's own box above and below its center
inline constexpr float kRampPerFullAlpha = 1.5f;

inline constexpr uintptr_t kM2DecalPadRefAddr = 0x0082080B;
inline constexpr uintptr_t kM2DecalPadAddr = 0x00A4040C;      // 2.0f
inline constexpr uintptr_t kM2DecalPadWideAddr = 0x00A393A8;  // 50.0f

// the gather's result for the decal being drawn
inline const ProjectedBatch (&batches)[kMaxBatches] = *reinterpret_cast<const ProjectedBatch (*)[kMaxBatches]>(
    0x00CD8080);
inline uint32_t& batch_count = *reinterpret_cast<uint32_t*>(0x00CB752C);
inline uint32_t& batch_overflow = *reinterpret_cast<uint32_t*>(0x00CB7528);

[[nodiscard]]
inline std::span<const ProjectedBatch> gatheredBatches() {
    return std::span(batches).first(batch_count);
}

[[nodiscard]]
inline bool isAoeProj(const Mat4f* matrix, char vtx_mode) {
    return matrix == nullptr && vtx_mode == (eProjectedVtxReceiveM2 | eProjectedVtxAllWindings);
}

inline constexpr hookkit::WildAbi<4> kBuildUnlitVerticesAbi = {{{hookkit::ArgLoc::inReg(hookkit::Reg::eAx),
    hookkit::ArgLoc::onStack(0), hookkit::ArgLoc::onStack(4), hookkit::ArgLoc::onStack(8)}}};
// batch, color, gx batch, vtx mode
HOOKKIT_HOOK_WILD_HANDLE(buildUnlitVertices, 0x007E32F0, hookkit::Conv::eUsercall, int, kBuildUnlitVerticesAbi,
    const ProjectedBatch*, uint32_t, CGxBatch*, uint32_t);

// a batch's corners in world space
class BatchPoints {
public:
    explicit BatchPoints(const ProjectedBatch& batch)
        : transform_(batch.transform), vertices_(batch.vertexSpan()), heights_(batch.heightSpan()) {}

    [[nodiscard]]
    Vec3f operator[](uint16_t index) const {
        const Mat4f& m = *transform_;
        const Vec3f& p = vertices_[index];
        if (!heights_.empty()) { return {p.x + m.m[3][0], p.y + m.m[3][1], heights_[index] + m.m[3][2]}; }
        return {p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0],
            p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1],
            p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2]};
    }

private:
    const Mat4f* transform_;
    std::span<const Vec3f> vertices_;
    std::span<const float> heights_;  // terrain only
};
}  // namespace projected_texture
