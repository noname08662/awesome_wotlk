#pragma once

#include "Camera/CGCamera.h"
#include "FX/CShaderEffect.h"
#include "Graphics/CGxDevice.h"
#include "M2/M2Enums.h"
#include "M2/M2Sequence.h"
#include "Math/Primitives.h"

template <typename T>
struct M2Array {
    uint32_t count;

    union {
        T* data;
        uint32_t offset;
    };
};

struct M2SequenceTimes {
    M2Array<uint32_t> times;
};

struct M2TrackBase {
    enum M2TrackInterpolationType : uint16_t {
        eInterpNone = 0x0,
        eInterpLinear = 0x1,
        eInterpBezier = 0x2,
        eInterpHermite = 0x3,
    };

    M2TrackInterpolationType track_type;
    uint16_t loop_index;
    M2Array<M2SequenceTimes> sequence_times;
};

template <typename T>
struct M2SequenceKeys {
    M2Array<T> keys;
};

template <typename T>
struct M2Track : M2TrackBase {
    M2Array<M2SequenceKeys<T>> sequence_keys;
};

struct M2CompQuat {
    uint32_t raw_components[2];
};

template <typename T>
struct M2SplineKey {
    T value, in_tan, out_tan;
};

template <typename T>
struct M2ModelTrack {
    uint32_t current_key;
    uint32_t secondary_key;
    T current_val;
};

template <typename T>
struct M2PartTrack {
    M2Array<int16_t> times;
    M2Array<T> values;
};

struct M2Attachment {
    uint32_t attachment_id;
    uint16_t bone_index;
    char _pad[2];
    Vec3f position;
    M2Track<uint8_t> visibility_track;
};

static_assert(sizeof(M2Attachment) == 0x28);

struct M2Batch {
    M2BatchFlags flags;
    int8_t priority_plane;
    M2BatchShaderFlags shader_flags;
    uint16_t skin_section_index;
    uint16_t geoset_index;
    uint16_t color_index;
    uint16_t material_index;
    uint16_t material_layer;
    uint16_t texture_count;
    uint16_t texture_combo_index;
    uint16_t texture_coord_combo_index;
    uint16_t texture_weight_combo_index;
    uint16_t texture_transform_combo_index;
};

static_assert(sizeof(M2Batch) == 0x18);

struct M2Camera {
    uint32_t camera_id;
    float field_of_view;
    float far_clip;
    float near_clip;
    M2Track<M2SplineKey<Vec3f>> position_track;
    Vec3f position_pivot;
    M2Track<M2SplineKey<Vec3f>> target_track;
    Vec3f target_pivot;
    M2Track<M2SplineKey<float>> roll_track;
};

static_assert(sizeof(M2Camera) == 0x64);

struct M2Color {
    M2Track<Vec3f> color_track;
    M2Track<int16_t> alpha_track;
};

static_assert(sizeof(M2Color) == 0x28);

struct M2CompBone {
    struct CompressData {
        uint16_t u_dist_to_furth_desc;
        uint16_t u_z_ratio_of_chain;
    };

    uint32_t bone_id;
    uint32_t flags;
    uint16_t parent_index;
    uint16_t u_dist_to_parent;

    union {
        CompressData compress_data;
        uint32_t bone_name_crc;
    };

    M2Track<Vec3f> translation_track;
    M2Track<M2CompQuat> rotation_track;
    M2Track<Vec3f> scale_track;
    Vec3f pivot;
};

static_assert(sizeof(M2CompBone) == 0x58);

struct M2Event {
    uint32_t event_id;
    uint32_t data;
    uint16_t bone_index;
    char _pad[2];
    Vec3f position;
    M2TrackBase event_track;
};

static_assert(sizeof(M2Event) == 0x24);

struct M2Light {
    uint16_t light_type;
    uint16_t bone_index;
    Vec3f position;
    M2Track<Vec3f> ambient_color_track;
    M2Track<float> ambient_intensity_track;
    M2Track<Vec3f> diffuse_color_track;
    M2Track<float> diffuse_intensity_track;
    M2Track<float> attenuation_start_track;
    M2Track<float> attenuation_end_track;
    M2Track<uint8_t> visibility_track;
};

static_assert(sizeof(M2Light) == 0x9C);

struct M2Loop {
    uint32_t length;
};

static_assert(sizeof(M2Loop) == 0x4);

struct M2Material {
    uint16_t flags;
    uint16_t blend_mode;
};

static_assert(sizeof(M2Material) == 0x4);

