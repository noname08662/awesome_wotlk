#pragma once

class CMapEntity;

struct Shard {
    TSLink<Shard> link;
    Vec3f pos;
    CM2Model* model;
    unk_t unk;
    uint8_t flag;
    char _pad[3];
};

static_assert(sizeof(Shard) == 0x20);

struct BlizzardObject {
    TSLink<BlizzardObject> link;
    char model_path[260];
    CMapEntity* map_entity;
    AaSphere sphere;
    float spawn_accum;
    float spawn_rate;
    unk_t unk[4];
    TSExplicitList<Shard> shard_link;
    Mat4f transform_mat;
    int attached;
};

static_assert(sizeof(BlizzardObject) == 0x188);
