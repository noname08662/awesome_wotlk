#pragma once
#include <windows.h>
#undef min
#undef max

#include "GameClient.h"
#include "D3D.h"

#include <msdfgen.h>
#include <msdfgen-ext.h>

struct GlyphMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
	FT_Int bitmapTop = 0;
	FT_Int bitmapLeft = 0;
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    uint16_t atlasPageIndex = 0;
    const uint8_t* pixelData = nullptr;
};

struct GlyphMetricsToStore {
    uint32_t codepoint = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    FT_Int bitmapTop = 0;
	FT_Int bitmapLeft = 0;
    std::vector<uint8_t> ownedPixelData;
    uint32_t dataSize = 0;
};

class MSDFCache;
class MSDFFont;

namespace MSDF {
	// ----  if you want overkill quality, try raising these
	inline constexpr uint32_t ATLAS_SIZE = 1024; // 1024-2048
	inline constexpr uint32_t PREGEN_START_KEY = VK_F11;
	inline constexpr uint32_t SDF_SAMPLER_SLOT = 23;
	inline constexpr uint32_t ATLAS_GUTTER = 12; // usually spread + 2-4
	inline constexpr uint32_t SDF_RENDER_SIZE = 64; // 48-128
	inline constexpr uint32_t SDF_SPREAD = 8; // 6-12
	inline constexpr D3DFORMAT D3DFMT = D3DFMT_A8R8G8B8; // D3DFMT_A8R8G8B8-D3DFMT_A16B16G16R16
	// ----

	inline CGxDevice::ShaderData*& g_FontPixelShader = *reinterpret_cast<CGxDevice::ShaderData**>(0x00C7D2CC);
	inline CGxDevice::ShaderData*& g_FontVertexShader = *reinterpret_cast<CGxDevice::ShaderData**>(0x00C7D2D0);

	inline FT_Library g_realFtLibrary = nullptr;
	inline msdfgen::FreetypeHandle* g_msdfFreetype = nullptr;

    inline constexpr uint32_t MAX_ATLAS_PAGES = 4;

	inline bool IS_CJK = false;
	inline bool INITIALIZED = false;
	inline bool ALLOW_UNSAFE_FONTS = false; // due to how distance fields are calculated, some fonts with self-intersecting contours (e.g. diediedie) will break

	inline const bool IS_WIN10 = []() {
		HMODULE hKernel = GetModuleHandleW(L"kernelbase.dll");
		if (!hKernel) return false;

		return (GetProcAddress(hKernel, "VirtualAlloc2") != nullptr &&
			GetProcAddress(hKernel, "MapViewOfFile3") != nullptr &&
			GetProcAddress(hKernel, "UnmapViewOfFile2") != nullptr);
		}();

	inline std::string GetGameLocale() {
		CVar* locale = CVar::Get("locale");
		return (locale && locale->m_str) ? locale->m_str : std::string{};
	}

    void initialize();
};