struct M2Particle {
    uint32_t particle_id;
    uint32_t flags;
    Vec3f position;
    uint16_t bone_index;
    uint16_t texture_index;
    M2Array<uint8_t> geometry_mdl;
    M2Array<uint8_t> recursion_mdl;
    uint8_t blend_mode;
    uint8_t emitter_type;
    uint16_t color_index;
    uint16_t pad;
    int16_t priority_plane;
    uint16_t rows;
    uint16_t cols;
    M2Track<float> speed_track;
    M2Track<float> variation_track;
    M2Track<float> latitude_track;
    M2Track<float> longitude_track;
    M2Track<float> gravity_track;
    M2Track<float> life_track;
    float life_variation;
    M2Track<float> emission_rate_track;
    float emission_rate_variation;
    M2Track<float> width_track;
    M2Track<float> length_track;
    M2Track<float> zsource_track;
    M2PartTrack<Vec3f> color_track;
    M2PartTrack<int16_t> alpha_track;
    M2PartTrack<Vec2f> scale_track;
    Vec2f scale_variation;
    M2PartTrack<uint16_t> head_cell_track;
    M2PartTrack<uint16_t> tail_cell_track;
    float tail_length;
    float twinkle_fps;
    float twinkle_on_off;
    Vec2f twinkle_scale;
    float ivel_scale;
    float drag;
    float initial_spin;
    float initial_spin_variation;
    float spin;
    float spin_variation;
    AaBox tumble;
    Vec3f wind_vector;
    float wind_time;
    float follow_speed1;
    float follow_scale1;
    float follow_speed2;
    float follow_scale2;
    M2Array<Vec3f> spline;
    M2Track<uint8_t> visibility_track;
};

static_assert(sizeof(M2Particle) == 0x1DC);

struct M2Ribbon {
    uint32_t ribbon_id;
    uint16_t bone_index;
    char _pad1[2];
    Vec3f position;
    M2Track<uint16_t> texture_indices;
    M2Track<uint16_t> material_indices;
    M2Track<Vec3f> color_track;
    M2Track<int16_t> alpha_track;
    M2Track<float> height_above_track;
    M2Track<float> height_below_track;
    float edges_per_second;
    float edge_lifetime;
    float gravity;
    uint16_t texture_rows;
    uint16_t texture_cols;
    M2Track<uint16_t> texture_slot_track;
    M2Track<uint8_t> visibility_track;
    int16_t priority_plane;
    char _pad2[2];
};

static_assert(sizeof(M2Ribbon) == 0xC8);

struct M2Sequence {
    uint16_t id;
    uint16_t variation_index;
    uint32_t duration;
    float movespeed;
    uint32_t flags;
    uint32_t frequency;
    Vec2i replay;
    uint32_t blendtime;
    Bounds bounds;
    uint16_t variation_next;
    uint16_t alias_next;
};

static_assert(sizeof(M2Sequence) == 0x40);

struct M2SkinSection {
    uint32_t skin_section_id;
    uint16_t vertex_start;
    uint16_t vertex_count;
    uint16_t index_start;
    uint16_t index_count;
    uint16_t bone_count;
    uint16_t bone_combo_index;
    uint16_t bone_influences;
    uint16_t center_bone_index;
    Vec3f center_position;
    Vec3f sort_center_position;
    float sort_radius;
};

static_assert(sizeof(M2SkinSection) == 0x30);

struct M2Texture {
    uint32_t texture_id;
    uint16_t flags;
    char _pad[2];
    M2Array<uint8_t> filename;
};

static_assert(sizeof(M2Texture) == 0x10);

struct M2TextureTransform {
    M2Track<Vec3f> translation_track;
    M2Track<M2CompQuat> rotation_track;
    M2Track<Vec3f> scale_track;
};

static_assert(sizeof(M2TextureTransform) == 0x3C);

struct M2TextureWeight {
    M2Track<int16_t> weight_track;
};

static_assert(sizeof(M2TextureWeight) == 0x14);

struct M2Vertex {
    Vec3f position;
    Vec4u8 weights;
    Vec4u8 indices;
    Vec3f normal;
    Vec2f texcoord[2];
};

static_assert(sizeof(M2Vertex) == 0x30);

struct M2ModelColor {
    M2Track<Vec3f> color_track;
    M2ModelTrack<float> alpha_track;
};

static_assert(sizeof(M2ModelColor) == 0x20);

struct M2ModelTextureTransform {
    M2ModelTrack<Vec3f> translation_track;
    M2ModelTrack<Vec4f> rotation_track;
    M2ModelTrack<Vec3f> scale_track;
};

static_assert(sizeof(M2ModelTextureTransform) == 0x40);

struct M2ModelLight {
    M2ModelColor ambient;
    M2ModelColor diffuse;
    M2ModelTrack<float> attenuation_start;
    M2ModelTrack<float> attenuation_end;
    M2ModelTrack<uint8_t> visibility;
    int initialized;
    CM2Light light;
};

