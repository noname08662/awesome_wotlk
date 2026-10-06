#pragma once

#include "BaseTypes.h"

#include "M2/M2Data.h"

class CM2Model;
class CShaderEffect;

struct M2Element {
    enum Type : uint32_t {
        eElementBatch = 0x0,
        eElementBatchProj = 0x1,
        eElementBatchDoodad = 0x2,
        eElementRibbon = 0x3,
        eElementParticle = 0x4,
        eElementCallback = 0x5,
    };

    Type type;
    CM2Model* model;
    uint32_t flags;
    float alpha;
    float sort_depth;
    float camera_dist_sq;
    int32_t index;
    int32_t doodad_instance_count;
    int doodad_group_key;
    int8_t priority_plane;
    char _pad[3];
    M2Batch* batch;
    M2SkinSection* skin_section;
    CShaderEffect* pixel_permute;
    uint32_t vertex_shader_index;
    uint32_t pixel_shader_index;
    uint32_t light_mode;
    uint32_t _unk40;
};

static_assert(sizeof(M2Element) == 0x44);
