#pragma once
#include <windows.h>
#undef min
#undef max

#include "MSDFTypes.h"

#include <msdfgen.h>
#include <msdfgen-ext.h>

class MSDFCache;
class MSDFFont;

namespace MSDF {
	namespace {
        inline uint32_t g_runtimeVBSize = 0;

		inline IDirect3DPixelShader9* s_cachedPS = nullptr;
        inline IDirect3DVertexShader9* s_cachedVS = nullptr;

		typedef double(__cdecl* GetFontEffectiveWidth_t)(int a1, float a2);
		typedef double(__cdecl* GetFontEffectiveHeight_t)(int a1, float a2);

        typedef CGlyphCacheEntry* (__thiscall* IGxuFontInitGlyphMetrics_t)(CFramePoint* pAtlasPage, CGlyphMetrics* resultBuffer);
		typedef void(__thiscall* IGxuFontRenderBatch_t)(IGxuFontObj* pThis);
		typedef char(__cdecl* IGxuFontGlyphRenderGlyph_t)(FT_Face fontFace, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGlyphMetrics* resultBuffer, uint32_t outline_flag, uint32_t pad);

        typedef void(__thiscall* CGxString__WriteGeometry_t)(CGxString* pThis, int destPtr, int index, int vertIndex, int vertCount);
		typedef CGlyphCacheEntry* (__thiscall* CGxString__GetOrCreateGlyphEntry_t)(CFontObject* fontObj, uint32_t codepoint);
		typedef double(__thiscall* CGxString__GetBearingX_t)(CFontObject* fontObj, CGlyphCacheEntry* entry, float flag, float scale);
		typedef int(__thiscall* CGxString__InitializeTextLine_t)(CGxString* pThis, char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7);
		typedef int*(__thiscall* CGxString__ClearInstanceData_t)(CGxString* pThis);
		typedef bool(__thiscall* CGxString__CheckGeometry_t)(CGxString* pThis);
		typedef uint32_t(__thiscall* CGxString__GetVertCountForPage_t)(CGxString* pThis, int pageIdx);
        
        typedef int(__cdecl* CGxDevice__FlushBuffer_t)(int* bufferHandle, int bufferSize);
        typedef int(__cdecl* CGxDevice__InitFontIndexBuffer_t)();
        typedef void* (__thiscall* CGxDevice__PoolCreate_t)(IDirect3DDevice9* pDevice, int flags, int pad, int size, int usage, const char* name);