static_assert(sizeof(M2ModelLight) == 0xD4);

struct M2ModelBone {
    M2ModelTextureTransform texture_transform;
    CM2BoneSequenceState primary_sequence;
    CM2BoneSequenceState secondary_sequence;
    Mat4f* bone_matrix_override;
    uint16_t bone_flags;
    char _pad[2];
    uint32_t sequence_callback_arg;
    uint16_t pending_variant_chain_index;
    uint16_t sibling_next_index;
    void* sibling_prev_link;
    uint32_t blend_start_time;
    float blend_inv_rate;
    float blend_progress;
    float blend_weight;
};

static_assert(sizeof(M2ModelBone) == 0xAC);

struct M2ModelCamera {
    M2ModelTrack<Vec3f> position_track;
    M2ModelTrack<Vec3f> target_track;
    M2ModelTrack<float> roll_track;
    CGCamera* camera;
};

static_assert(sizeof(M2ModelCamera) == 0x38);

struct M2ModelRibbon {
    M2ModelTrack<Vec3f> color_track;
    M2ModelTrack<float> alpha_track;
    M2ModelTrack<float> height_above_track;
    M2ModelTrack<float> height_below_track;
    M2ModelTrack<uint16_t> texture_slot_track;
    M2ModelTrack<uint8_t> visible_track;
};

static_assert(sizeof(M2ModelRibbon) == 0x50);

struct M2ModelParticle {
    M2ModelTrack<float> speed_track;
    M2ModelTrack<float> variation_track;
    M2ModelTrack<float> latitude_track;
    M2ModelTrack<float> longitude_track;
    M2ModelTrack<float> gravity_track;
    M2ModelTrack<float> life_track;
    M2ModelTrack<float> emission_rate_track;
    M2ModelTrack<float> width_track;
    M2ModelTrack<float> length_track;
    M2ModelTrack<float> zsource_track;
    M2ModelTrack<uint8_t> visible_track;
    uint8_t should_render;
    uint8_t is_active;
    char _pad[2];
};

static_assert(sizeof(M2ModelRibbon) == 0x50);

struct M2ModelOptGeo {
    struct IndexPair {
        uint32_t created_source_index;
        uint32_t last_merged_source_index;
    };

    M2Batch* batches;
    uint32_t batch_count;
    M2SkinSection* skin_sections;
    uint32_t skin_section_count;
    IndexPair* section_index_map;
    CGxPool* pool;
    CGxBuf* buf;
    CShaderEffect** effects;
};

static_assert(sizeof(M2ModelOptGeo) == 0x20);

// .m2 file
struct M2Data {
    uint32_t md20;
    uint32_t version;
    M2Array<uint8_t> name;
    uint32_t flags;
    M2Array<M2Loop> loops;
    M2Array<M2Sequence> sequences;
    M2Array<uint16_t> sequence_idx_hash_by_id;
    M2Array<M2CompBone> bones;
    M2Array<uint16_t> bone_indices_by_id;
    M2Array<M2Vertex> vertices;
    uint32_t num_skin_profiles;
    M2Array<M2Color> colors;
    M2Array<M2Texture> textures;
    M2Array<M2TextureWeight> texture_weights;
    M2Array<M2TextureTransform> texture_transforms;
    M2Array<uint16_t> texture_indices_by_id;
    M2Array<M2Material> materials;
    M2Array<uint16_t> bone_combos;
    M2Array<uint16_t> texture_combos;
    M2Array<uint16_t> texture_coord_combos;
    M2Array<uint16_t> texture_weight_combos;
    M2Array<uint16_t> texture_transform_combos;
    Bounds bounds;
    Bounds collision_bounds;
    M2Array<uint16_t> collision_indices;
    M2Array<Vec3f> collision_positions;
    M2Array<Vec3f> collision_face_normals;
    M2Array<M2Attachment> attachments;
    M2Array<uint16_t> attachment_indices_by_id;
    M2Array<M2Event> events;
    M2Array<M2Light> lights;
    M2Array<M2Camera> cameras;
    M2Array<uint16_t> camera_indices_by_id;
    M2Array<M2Ribbon> ribbons;
    M2Array<M2Particle> particles;
    M2Array<uint16_t> texture_combiner_combos;
};

static_assert(sizeof(M2Data) == 0x138);

// .skin file
struct M2SkinProfile {
    uint32_t magic;
    M2Array<uint16_t> vertices;
    M2Array<uint16_t> indices;
    M2Array<Vec4u8> bones;
    M2Array<M2SkinSection> skin_sections;
    M2Array<M2Batch> batches;
    uint32_t bone_count_max;
};

static_assert(sizeof(M2SkinProfile) == 0x30);
