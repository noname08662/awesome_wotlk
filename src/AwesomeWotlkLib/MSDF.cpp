#include "MSDF.h"
#include "MSDFFont.h"
#include "MSDFCache.h"
#include "MSDFManager.h"
#include "MSDFShaders.h"
#include "Utils.h"
#include "Hooks.h"
#include <ranges>

namespace {
    uint32_t g_runtimeVBSize = 0;

    IDirect3DPixelShader9* s_cachedPS = nullptr;
    IDirect3DVertexShader9* s_cachedVS = nullptr;

    auto(*CGxString__CheckGeometry_call)() = reinterpret_cast<void(*)()>(0x006C4B09);
    constexpr uintptr_t CGxString__CheckGeometry_call_jmpback = 0x006C4B10;

    auto(*CGxString__CheckGeometry_site)() = reinterpret_cast<void(*)()>(0x006C4AF3);
    constexpr uintptr_t CGxString__CheckGeometry_site_loopstart = 0x006C4B00;

    auto(*CGxString__GetGlyphYMetrics_site)() = reinterpret_cast<void(*)()>(0x006C8C71);
    constexpr uintptr_t CGxString__GetGlyphYMetrics_site_jmpback = 0x006C8C77;

    auto(*CGxDevice__AllocateFontIndexBuffer_site)() = reinterpret_cast<void(*)()>(0x006C480C);
    constexpr uintptr_t CGxDevice__AllocateFontIndexBuffer_site_jmpback = 0x006C4811;

    auto(*CGxDevice__InitFontIndexBuffer_site)() = reinterpret_cast<void(*)()>(0x006C47BD);
    constexpr uintptr_t CGxDevice__InitFontIndexBuffer_site_jmpback = 0x006C47D8;

    auto(*IGxuFontProcessBatch_site)() = reinterpret_cast<void(*)()>(0x006C4CC4);
    constexpr uintptr_t IGxuFontProcessBatch_site_jmpback = 0x006C4CC9;

    auto(*CGxDevice__BufStream_site)() = reinterpret_cast<void(*)()>(0x006C4B40);
    constexpr uintptr_t CGxDevice__BufStream_site_jmpback = 0x006C4B45;

    auto(*bufalloc_1_site)() = reinterpret_cast<void(*)()>(0x006C4B64);
    constexpr uintptr_t bufalloc_1_site_jmpback = 0x006C4B70;

    auto(*bufalloc_2_site)() = reinterpret_cast<void(*)()>(0x006C4C67);
    constexpr uintptr_t bufalloc_2_site_jmpback = 0x006C4C8A;

    auto(*bufalloc_3_site)() = reinterpret_cast<void(*)()>(0x006C4C36);
    constexpr uintptr_t bufalloc_3_site_jmpback = 0x006C4C4B;

	CVar* s_cvar_MSDFMode;
	std::vector<uint8_t> s_prefetchPayload;

    void __cdecl PrefetchCodepoints(CGxString* pThis) {
        if (s_prefetchPayload.empty()) return;
        if (!pThis || reinterpret_cast<uintptr_t>(pThis) & 1) return;

        // sort everything in a batch (no idea if 1 batch == 1 face, though)
        if (MSDFFont* fontHandle = MSDFFont::Get(pThis->GetFontFace())) {
            std::ranges::sort(s_prefetchPayload);
            s_prefetchPayload.erase(std::ranges::unique(s_prefetchPayload).begin(), s_prefetchPayload.end());
            for (uint32_t codepoint : s_prefetchPayload) {
                fontHandle->GetGlyph(codepoint);
            }
        }
        s_prefetchPayload.clear();
    }

