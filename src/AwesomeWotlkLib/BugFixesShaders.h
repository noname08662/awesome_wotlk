#pragma once

namespace bug_fixes_shaders {
inline constexpr auto kVertexShaderHlsl = R"(
    float4 transform[4] : register(c0);
    float4 viewport : register(c222);    // D3D viewport width, height, 1/width, 1/height; 0 skips the snap

    struct Output {
        float4 position : POSITION;
        float4 color : COLOR;
        float2 texcoord : TEXCOORD;
    };

    float4 project(float4 position) {
        return float4(dot(transform[0], position), dot(transform[1], position), dot(transform[2], position),
            dot(transform[3], position));
    }

    // d3d9 puts pixel centers on integer window coordinates, so pixel edges sit at k - 0.5
    float4 snap(float4 position) {
        [branch] if (viewport.x <= 0.0) { return position; }
        float2 window = float2(1.0 + position.x / position.w, 1.0 - position.y / position.w) * 0.5 * viewport.xy;
        window = floor(window + 1.0) - 0.5;
        float2 ndc = float2(window.x * viewport.z * 2.0 - 1.0, 1.0 - window.y * viewport.w * 2.0);
        return float4(ndc * position.w, position.zw);
    }

    Output mainSnap(float4 position : POSITION, float4 color : COLOR, float4 texcoord : TEXCOORD) {
        Output o;
        o.position = snap(project(position));
        o.color = color;
        o.texcoord = texcoord.xy;
        return o;
    }

    Output mainSnapStereo(float4 position : POSITION, float4 color : COLOR, float4 texcoord : TEXCOORD) {
        float4 r = project(position);
        Output o;
        o.position = snap(float4(r.xy * r.w, r.z, r.w));
        o.color = color;
        o.texcoord = texcoord.xy;
        return o;
    }
)";

inline constexpr auto kPixelShaderHlsl = R"(
    sampler2D uiTexture : register(s0);
    float4 texelSize : register(c222);  // width, height, 1/width, 1/height of the stage-0 texture; 0 if unknown

    static const bool kTent = TENT;               // uiTextureSampling 2, one compile per mode

    static const float kMinTexelsPerPixel = 2.0;  // per axis; a 64px icon at ~35px (1.8:1) keeps the sharper single tap
    static const float kMaxTaps = 16.0;           // loop bound, per texture axis
    static const float kBoxBudget = 8.0;          // taps per axis the box footprint may use
    static const float kTentBudget = 16.0;        // the tent spans twice as far; 12 per axis still drops lines

    float4 sampleFootprint(float2 uv) {
        float2 du = ddx(uv);
        float2 dv = ddy(uv);
        float4 size = texelSize;

        float2 extent = (abs(du) + abs(dv)) * size.xy;  // pixel footprint bounding box, texels
        [branch] if (size.x <= 0.0 || max(extent.x, extent.y) <= kMinTexelsPerPixel) {
            return tex2Dgrad(uiTexture, uv, du, dv);
        }

        float2 half_span = kTent ? extent : extent * 0.5;
        float2 center = uv * size.xy;
        float2 first = ceil(center - half_span - 0.5);
        float2 last = floor(center + half_span - 0.5);
        float2 count = extent > kMinTexelsPerPixel ? max(last - first + 1.0, 0.0) : 0.0;
        float2 stride = max(ceil(count / (kTent ? kTentBudget : kBoxBudget)), 1.0);
        float nx = count.x >= 1.0 ? ceil(count.x / stride.x) : 1.0;
        float ny = count.y >= 1.0 ? ceil(count.y / stride.y) : 1.0;

        float3 weighted = 0.0;
        float3 plain = 0.0;
        float weight_sum = 0.0;
        float alpha_sum = 0.0;
        float alpha_max = 0.0;
        [loop] for (float i = 0.0; i < kMaxTaps; i += 1.0) {
            if (i >= nx) { break; }
            float tx = count.x >= 1.0 ? first.x + i * stride.x + 0.5 : center.x;
            float wx = kTent && count.x >= 1.0 ? saturate(2.0 - 2.0 * abs(tx - center.x) / extent.x) : 1.0;
            [loop] for (float j = 0.0; j < kMaxTaps; j += 1.0) {
                if (j >= ny) { break; }
                float ty = count.y >= 1.0 ? first.y + j * stride.y + 0.5 : center.y;
                float wy = kTent && count.y >= 1.0 ? saturate(2.0 - 2.0 * abs(ty - center.y) / extent.y) : 1.0;
                float w = wx * wy;
                float4 s = tex2Dlod(uiTexture, float4(float2(tx, ty) * size.zw, 0.0, 0.0));
                weighted += s.rgb * s.a * w;
                plain += s.rgb * w;
                weight_sum += w;
                alpha_sum += s.a * w;
                alpha_max = max(alpha_max, s.a * w);
            }
        }
        float3 rgb = alpha_sum > 1e-5 ? weighted / alpha_sum : plain / max(weight_sum, 1e-5);
        return float4(rgb, alpha_max);
    }

    float4 mainNormal(float4 color : COLOR0, float2 uv : TEXCOORD0) : COLOR {
        return sampleFootprint(uv) * color;
    }

    float4 mainDesaturate(float4 color : COLOR0, float2 uv : TEXCOORD0) : COLOR {
        float4 s = sampleFootprint(uv);
        float gray = dot(s.rgb, float3(0.299, 0.587, 0.114));
        return float4(gray, gray, gray, s.a * color.a);
    }
)";
}  // namespace bug_fixes_shaders