		typedef int(__cdecl* FreeType_Init_t)(void* memory, FT_Library* alibrary);
		typedef int(__cdecl* FreeType_NewMemoryFace_t)(FT_Library library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face* aface);
		typedef int(__cdecl* FreeType_Done_Face_t)(FT_Face face);
		typedef int(__cdecl* FreeType_SetPixelSizes_t)(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
		typedef FT_UInt(__cdecl* FreeType_GetCharIndex_t)(FT_Face face, FT_ULong charcode);
		typedef int(__cdecl* FreeType_LoadGlyph_t)(FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags);
		typedef int(__cdecl* FreeType_GetKerning_t)(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning);
		typedef int(__cdecl* FreeType_Done_FreeType_t)(FT_Library library);
		typedef int(__cdecl* FreeType_NewFace_t)(int* library, int face_descriptor);


		inline GetFontEffectiveWidth_t GetFontEffectiveWidth_orig = reinterpret_cast<GetFontEffectiveWidth_t>(0x006C0B60);
		inline GetFontEffectiveHeight_t GetFontEffectiveHeight_orig = reinterpret_cast<GetFontEffectiveHeight_t>(0x006C0B20);

		inline IGxuFontRenderBatch_t IGxuFontRenderBatch_orig = reinterpret_cast<IGxuFontRenderBatch_t>(0x006C53A0);
		inline IGxuFontGlyphRenderGlyph_t IGxuFontGlyphRenderGlyph_orig = reinterpret_cast<IGxuFontGlyphRenderGlyph_t>(0x006C8CC0);

        inline CGxString__WriteGeometry_t CGxString__WriteGeometry_orig = reinterpret_cast<CGxString__WriteGeometry_t>(0x006C5E90);
		inline CGxString__GetOrCreateGlyphEntry_t CGxString__GetOrCreateGlyphEntry_orig = reinterpret_cast<CGxString__GetOrCreateGlyphEntry_t>(0x006C3FC0);
		inline CGxString__GetBearingX_t CGxString__GetBearingX_orig = reinterpret_cast<CGxString__GetBearingX_t>(0x006C24F0);
		inline CGxString__InitializeTextLine_t CGxString__InitializeTextLine_orig = reinterpret_cast<CGxString__InitializeTextLine_t>(0x006C6CD0);
		inline CGxString__ClearInstanceData_t CGxString__ClearInstanceData_orig = reinterpret_cast<CGxString__ClearInstanceData_t>(0x006C6B90);
		inline CGxString__CheckGeometry_t CGxString__CheckGeometry_orig = reinterpret_cast<CGxString__CheckGeometry_t>(0x006C7480);
		inline CGxString__GetVertCountForPage_t CGxString__GetVertCountForPage_orig = reinterpret_cast<CGxString__GetVertCountForPage_t>(0x006C63E0);


        inline CGxDevice__FlushBuffer_t CGxDevice__FlushBuffer_orig = reinterpret_cast<CGxDevice__FlushBuffer_t>(0x006C48D0);
        inline CGxDevice__InitFontIndexBuffer_t CGxDevice__InitFontIndexBuffer_orig = reinterpret_cast<CGxDevice__InitFontIndexBuffer_t>(0x006C47B0);
        inline CGxDevice__PoolCreate_t CGxDevice__PoolCreate_orig = reinterpret_cast<CGxDevice__PoolCreate_t>(0x006876D0);


		inline void(*CGxString__CheckGeometry_call)() = reinterpret_cast<decltype(CGxString__CheckGeometry_call)>(0x006C4B09);
		inline constexpr DWORD_PTR CGxString__CheckGeometry_call_jmpback = 0x006C4B10;

		inline void(*CGxString__CheckGeometry_site)() = reinterpret_cast<decltype(CGxString__CheckGeometry_site)>(0x006C4AF3);
		inline constexpr DWORD_PTR CGxString__CheckGeometry_site_loopstart = 0x006C4B00;


		inline void(*CGxString__GetGlyphYMetrics_site)() = reinterpret_cast<decltype(CGxString__GetGlyphYMetrics_site)>(0x006C8C71);
		inline constexpr DWORD_PTR CGxString__GetGlyphYMetrics_site_jmpback = 0x006C8C77;

        inline void(*CGxDevice__AllocateFontIndexBuffer_site)() = reinterpret_cast<decltype(CGxDevice__AllocateFontIndexBuffer_site)>(0x006C480C);
		inline constexpr DWORD_PTR CGxDevice__AllocateFontIndexBuffer_site_jmpback = 0x006C4811;

        inline void(*CGxDevice__InitFontIndexBuffer_site)() = reinterpret_cast<decltype(CGxDevice__InitFontIndexBuffer_site)>(0x006C47BD);
		inline constexpr DWORD_PTR CGxDevice__InitFontIndexBuffer_site_jmpback = 0x006C47D8;

        inline void(*IGxuFontProcessBatch_site)() = reinterpret_cast<decltype(IGxuFontProcessBatch_site)>(0x006C4CC4);
		inline constexpr DWORD_PTR IGxuFontProcessBatch_site_jmpback = 0x006C4CC9;

        inline void(*CGxDevice__BufStream_site)() = reinterpret_cast<decltype(CGxDevice__BufStream_site)>(0x006C4B40);
		inline constexpr DWORD_PTR CGxDevice__BufStream_site_jmpback = 0x006C4B45;

        inline void(*bufalloc_1_site)() = reinterpret_cast<decltype(bufalloc_1_site)>(0x006C4B64);
		inline constexpr DWORD_PTR bufalloc_1_site_jmpback = 0x006C4B70;

        inline void(*bufalloc_2_site)() = reinterpret_cast<decltype(bufalloc_2_site)>(0x006C4C67);
		inline constexpr DWORD_PTR bufalloc_2_site_jmpback = 0x006C4C8A;

        inline void(*bufalloc_3_site)() = reinterpret_cast<decltype(bufalloc_3_site)>(0x006C4C36);
		inline constexpr DWORD_PTR bufalloc_3_site_jmpback = 0x006C4C4B;


        inline FreeType_Init_t FreeType_Init_orig = reinterpret_cast<FreeType_Init_t>(0x00991320);
        inline FreeType_NewMemoryFace_t FreeType_NewMemoryFace_orig = reinterpret_cast<FreeType_NewMemoryFace_t>(0x00993370);
        inline FreeType_Done_Face_t FreeType_Done_Face_orig = reinterpret_cast<FreeType_Done_Face_t>(0x00992610);
        inline FreeType_SetPixelSizes_t FreeType_SetPixelSizes_orig = reinterpret_cast<FreeType_SetPixelSizes_t>(0x00992780);
        inline FreeType_GetCharIndex_t FreeType_GetCharIndex_orig = reinterpret_cast<FreeType_GetCharIndex_t>(0x009911A0);
        inline FreeType_LoadGlyph_t FreeType_LoadGlyph_orig = reinterpret_cast<FreeType_LoadGlyph_t>(0x00992DA0);
        inline FreeType_GetKerning_t FreeType_GetKerning_orig = reinterpret_cast<FreeType_GetKerning_t>(0x00991050);
        inline FreeType_Done_FreeType_t FreeType_Done_FreeType_orig = reinterpret_cast<FreeType_Done_FreeType_t>(0x00992CB0);
        inline FreeType_NewFace_t FreeType_NewFace_orig = reinterpret_cast<FreeType_NewFace_t>(0x009931A0);
	}

