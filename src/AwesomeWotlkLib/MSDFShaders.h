#pragma once

namespace msdf_shaders {
inline constexpr auto kVertexShaderHlsl = R"(
    uniform float4x4 WorldViewProj;
    float4 control : register(c220); // is3d, outline mode, spread, atlas size

    struct VS_IN {
        float4 pos  : POSITION0;
        float4 col  : COLOR0;
        float2 uv0  : TEXCOORD0;
    };

    struct VS_OUT {
        float4 hpos : POSITION;
        float4 col  : COLOR0;
        float2 uv0  : TEXCOORD0;
        float4 info : TEXCOORD1; // target page index, outline vert
    };

    VS_OUT main(VS_IN IN) {
        VS_OUT OUT;
        OUT.col = IN.col;

        if (control.z < 1.0f) {
            OUT.hpos = mul(IN.pos, WorldViewProj);
            OUT.uv0  = IN.uv0;
            OUT.info = float4(0, 1.0f, 0, 1.0f);
        } else {
            float isCore = 1.0f;
            float z = IN.pos.z;

            if (control.x < 0.5f) { // !is3d
                isCore = (z < 0.0f) ? 0.0f : 1.0f;
                z = abs(IN.pos.z);
            }

            float4 pos = IN.pos;
            pos.z = z;
            OUT.hpos = mul(pos, WorldViewProj);

            // encoded sign bits safely before decoding
            float bit0 = (IN.uv0.x < 0.0f) ? 1.0f : 0.0f;
            float bit1 = (IN.uv0.y < 0.0f) ? 2.0f : 0.0f;

            OUT.info = float4(bit0 + bit1, isCore, 0, 0);
            OUT.uv0 = float2(abs(IN.uv0.x), abs(IN.uv0.y));
        }
        return OUT;
    }
)";

inline constexpr auto kPixelShaderHlsl = R"(
    sampler2D gameTexture : register(s0);

    sampler2D sdfAtlas0   : register(s12);
    sampler2D sdfAtlas1   : register(s13);
    sampler2D sdfAtlas2   : register(s14);
    sampler2D sdfAtlas3   : register(s15);

    float4 control     : register(c220); // is3d, outline mode, spread, atlas size
    float4 faceMetrics : register(c221); // stem, hairline, smallest counter (atlas texels), render size (px per em)

    static const bool kOutlinePass = OUTLINE_PASS; // MSDFOutlinePass, one compile per mode

    struct PS_IN {
        float4 col : COLOR0;
        float2 uv0 : TEXCOORD0;
        float4 info : TEXCOORD1; // target page index, outline vert
    };

    float median(float r, float g, float b) {
        return max(min(r, g), min(max(r, g), b));
    }

    float4 main(PS_IN IN) : COLOR {
        if (control.z < 1.0f) return tex2D(gameTexture, IN.uv0) * IN.col;

        float2 uv = IN.uv0;

        float2 dx = ddx(uv);
        float2 dy = ddy(uv);
        float fontSize = (1.0f / control.w) * rsqrt(max(max(dot(dx, dx), dot(dy, dy)), 1e-8f));

        float outlineHint = control.y;
        float outlinePx = 0.0f;

        if (outlineHint >= 1.5f) {
            outlinePx = max(2.75f, pow(fontSize, 1.25f) * 2.5f);
        } else if (outlineHint >= 0.5f) {
            outlinePx = max(1.75f, pow(fontSize, 0.75f) * 2.0f);
        }

        int atlasPage = int(IN.info.x + 0.5f);

        float4 sample;
        if (atlasPage == 0) sample = tex2D(sdfAtlas0, uv);
        else if (atlasPage == 1) sample = tex2D(sdfAtlas1, uv);
        else if (atlasPage == 2) sample = tex2D(sdfAtlas2, uv);
        else sample = tex2D(sdfAtlas3, uv);

        float sd = median(sample.r, sample.g, sample.b);
        float screenPxRange = control.z / max(max(fwidth(uv.x), fwidth(uv.y)) * control.w, 1e-6);
        // smoother edges for larger text only: fontSize is screen px per atlas texel, fontSize * render size the em in
        // screen px - no softening up to a 16px em, the full 20% from 26px
        screenPxRange *= 1.0f - 0.2f * saturate((fontSize * faceMetrics.w - 16.0f) / 10.0f);

        // signed distances in screen px: the msdf reaches +-spread/2 texels, the alpha sdf five times that
        float distMsdf = (sd - 0.5f) * screenPxRange;
        float distSdf = (sample.a - 0.5f) * screenPxRange * 5.0f;
        // past ~90% of the msdf reach the true sdf takes over, so a grown edge never reads a clamped msdf
        float dist = abs(distSdf) < 0.45f * screenPxRange ? distMsdf : distSdf;

        // grow sub-pixel stems towards 0.85px and hairlines towards half that, at most 0.35px a side and never more
        // than a quarter of the smallest counter so counters can't close; faces without metrics don't grow
        float grow = 0.0f;
        if (faceMetrics.x > 0.0f) {
            float stemPx = faceMetrics.x * fontSize;
            float hairPx = faceMetrics.y * fontSize;
            float counterPx = faceMetrics.z * fontSize;
            grow = clamp(max(0.85f - stemPx, 0.425f - hairPx) * 0.5f, 0.0f, min(0.35f, 0.25f * counterPx));
        }

        float bias = min(0.025f, outlinePx * 0.01f) * screenPxRange; // scale down the larger glyphs to compensate for the outlines
        float opacity = saturate(dist + grow - bias + 0.5f);

        if (outlinePx > 0.0f) {
            if (!kOutlinePass) {
                // core + outline pass in single step
                float outlineAlpha = saturate(distSdf + grow + outlinePx);
                float4 outlineCol = float4(0.0f, 0.0f, 0.0f, outlineAlpha * IN.col.a);
                float4 coreCol = float4(IN.col.rgb, opacity * IN.col.a);

                float outAlpha = coreCol.a + outlineCol.a * (1.0f - coreCol.a);
                if (outAlpha <= 0.0f) discard;
                float3 outRGB = coreCol.rgb * (coreCol.a / outAlpha);
                return float4(outRGB, outAlpha);
            } else {
                if (IN.info.y > 0.5f) {
                    // core pass
                    return float4(IN.col.rgb, opacity * IN.col.a);
                } else {
                    // outline pass
                    float outlineAlpha = saturate(distSdf + grow + outlinePx);
                    if (outlineAlpha <= 0.0f) discard;
                    return float4(0.0f, 0.0f, 0.0f, outlineAlpha * IN.col.a);
                }
            }
        }

        if (IN.info.y < 0.5f) discard; // no outline, skip outline pass
        if (opacity <= 0.0f) discard;
        return float4(IN.col.rgb, opacity * IN.col.a);
    }
)";
}  // namespace msdf_shaders
