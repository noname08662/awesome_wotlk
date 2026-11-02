#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <span>

#include "Lib/Storm.h"
#include "Math/Primitives.h"

struct SMChunk {
    uint32_t flags;
    uint32_t index_x;
    uint32_t index_y;
    uint32_t n_layers;
    uint32_t n_doodad_refs;
    uint32_t ofs_height;
    uint32_t ofs_normal;
    uint32_t ofs_layer;
    uint32_t ofs_refs;
    uint32_t ofs_alpha;
    uint32_t size_alpha;
    uint32_t ofs_shadow;
    uint32_t size_shadow;
    uint32_t area_id;
    uint32_t n_map_obj_refs;
    uint16_t holes_low_res;
    uint16_t _unk3E;
};

static_assert(sizeof(SMChunk) == 0x40);

// inclusive chunk indices
struct ChunkRange {
    int32_t x_min;
    int32_t y_min;
    int32_t x_max;
    int32_t y_max;

    constexpr bool intersects(const ChunkRange& other) const {
        return x_min <= other.x_max && other.x_min <= x_max && y_min <= other.y_max && other.y_min <= y_max;
    }

    constexpr ChunkRange clipped(const ChunkRange& other) const {
        return {std::max(x_min, other.x_min), std::max(y_min, other.y_min), std::min(x_max, other.x_max),
            std::min(y_max, other.y_max)};
    }

    // the CMapArea grid indices it touches, clamped to the grid
    constexpr ChunkRange areas() const {
        return {std::max(x_min >> 4, 0), std::max(y_min >> 4, 0), std::min(x_max >> 4, 63), std::min(y_max >> 4, 63)};
    }
};

class CMapChunk {
public:
    // the chunk grid: 16 x 16 chunks per area, 64 x 64 areas, the indices rising as world x and y fall
    static constexpr float kSize = 33.333332f;       // 533.333 / 16
    static constexpr float kInvSize = 0.029999999f;  // 1 / kSize as the engine stores it
    static constexpr float kGridHalf = 17066.666f;   // 32 areas
    static constexpr uint32_t kOuterRowVertices = 9;
    static constexpr uint32_t kRowStride = 2 * kOuterRowVertices - 1;
    static constexpr uint32_t kVertexCount = kOuterRowVertices + (kOuterRowVertices - 1) * kRowStride;
    static constexpr uint32_t kCornerVertex[4] = {
        0, kOuterRowVertices - 1, kVertexCount - kOuterRowVertices, kVertexCount - 1};

    // z unused
    inline static const auto& kVertexOffsets = *reinterpret_cast<const Vec3f (*)[kVertexCount]>(0x00D25498);

    uint8_t _unk00[0x3C];
    AaSphere sphere_;
    AaBox bounds_;
    uint8_t _unk64[0x18];
    Vec3f origin_;
    float depth_;
    AaBox detail_bounds_;
    uint8_t _unkA4[0x18];
    TSLink<CMapChunk> bucket_link_;
    uint8_t _unkC4[0x4C];
    SMChunk* header_;
    uint8_t _unk114[0x8];
    float* heights_;  // kVertexCount

    // MCNR, z up, 127 = 1.0
    struct Normal {
        int8_t x;
        int8_t y;
        int8_t z;
    };

    const void* vertex_colors_;  // MCCV
    const Normal* normals_;      // kVertexCount, alive as long as heights_

    using VertexHeights = std::span<const float, CMapChunk::kVertexCount>;
    using VertexOffsets = std::span<const Vec3f, CMapChunk::kVertexCount>;

    struct Chunk {
        Vec3f origin;
        float depth{};
        VertexHeights heights;
        uint16_t holes{};
    };

    struct Edge {
        Vec3f a;
        Vec3f b;
    };

    VertexHeights chunkHeights() const { return VertexHeights(heights_, CMapChunk::kVertexCount); }

    static Vec3f chunkPoint(const Vec3f& origin, VertexHeights heights, VertexOffsets offsets, uint32_t index) {
        const auto r = [](float f) { return static_cast<double>(f); };
        return {static_cast<float>(r(offsets[index].x) + r(origin.x)),
            static_cast<float>(r(offsets[index].y) + r(origin.y)), static_cast<float>(r(heights[index]) + r(origin.z))};
    }

    // quadrant: world_scene::view_quadrant or view_quadrant_opposite
    Vec3f chunkCorner(int32_t quadrant) const {
        return chunkPoint(origin_, chunkHeights(), CMapChunk::kVertexOffsets, CMapChunk::kCornerVertex[quadrant]);
    }
};

static_assert(sizeof(CMapChunk::Normal) == 3);
static_assert(sizeof(CMapChunk) == 0x128);
static_assert(std::bit_cast<uint32_t>(CMapChunk::kSize) == 0x42055555);
static_assert(std::bit_cast<uint32_t>(CMapChunk::kInvSize) == 0x3CF5C28F);
static_assert(std::bit_cast<uint32_t>(CMapChunk::kGridHalf) == 0x46855555);
static_assert(CMapChunk::kVertexCount == 145 && CMapChunk::kCornerVertex[2] == 136);

class CMapArea {
public:
    uint8_t _unk00[0x24];
    AaBox bounds_;
    uint8_t _unk3C[0xC];
    int32_t area_y_;
    int32_t area_x_;
    int32_t chunk_y_;
    int32_t chunk_x_;
    uint8_t _unk58[0x18];
    void* async_object_;
    uint8_t _unk74[0xC];
    int32_t loaded_;
    uint8_t _unk84[0x38];
    CMapChunk* chunks_[16][16];

    // [x][y]
    inline static const auto& kMapAreas = *reinterpret_cast<CMapArea* const (*)[64][64]>(0x00CE48D0);

    bool loading() const { return async_object_ != nullptr; }

    // global chunk indices
    CMapChunk* chunkAt(int32_t cx, int32_t cy) const { return chunks_[cx & 15][cy & 15]; }
};

static_assert(sizeof(CMapArea) == 0x4BC);
