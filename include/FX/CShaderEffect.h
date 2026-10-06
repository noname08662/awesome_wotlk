#pragma once

class CShaderEffect {
public:
    inline static auto& alpha_op_arr = *reinterpret_cast<GxTexOp (*)[8]>(0x00AF59E8);
    inline static auto& color_op_arr = *reinterpret_cast<GxTexOp (*)[8]>(0x00AF5A08);

    TSHashObject<CShaderEffect> hash_obj_;
    uint32_t pass_count_;
    GxTexOp color_op_[2];
    GxTexOp alpha_op_[2];
    CGxShader* vertex_shaders_[90];
    CGxShader* pixel_shaders_[16];
};

static_assert(sizeof(CShaderEffect) == 0x1D4);