    inline int __cdecl FreeType_NewMemoryFace_hk(FT_Library library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face* aface);
    inline int __cdecl FreeType_NewFace_hk(int* library, int face_descriptor);
    inline int __cdecl FreeType_Done_Face_hk(FT_Face face);
    inline int __cdecl FreeType_SetPixelSizes_hk(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
    inline FT_UInt __cdecl FreeType_GetCharIndex_hk(FT_Face face, FT_ULong charcode);
    inline int __cdecl FreeType_LoadGlyph_hk(FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags);
    inline int __cdecl FreeType_GetKerning_hk(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning);
    inline int __cdecl FreeType_Done_FreeType_hk(FT_Library library);
    inline int __cdecl FreeType_Init_hk(void* memory, FT_Library* alibrary);

    inline void __cdecl PrefetchCodepoints(CGxString* pThis);
    inline void __fastcall ProcessGeometry(CGxString* pThis);

    inline void __fastcall IGxuFontRenderBatch_hk(IGxuFontObj* pThis);
    inline void __fastcall IGxuFontRenderBatch_hk_pregen(IGxuFontObj* pThis);
    inline char __cdecl IGxuFontGlyphRenderGlyph_hk(FT_Face fontFace, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGlyphMetrics* resultBuffer, uint32_t outline_flag, uint32_t pad);
    inline bool __fastcall CGxString__CheckGeometry_hk(CGxString* pThis);
    inline void __fastcall CGxString__WriteGeometry_hk(CGxString* pThis, void* edx, int destPtr, int index, int vertIndex, int vertCount);
    inline CGlyphCacheEntry* __fastcall CGxString__GetOrCreateGlyphEntry_hk(CFontObject* fontObj, void* edx, uint32_t codepoint);
    inline int __fastcall CGxString__InitializeTextLine_hk(CGxString* pThis, void* edx, char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7);

    inline void CGxDevice__AllocateFontIndexBuffer_site_hk();
    inline void CGxDevice__InitFontIndexBuffer_site_hk();
    inline void IGxuFontProcessBatch_site_hk();
    inline void CGxDevice__BufStream_site_hk();
    inline void bufalloc_1_site_hk();
    inline void bufalloc_2_site_hk();
    inline void bufalloc_3_site_hk();

    inline void CGxString_GetGlyphYMetrics_site_hk();

	inline void CGxString__CheckGeometry_site_hk();
    inline void CGxString__CheckGeometry_call_hk();

	// ----  if you want overkill quality, try raising these
	inline uint32_t ATLAS_SIZE = 1024; // 1024-2048
	inline constexpr uint32_t PREGEN_START_KEY = VK_F11;
	inline constexpr uint32_t SDF_SAMPLER_SLOT = 23;
	inline constexpr uint32_t ATLAS_SIZE_CJK = 2048; // 2048-4096
	inline constexpr uint32_t ATLAS_GUTTER = 12; // usually spread + 2-4
	inline constexpr uint32_t SDF_RENDER_SIZE = 64; // 48-128
	inline constexpr uint32_t SDF_SPREAD = 8; // 6-12
	inline constexpr D3DFORMAT D3DFMT = D3DFMT_A8R8G8B8; // D3DFMT_A8R8G8B8-D3DFMT_A16B16G16R16
	// ----

	inline D3D::ShaderData*& g_FontPixelShader = *reinterpret_cast<D3D::ShaderData**>(0x00C7D2CC);
	inline D3D::ShaderData*& g_FontVertexShader = *reinterpret_cast<D3D::ShaderData**>(0x00C7D2D0);

	inline FT_Library g_realFtLibrary = nullptr;
	inline msdfgen::FreetypeHandle* g_msdfFreetype = nullptr;

    inline constexpr uint32_t MAX_ATLAS_PAGES = 4;
    inline constexpr size_t CJK_CACHE_THRESHOLD = 16661;

	inline int MODE = 0;
	inline bool IS_CJK = false;
	inline bool INITIALIZED = false;
	inline bool ALLOW_UNSAFE_FONTS = false; // due to how distance fields are calculated, some fonts with self-intersecting contours (e.g. diediedie) will break
    inline LPWSTR IS_PREGEN = wcsstr(GetCommandLineW(), L"-msdf-pregen");

	inline const bool IS_WIN10 = []() {
		HMODULE hKernel = GetModuleHandleW(L"kernelbase.dll");
		if (!hKernel) return false;

		return (GetProcAddress(hKernel, "VirtualAlloc2") != nullptr &&
			GetProcAddress(hKernel, "MapViewOfFile3") != nullptr &&
			GetProcAddress(hKernel, "UnmapViewOfFile2") != nullptr);
		}();

	inline Console::CVar* s_cvar_MSDFMode;
	inline int CVarHandler_MSDFMode(Console::CVar* cvar, const char*, const char* value, void*) {
		const int val = std::atoi(value);
		return (val >= 0 && val <= 2) ? 1 : 0;
	}

	inline std::string GetGameLocale() {
		Console::CVar* locale = Console::GetCVar("locale");
		return (locale && locale->vStr) ? locale->vStr : std::string{};
	}

	inline static std::vector<uint8_t> s_prefetchPayload;

    void initialize();
};