    void __fastcall ProcessGeometry(CGxString* pThis) {
        if (!(pThis->m_flags & 0x40000000)) return;

        CGxFont* fontObj = pThis->m_fontObj;
        FT_Face fontFace = pThis->GetFontFace();
        MSDFFont* fontHandle = MSDFFont::Get(fontFace);
        if (!fontHandle) return;

        CGxFontGeomBatch* batch = pThis->m_geomBuffers[0];
        if (!batch || batch->m_verts.m_count < 4) return;

        TSGrowableArray<CGxFontVertex>& verts = batch->m_verts;
        if (verts.m_count < 4) return;

        const uint32_t flags = fontObj->m_atlasPages[0].m_flags;
        const bool is3d = pThis->m_flags & 0x80; // native 3d obj - nameplate text, etc.
        const double fontSizeMult = pThis->m_fontSizeMult;
        const double fontOffs = !is3d ? ((flags & 8) ? 4.5 : ((flags & 1) ? 2.5 : 0.0)) : 0.0;
        const double baselineOffs = (fontOffs > 0.0) ? 1.0 : 0.0;
        const double scale = (is3d ? fontSizeMult : CGxuFont::GetFontEffectiveHeight(is3d, fontSizeMult) * 0.98) / MSDF::SDF_RENDER_SIZE; // 0.98 compensation
        const double pad = MSDF::SDF_SPREAD * scale;

        for (uint32_t q = 0; q < verts.m_count; q += 4) {
            CGxFontVertex* vBase = &verts.m_data[q];
            if (vBase[0].u > 1.0f) {
                const uint32_t codepoint = vBase[0].u - 1.0f;

                const GlyphMetrics* gm = fontHandle->GetGlyph(codepoint);
                if (!gm) continue;

                CGxGlyphCacheEntry* entry = fontObj->GetOrCreateGlyphEntry(codepoint);
                if (!entry) continue;

                CGxFontVertex* vert0 = &vBase[0];
                CGxFontVertex* vert1 = &vBase[1];
                CGxFontVertex* vert2 = &vBase[2];
                CGxFontVertex* vert3 = &vBase[3];

                const double leftOffs = fontObj->GetBearingX(entry, is3d, fontSizeMult);
                const double bitmapLeft = is3d ? leftOffs : gm->bitmapLeft * scale - leftOffs;

                // no clue where this  + 1.0  comes from, but it works, I guess?..
                const double newLeft = static_cast<double>(vert0->pos.X) + (bitmapLeft != leftOffs ? bitmapLeft + 1.0 : 0.0) - pad + fontOffs * 0.5;
                const double newRight = newLeft + (gm->width * scale);

                const double newTop = static_cast<double>(vert1->pos.Y) + (gm->bitmapTop * scale) + pad - baselineOffs;
                const double newBottom = newTop - (gm->height * scale);

                vert0->pos.X = static_cast<float>(newLeft);  vert0->pos.Y = static_cast<float>(newBottom);
                vert1->pos.X = static_cast<float>(newLeft);  vert1->pos.Y = static_cast<float>(newTop);
                vert2->pos.X = static_cast<float>(newRight); vert2->pos.Y = static_cast<float>(newBottom);
                vert3->pos.X = static_cast<float>(newRight); vert3->pos.Y = static_cast<float>(newTop);

                const float u0 = gm->u0;
                const float u1 = gm->u1;
                const float v0 = gm->v0;
                const float v1 = gm->v1;

                // encode target msdf atlas page ifx into the sign bits preserving the mantissa part bit-perfect
                const float uSign = (gm->atlasPageIndex & 1) ? -1.0f : 1.0f;
                const float vSign = (gm->atlasPageIndex & 2) ? -1.0f : 1.0f;

                vert0->u = u0 * uSign; vert0->v = v0 * vSign;
                vert1->u = u0 * uSign; vert1->v = v1 * vSign;
                vert2->u = u1 * uSign; vert2->v = v0 * vSign;
                vert3->u = u1 * uSign; vert3->v = v1 * vSign;
            }
        }
        pThis->m_flags &= ~0x40000000;

        // store eviction count to later force engine to re-calc geometry when msdf page gets evicted
        uint32_t versionToken = (fontHandle->GetAtlasEvictionCount() & 0x7F) | 0x80;
        pThis->m_flags = (pThis->m_flags & 0x00FFFFFF) | (versionToken << 24);
    }


