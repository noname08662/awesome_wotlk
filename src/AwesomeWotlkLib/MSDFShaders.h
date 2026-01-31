#pragma once

const char* vertexShaderHLSL = R"(
	uniform float4x4 WorldViewProj;
	float4 control : register(c23); // font size, outline mode, spread, atlas size

	struct VS_IN {
		float4 pos  : POSITION0;
		float4 col  : COLOR0;
		float2 uv0  : TEXCOORD0;
	};

	struct VS_OUT {
		float4 hpos : POSITION;
		float4 col  : COLOR0;
		float2 uv0  : TEXCOORD0;
		float4 pageIdx : TEXCOORD1; // target page index
	};

	VS_OUT main(VS_IN IN) {
		VS_OUT OUT;
		OUT.hpos = mul(IN.pos, WorldViewProj);
		OUT.col  = IN.col;

		if (control.x < 1.0f) {
			OUT.uv0  = IN.uv0;
			OUT.pageIdx = float4(0, 0, 0, 1.0f);
		} else {
			// encoded sign bits
			float bit0 = (IN.uv0.x < 0.0f) ? 1.0f : 0.0f;
			float bit1 = (IN.uv0.y < 0.0f) ? 2.0f : 0.0f;
			OUT.pageIdx = float4(bit0 + bit1, 0, 0, 0);
			OUT.uv0 = abs(IN.uv0);
		}
		return OUT;
	}
)";

const char* pixelShaderHLSL = R"(
	sampler2D gameTexture : register(s0);

	sampler2D sdfAtlas0   : register(s12);
	sampler2D sdfAtlas1   : register(s13);
	sampler2D sdfAtlas2   : register(s14);
	sampler2D sdfAtlas3   : register(s15);

	float4 control : register(c23); // font size, outline mode, spread, atlas size

	struct PS_IN {
		float4 col : COLOR0;
		float2 uv0 : TEXCOORD0;
		float4 pageIdx : TEXCOORD1; // target page index
	};

	float median(float r, float g, float b) {
		return max(min(r, g), min(max(r, g), b));
	}

	float4 main(PS_IN IN) : COLOR {
		if (control.x < 1.0f) return tex2D(gameTexture, IN.uv0) * IN.col;

		float2 uv = IN.uv0;

		float outlineHint = control.y;
		float fontSize = control.x;
		float outlinePx = 0.0f;

		if (outlineHint >= 1.5f) {
			outlinePx = max(2.75f, pow(fontSize, 0.150f) * 1.6f);
		} else if (outlineHint >= 0.5f) {
			outlinePx = max(1.50f, pow(fontSize, 0.075f) * 1.5f);
		}

		int atlasPage = int(IN.pageIdx.x + 0.5f);

		float4 sample;
		if (atlasPage == 0) sample = tex2D(sdfAtlas0, uv);
		else if (atlasPage == 1) sample = tex2D(sdfAtlas1, uv);
		else if (atlasPage == 2) sample = tex2D(sdfAtlas2, uv);
		else sample = tex2D(sdfAtlas3, uv);

		float sd = median(sample.r, sample.g, sample.b);
		float screenPxRange = (control.z / max(max(fwidth(uv.x), fwidth(uv.y)) * control.a, 1e-6)) * (1.0f - min(0.3f, fontSize * 0.0035f)); // smoother edges for larger text
		float opacity = saturate((sd - 0.5f) * screenPxRange + 0.5f);

		if (outlinePx > 0.0f) {
			// *5 of the outline msdf, mult has to match alpha-channel range
			return float4(
				lerp(float3(0.0f, 0.0f, 0.0f), IN.col.rgb, opacity),
				max(opacity, saturate((sample.a - 0.5f) * screenPxRange * 5.0f + outlinePx)) * IN.col.a
			);
		}
		return float4(IN.col.rgb, opacity * IN.col.a);
	}
)";