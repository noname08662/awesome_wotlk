#pragma once

#include <d3d9.h>
#include <hookkit/hook.h>

#include "Graphics/GraphicsEnums.h"
#include "Lib/Storm.h"

// what IShaderLoad reads ahead of the records of a path\<profile>\name.bls
struct CGxShaderFileHeader {
    static constexpr uint32_t kMagic = 0x47585348;  // 'GXSH'
    static constexpr uint32_t kVersion = 0x10003;

    uint32_t magic;
    uint32_t version;
    uint32_t record_count;  // read but ignored: IShaderLoad hands the records to shaders[0..count) in order
};

static_assert(sizeof(CGxShaderFileHeader) == 0xC);

class CGxShader {
public:
    inline static auto& font_pixel_shader = *reinterpret_cast<CGxShader**>(0x00C7D2CC);
    inline static auto& font_vertex_shader = *reinterpret_cast<CGxShader**>(0x00C7D2D0);

    using Load = bool(__thiscall*)(CGxShader*, void*);
    Load* load_;  // vtable

    TSLink<CGxShader> link_hash_;
    TSLink<CGxShader> link_full_;
    uint32_t hash_key_;
    char* name_;
    uint32_t ref_count_;
    IUnknown* shader_;
    GxShaderType type_;
    uint32_t is_loaded_;
    uint32_t compile_flags_;
    uint32_t is_created_;
    uint32_t _unk[2];
    uint32_t sampler_types_;
    uint32_t constant_count_;
    uint16_t active_sampler_mask_;
    uint16_t param_mask_;
    TSGrowableArray<uint8_t> byte_code_;

    // file (storm handle): reads one .bls record into the node, sampler_types_, constant_count_, active_sampler_mask_,
    // param_mask_, then a u32 size and the bytecode padded to 4 bytes; the only vtable entry (load_)
    HOOKKIT_HOOK(load, 0x00689A70, ::hookkit::Conv::eThiscall, bool, CGxShader*, void*);
    // creates the d3d object from byte_code_ if not attempted yet (IShaderCreate), returns compile_flags_ (creation ok)
    HOOKKIT_HOOK(isValid, 0x00689A50, ::hookkit::Conv::eThiscall, uint32_t, CGxShader*);
};

static_assert(sizeof(CGxShader) == 0x58);