    bool __fastcall CGxString__CheckGeometryHk(CGxString* pThis) {
        if (MSDFFont* fontHandle = MSDFFont::Get(pThis->GetFontFace())) {
            // force re-calc geometry if any msdf page was evicted
            uint32_t highByte = (pThis->m_flags >> 24) & 0xFF;
            if ((highByte & 0x80) != 0) {
                uint8_t storedVersion = highByte & 0x7F;
                uint8_t currentVersion = static_cast<uint8_t>(fontHandle->GetAtlasEvictionCount() & 0x7F);
                if (storedVersion != currentVersion) {
                    pThis->ClearInstanceData();
                    pThis->m_flags &= 0x00FFFFFF;
                }
            }
        }
        CGxFontGeomBatch* batch = pThis->m_geomBuffers[0];
        if (!batch || !batch->m_verts.m_data) return pThis->CheckGeometry();

        g_runtimeVBSize += pThis->GetVertCountForPage(0);
        return pThis->CheckGeometry();
    }

    void __fastcall CGxString__WriteGeometryHk(CGxString* pThis, void* edx, int destPtr, int index, int vertIndex, int vertCount) {
        pThis->WriteGeometry(destPtr, index, vertIndex, vertCount);

        MSDFFont* fontHandle = MSDFFont::Get(pThis->GetFontFace());
        if (!fontHandle) {
            if (IDirect3DDevice9* device = D3D::GetDevice()) {
                constexpr float resetControl[4] = { 0, 0, 0, 0 };
                device->SetPixelShaderConstantF(MSDF::SDF_SAMPLER_SLOT, resetControl, 1);
                device->SetVertexShaderConstantF(MSDF::SDF_SAMPLER_SLOT, resetControl, 1);
            }
            return;
        }

        IDirect3DDevice9* device = D3D::GetDevice();
        if (!device) return;

        for (uint32_t pageIdx = 0; pageIdx < fontHandle->GetAtlasPageCount(); ++pageIdx) {
            auto* atlasTexture = fontHandle->GetAtlasPage(pageIdx);
            if (atlasTexture && atlasTexture->texture) {
                uint32_t slot = (/* max d3d9 tex slots */ 15 - MSDF::MAX_ATLAS_PAGES + 1) + pageIdx;
                device->SetTexture(slot, atlasTexture->texture);
                device->SetSamplerState(slot, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
                device->SetSamplerState(slot, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
                device->SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                device->SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                device->SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            }
        }

        const uint32_t flags = pThis->m_fontObj->m_atlasPages[0].m_flags;
        const bool is3d = pThis->m_flags & 0x80;
        const float controlFlag[4] = {
            is3d ? pThis->m_fontObj->m_rasterTargetSize : static_cast<float>(CGxuFont::GetFontEffectiveHeight(is3d, pThis->m_fontSizeMult)),
            is3d ? 0.0f : ((flags & 8) ? 2.0f : ((flags & 1) ? 1.0f : 0.0f)),
            MSDF::SDF_SPREAD, MSDF::ATLAS_SIZE
        };
        device->SetPixelShaderConstantF(MSDF::SDF_SAMPLER_SLOT, controlFlag, 1);
        device->SetVertexShaderConstantF(MSDF::SDF_SAMPLER_SLOT, controlFlag, 1);
    }

    void __fastcall CGxuFontRenderBatchHk(CGxuFont* pThis) {
        pThis->RenderBatch();
        if (IDirect3DDevice9* device = D3D::GetDevice()) {
            // reset since font PS also handles UI elements
            constexpr float resetControl[4] = { 0, 0, 0, 0 };
            device->SetPixelShaderConstantF(MSDF::SDF_SAMPLER_SLOT, resetControl, 1);
            device->SetVertexShaderConstantF(MSDF::SDF_SAMPLER_SLOT, resetControl, 1);
        }
        return;
    }

    char __cdecl GxuFontGlyphRenderGlyphHk(FT_Face fontFace, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGxGlyphMetrics* resultBuffer, uint32_t outline_flag, uint32_t pad) {
        const char result = CGxuFont::RenderGlyph(fontFace, fontSize, codepoint, pageInfo, resultBuffer, outline_flag, pad);
        if (MSDFFont::Get(fontFace)) {
            // verAdv is distance from the top of the quad to the top of the glyph, bearingY is the quad's vert correction (descenders)
            resultBuffer->m_bearingY -= resultBuffer->m_verAdv;
        }
        return result;
    }

    CGxGlyphCacheEntry* __fastcall CGxString__GetOrCreateGlyphEntryHk(CGxFont* fontObj, void* edx, uint32_t codepoint) {
        CGxGlyphCacheEntry* result = fontObj->GetOrCreateGlyphEntry(codepoint);
        if (result && MSDFFont::Get(CGxString::GetFontFace(fontObj->m_ftWrapper))) {
            result->m_metrics.u0 = 1.0f + codepoint; // store codepoint
            // keep everything at page0 - this is required due to how the engine handles strings with text spanning across multiple atlas pages
            // in short, it renders page0 for all CGxString instances, then page1, and so on
            // this makes shadows on adjacent glyphs overlap due to the lack of depth info
            // e.g. engine renders page0 ['some hing'] -> renders page1 ['t'] -> the shadows of ['t'] now overlaps the countour of ['h']
            result->m_cellIndexMin = 0;
            result->m_cellIndexMax = 0;
            result->m_texturePageIndex = 0;
        }
        return result;
    }

    int __fastcall CGxString__InitializeTextLineHk(CGxString* pThis, void* edx, char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7) {
        const int result = pThis->InitializeTextLine(text, textLength, a4, startPos, a6, a7);

        // 1-st pass - only collect codepoints
        // all of thes will then get sorted to ensure sequentiality
        // == minimal syscall churn @ GetGlyph -> cold cache path == fewer stutters
        if (pThis->m_flags & 0x40000000) return result;
        for (char* p = pThis->m_text; *p; ++p) {
            s_prefetchPayload.push_back(static_cast<uint8_t>(*p));
        }
        pThis->m_flags |= 0x40000000;
        return result;
    }

    // run the original loop, then prefetch once per frame
    // drop out back to loopstart and loop again (ebx preserved)
    // this time, call processGeom to resolve, now with a warmed up cache
    __declspec(naked) void CGxString__CheckGeometry_siteHk() {
        __asm {
            pushad;
            mov edi, ebx;

            test ebx, ebx;
            jnz pre_pass_loop;

            xor ebx, ebx;
            lea esp, [esp + 0]; // ??? (0x006C4B1C)

        pre_pass_loop:
            test edi, edi;
            jz pre_pass_done;
            test di, 1;
            jnz pre_pass_done;

            mov ecx, edi;
            call CGxString__CheckGeometryHk;

            mov eax, [esi + 1Ch];
            add eax, edi;
            mov edi, [eax + 4];
            jmp pre_pass_loop;

        pre_pass_done:
            popad;
            push ebx;
            call PrefetchCodepoints;
            add esp, 4;
            jmp CGxString__CheckGeometry_site_loopstart;
        }
    }

    __declspec(naked) void CGxString__CheckGeometry_callHk() {
        __asm {
            mov ecx, ebx;
            call ProcessGeometry;
            jmp CGxString__CheckGeometry_call_jmpback;
        }
    }

     bool __cdecl MSDFFont_Get(FT_Face face) { return MSDFFont::Get(face); }
    __declspec(naked) void CGxString_GetGlyphYMetrics_siteHk() { // skip the orig baseline calc
        __asm {
            mov edx, [ecx + 54h];
            pushad;
            push ecx;
            call MSDFFont_Get;
            add esp, 4;
            test al, al;
            popad;
            jz font_unsafe;
            xor ecx, ecx;
            jmp CGxString__GetGlyphYMetrics_site_jmpback;
        font_unsafe:
            mov ecx, [edx + 68h];
            jmp CGxString__GetGlyphYMetrics_site_jmpback;
        }
    }

    // allocate as much as needed for every cgxstring in a batch to perform exactly one draw at max
    // makes sure the overlap from having a single piece of text rendered with multiple draws isn't happening
    // this is probably overkill and irrelevant for 99.99% of the actual draw cases where text vertCount is naturally < hardcoded 2004-era 2048 buffer
    // but...
    __declspec(naked) void CGxDevice__AllocateFontIndexBuffer_siteHk() {
        __asm {
            mov ebx, 3FFFh;
            jmp CGxDevice__AllocateFontIndexBuffer_site_jmpback;
        }
    }
    __declspec(naked) void CGxDevice__InitFontIndexBuffer_siteHk() {
        __asm {
            push 30000h;
            push 0;
            push 1;
            call CGxDevice::PoolCreateFn;
            mov ecx, dword ptr ds : [0C5DF88h] ; // g_theGxDevicePtr
            push 0;
            push 1801Ah;
            jmp CGxDevice__InitFontIndexBuffer_site_jmpback;
        }
    }
    __declspec(naked) void IGxuFontProcessBatch_siteHk() {
        __asm {
            mov g_runtimeVBSize, 0;
            pop ebx;
            pop esi;
            mov esp, ebp;
            pop ebp;
            jmp IGxuFontProcessBatch_site_jmpback;
        }
    }
    __declspec(naked) void CGxDevice__BufStream_siteHk() { // clamp to [2048-65532]
        __asm {
            mov eax, g_runtimeVBSize;
            cmp eax, 800h;
            jge check_upper;
            mov eax, 800h;
            jmp do_push;
        check_upper:
            cmp eax, 0FFFCh;
            jle do_push;
            mov eax, 0FFFCh;
        do_push:
            mov g_runtimeVBSize, eax;
            push eax;
            jmp CGxDevice__BufStream_site_jmpback;
        }
    }
    __declspec(naked) void bufalloc_1_siteHk() {
        __asm {
            xor eax, eax;
            mov esi, 0B4h;
            mov ebx, g_runtimeVBSize;
            jmp bufalloc_1_site_jmpback;
        }
    }
    __declspec(naked) void bufalloc_2_siteHk() {
        __asm {
            cmp ebx, g_runtimeVBSize;
            jz orig_skip;
            mov ecx, g_runtimeVBSize;
            sub ecx, ebx;
            push ecx;
            lea edx, [ebp - 18h];
            push edx;
            call CGxDevice::FlushBufferFn;
            add esp, 8;
            mov edi, eax;
            mov ebx, g_runtimeVBSize;
            jmp bufalloc_2_site_jmpback;
        orig_skip:
            jmp bufalloc_2_site_jmpback;
        }
    }
    __declspec(naked) void bufalloc_3_siteHk() {
        __asm {
            push g_runtimeVBSize;
            push eax;
            call CGxDevice::FlushBufferFn;
            add esp, 8;
            mov edi, eax;
            mov ebx, g_runtimeVBSize;
            jmp bufalloc_3_site_jmpback;
        }
    }


    int __cdecl FreeType_NewMemoryFaceHk(FT_Library library, const FT_Byte* file_base,
        FT_Long file_size, FT_Long face_index, FT_Face* aface) {
        if (!MSDF::g_realFtLibrary && FT_Init_FreeType(&MSDF::g_realFtLibrary) != 0) return -1;

        const int result = FT_New_Memory_Face(library, file_base, file_size, face_index, aface);
        if (result != 0 || !aface || !*aface) return result;

        MSDFFont::Register(*aface, file_base, file_size);
        return result;
    }

    int __cdecl FreeType_SetPixelSizesHk(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height) {
        return FT_Set_Pixel_Sizes(face, pixel_width, pixel_height);
    }

    int __cdecl FreeType_LoadGlyphHk(FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags) {
        return FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING); // original flags CTD
    }

    FT_UInt __cdecl FreeType_GetCharIndexHk(FT_Face face, FT_ULong charcode) {
        return FT_Get_Char_Index(face, charcode);
    }

    int __cdecl FreeType_GetKerningHk(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning) {
        return FT_Get_Kerning(face, left_glyph, right_glyph, kern_mode, akerning);
    }

    int __cdecl FreeType_Done_FaceHk(FT_Face face) {
        MSDFFont::Unregister(face);
        return FT_Done_Face(face);
    }

    int __cdecl FreeType_Done_FreeTypeHk(FT_Library library) {
        MSDFFont::Shutdown();
        if (MSDF::g_msdfFreetype) {
            msdfgen::deinitializeFreetype(MSDF::g_msdfFreetype);
            MSDF::g_msdfFreetype = nullptr;
        }
        if (MSDF::g_realFtLibrary) {
            FT_Done_FreeType(MSDF::g_realFtLibrary);
            MSDF::g_realFtLibrary = nullptr;
        }
        return 0;
    }

    int __cdecl FreeType_NewFaceHk(int* library, int face_descriptor_ptr) {
        return 1; // ../Fonts/* init at startup, falls back to FreeType_NewMemoryFaceHk
    }

    int __cdecl FreeType_InitHk(void* memory, FT_Library* alibrary) {
        if (!MSDF::INITIALIZED) {
            std::string localeStr = MSDF::GetGameLocale();
            const char* locale = localeStr.c_str();
            MSDF::IS_CJK = locale && (strcmp(locale, "zhCN") == 0 ||
                strcmp(locale, "zhTW") == 0 ||
                strcmp(locale, "koKR") == 0);

            MSDF::INITIALIZED = true;
            MSDF::ALLOW_UNSAFE_FONTS = std::atoi(s_cvar_MSDFMode->m_str) > 1;

            if (MSDF::IS_CJK) return FreeType::InitFn(memory, alibrary);

            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            Hooks::Detour(&FreeType::NewMemoryFaceFn, FreeType_NewMemoryFaceHk);
            Hooks::Detour(&FreeType::NewFaceFn, FreeType_NewFaceHk);
            Hooks::Detour(&FreeType::Done_FaceFn, FreeType_Done_FaceHk);
            Hooks::Detour(&FreeType::SetPixelSizesFn, FreeType_SetPixelSizesHk);
            Hooks::Detour(&FreeType::GetCharIndexFn, FreeType_GetCharIndexHk);
            Hooks::Detour(&FreeType::LoadGlyphFn, FreeType_LoadGlyphHk);
            Hooks::Detour(&FreeType::GetKerningFn, FreeType_GetKerningHk);
            Hooks::Detour(&FreeType::Done_FreeTypeFn, FreeType_Done_FreeTypeHk);

            Hooks::Detour(&CGxuFont::RenderBatchFn, CGxuFontRenderBatchHk);
            Hooks::Detour(&CGxuFont::RenderGlyphFn, GxuFontGlyphRenderGlyphHk);
            Hooks::Detour(&CGxFont::GetOrCreateGlyphEntryFn, CGxString__GetOrCreateGlyphEntryHk);

            Hooks::Detour(&CGxDevice__AllocateFontIndexBuffer_site, CGxDevice__AllocateFontIndexBuffer_siteHk);
            Hooks::Detour(&CGxDevice__InitFontIndexBuffer_site, CGxDevice__InitFontIndexBuffer_siteHk);

            Hooks::Detour(&CGxString__GetGlyphYMetrics_site, CGxString_GetGlyphYMetrics_siteHk);
            Hooks::Detour(&CGxString__CheckGeometry_site, CGxString__CheckGeometry_siteHk);
            Hooks::Detour(&CGxString__CheckGeometry_call, CGxString__CheckGeometry_callHk);

            Hooks::Detour(&IGxuFontProcessBatch_site, IGxuFontProcessBatch_siteHk);
            Hooks::Detour(&CGxDevice__BufStream_site, CGxDevice__BufStream_siteHk);
            Hooks::Detour(&bufalloc_1_site, bufalloc_1_siteHk);
            Hooks::Detour(&bufalloc_2_site, bufalloc_2_siteHk);
            Hooks::Detour(&bufalloc_3_site, bufalloc_3_siteHk);

            Hooks::Detour(&CGxString::CheckGeometryFn, CGxString__CheckGeometryHk);
            Hooks::Detour(&CGxString::WriteGeometryFn, CGxString__WriteGeometryHk);
            Hooks::Detour(&CGxString::InitializeTextLineFn, CGxString__InitializeTextLineHk);
            DetourTransactionCommit();

            D3D::RegisterOnDestroy([]() {
                if (s_cachedPS) { s_cachedPS->Release(); s_cachedPS = nullptr; }
                if (s_cachedVS) { s_cachedVS->Release(); s_cachedVS = nullptr; }
                MSDFFont::ClearAllCache();
                });

            D3D::RegisterPixelShaderInit([](CGxDevice::ShaderData* shaderData) {
                if (shaderData != MSDF::g_FontPixelShader && MSDF::g_FontPixelShader != nullptr) return;
                if (!s_cachedPS) {
                    s_cachedPS = D3D::CompilePixelShader({
                        .shaderCode = pixelShaderHLSL,
                        .target = "ps_3_0"
                        });
                }
                if (s_cachedPS) {
                    if (shaderData->pixel_shader) {
                        reinterpret_cast<IDirect3DPixelShader9*>(shaderData->pixel_shader)->Release();
                    }
                    shaderData->pixel_shader = s_cachedPS;
                    shaderData->compilation_flags = 1;
                }
                });

            s_cachedPS = D3D::CompilePixelShader({
                .shaderCode = pixelShaderHLSL,
                .target = "ps_3_0"
                });
            if (s_cachedPS) {
                if (MSDF::g_FontPixelShader->pixel_shader) MSDF::g_FontPixelShader->pixel_shader->Release();
                MSDF::g_FontPixelShader->pixel_shader = s_cachedPS;
                MSDF::g_FontPixelShader->compilation_flags = 1;
            }

            D3D::RegisterVertexShaderInit([](CGxDevice::ShaderData* shaderData) {
                if (shaderData != MSDF::g_FontVertexShader && MSDF::g_FontVertexShader != nullptr) return;
                if (!s_cachedVS) {
                    s_cachedVS = D3D::CompileVertexShader({
                        .shaderCode = vertexShaderHLSL,
                        .target = "vs_3_0"
                        });
                }
                if (s_cachedVS) {
                    if (shaderData->vertex_shader) shaderData->vertex_shader->Release();
                    shaderData->vertex_shader = s_cachedVS;
                    shaderData->compilation_flags = 1;
                }
                });

            s_cachedVS = D3D::CompileVertexShader({
                .shaderCode = vertexShaderHLSL,
                .target = "vs_3_0"
                });
            if (s_cachedVS) {
                if (MSDF::g_FontVertexShader->vertex_shader) MSDF::g_FontVertexShader->vertex_shader->Release();
                MSDF::g_FontVertexShader->vertex_shader = s_cachedVS;
                MSDF::g_FontVertexShader->compilation_flags = 1;
            }

            s_prefetchPayload.reserve(16383);

            CGxDevice::InitFontIndexBufferFn(); // engine has already run it at this point
        }
        else if (MSDF::IS_CJK) {
            return FreeType::InitFn(memory, alibrary);
        }
        if (const FT_Error error = FT_Init_FreeType(&MSDF::g_realFtLibrary)) return error;

        if (alibrary) *alibrary = MSDF::g_realFtLibrary;

        MSDF::g_msdfFreetype = msdfgen::initializeFreetype();
        if (!MSDF::g_msdfFreetype) {
            FT_Done_FreeType(MSDF::g_realFtLibrary);
            MSDF::g_realFtLibrary = nullptr;
            return -1;
        }
        return 0;
    }

    int CVarHandler_MSDFMode(CVar*, const char*, const char* value, void*) {
        if (std::atoi(value) == 1) {
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            Hooks::Detour(&FreeType::InitFn, FreeType_InitHk);
            DetourTransactionCommit();
        }
        return 1;
    }
}

void MSDF::initialize() {
    Hooks::FrameXML::registerCVar(&s_cvar_MSDFMode, "MSDFMode", nullptr, "1", CVarHandler_MSDFMode);
};
