#include <MSDFFont.h>
#include <MSDFCache.h>
#include <MSDFPregen.h>
#include <MSDFShaders.h>
#include <Detours/detours.h>

int __cdecl MSDF::FreeType_NewMemoryFace_hk(FT_Library library, const FT_Byte* file_base,
    FT_Long file_size, FT_Long face_index, FT_Face* aface) {
    if (!g_realFtLibrary && FT_Init_FreeType(&g_realFtLibrary) != 0) return -1;

    const int result = FT_New_Memory_Face(library, file_base, file_size, face_index, aface);
    if (result != 0 || !aface || !*aface) return result;

    MSDFFont::Register(*aface, file_base, file_size, face_index);
    if (IS_PREGEN) MSDFPregen::RegisterForPreGen(*aface, file_base, file_size, face_index);
    return result;
}

int __cdecl MSDF::FreeType_SetPixelSizes_hk(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height) {
    return FT_Set_Pixel_Sizes(face, pixel_width, pixel_height);
}

int __cdecl MSDF::FreeType_LoadGlyph_hk(FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags) {
    return FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_NO_HINTING); // original flags CTD
}

FT_UInt __cdecl MSDF::FreeType_GetCharIndex_hk(FT_Face face, FT_ULong charcode) {
    return FT_Get_Char_Index(face, charcode);
}

int __cdecl MSDF::FreeType_GetKerning_hk(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning) {
    return FT_Get_Kerning(face, left_glyph, right_glyph, kern_mode, akerning);
}

int __cdecl MSDF::FreeType_Done_Face_hk(FT_Face face) {
    MSDFFont::Unregister(face);
    return FT_Done_Face(face);
}

int __cdecl MSDF::FreeType_Done_FreeType_hk(FT_Library library) {
    MSDFFont::Shutdown();
    if (g_msdfFreetype) {
        msdfgen::deinitializeFreetype(g_msdfFreetype);
        g_msdfFreetype = nullptr;
    }
    if (g_realFtLibrary) {
        FT_Done_FreeType(g_realFtLibrary);
        g_realFtLibrary = nullptr;
    }
    return 0;
}

int __cdecl MSDF::FreeType_NewFace_hk(int* library, int face_descriptor_ptr) {
    return 1; // ../Fonts/* init at startup, falls back to FreeType_NewMemoryFace_hk
}


void __cdecl MSDF::PrefetchCodepoints(CGxString* pThis) {
    if (s_prefetchPayload.empty()) return;
    if (!pThis || reinterpret_cast<uintptr_t>(pThis) & 1) return;

    // sort everything in a batch (no idea if 1 batch == 1 face, though)
    MSDFFont* fontHandle = MSDFFont::Get(pThis->m_fontObj->m_fontResource->fontFace);
    if (fontHandle) {
        std::sort(s_prefetchPayload.begin(), s_prefetchPayload.end());
        s_prefetchPayload.erase(std::unique(s_prefetchPayload.begin(), s_prefetchPayload.end()), s_prefetchPayload.end());
        for (uint32_t codepoint : s_prefetchPayload) {
            fontHandle->GetGlyph(codepoint);
        }
    }
    s_prefetchPayload.clear();
}

void __fastcall MSDF::ProcessGeometry(CGxString* pThis) {
    CFontObject* fontObj = pThis->m_fontObj;
    FT_Face fontFace = fontObj->m_fontResource->fontFace;
    MSDFFont* fontHandle = MSDFFont::Get(fontFace);
    if (!fontHandle) return;

    CFontGeomBatch* batch = pThis->m_geomBuffers[0];
    if (!batch || batch->m_verts.m_count < 4) return;

    TSGrowableArray<CFontVertex>& verts = batch->m_verts;
    if (verts.m_count < 4) return;

    const uint32_t flags = fontObj->m_atlasPages[0].m_flags;
    const bool is3d = pThis->m_flags & 0x80; // native 3d obj - nameplate text, etc.
    const double fontSizeMult = pThis->m_fontSizeMult;
    const double fontOffs = !is3d ? ((flags & 8) ? 4.5 : ((flags & 1) ? 2.5 : 0.0)) : 0.0;
    const double baselineOffs = (fontOffs > 0.0) ? 1.0 : 0.0;
    const double scale = (is3d ? fontSizeMult : MSDF::GetFontEffectiveHeight_orig(is3d, fontSizeMult) * 0.98) / MSDF::SDF_RENDER_SIZE; // 0.98 compensation
    const double pad = MSDF::SDF_SPREAD * scale;

    for (uint32_t q = 0; q < verts.m_count; q += 4) {
        CFontVertex* vBase = &verts.m_data[q];
        if (vBase[0].u > 1.0f) {
            const uint32_t codepoint = vBase[0].u - 1.0f;

            const GlyphMetrics* gm = fontHandle->GetGlyph(codepoint);
            if (!gm) continue;

            CGlyphCacheEntry* entry = MSDF::CGxString__GetOrCreateGlyphEntry_orig(fontObj, codepoint);
            if (!entry) continue;

            CFontVertex* vert0 = &vBase[0];
            CFontVertex* vert1 = &vBase[1];
            CFontVertex* vert2 = &vBase[2];
            CFontVertex* vert3 = &vBase[3];

            const double leftOffs = MSDF::CGxString__GetBearingX_orig(fontObj, entry, is3d, fontSizeMult);
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


bool __fastcall MSDF::CGxString__CheckGeometry_hk(CGxString* pThis) {
    MSDFFont* fontHandle = MSDFFont::Get(pThis->m_fontObj->m_fontResource->fontFace);
    if (fontHandle) {
        // force re-calc geometry if any msdf page was evicted
        uint32_t highByte = (pThis->m_flags >> 24) & 0xFF;
        bool isSet = (highByte & 0x80) != 0;
        if (isSet) {
            uint8_t storedVersion = highByte & 0x7F;
            uint8_t currentVersion = static_cast<uint8_t>(fontHandle->GetAtlasEvictionCount() & 0x7F);
            if (storedVersion != currentVersion) {
                CGxString__ClearInstanceData_orig(pThis);
                pThis->m_flags &= 0x00FFFFFF;
            }
        }
    }

    CFontGeomBatch* batch = pThis->m_geomBuffers[0];
    if (!batch || !batch->m_verts.m_data) return CGxString__CheckGeometry_orig(pThis);

    g_runtimeVBSize += CGxString__GetVertCountForPage_orig(pThis, 0);
    return CGxString__CheckGeometry_orig(pThis);
}

void __fastcall MSDF::CGxString__WriteGeometry_hk(CGxString* pThis, void* edx, int destPtr, int index, int vertIndex, int vertCount) {
    CGxString__WriteGeometry_orig(pThis, destPtr, index, vertIndex, vertCount);

    MSDFFont* fontHandle = MSDFFont::Get(pThis->m_fontObj->m_fontResource->fontFace);
    if (!fontHandle) {
        IDirect3DDevice9* device = D3D::GetDevice();
        if (device) {
            const float resetControl[4] = { 0, 0, 0, 0 };
            device->SetPixelShaderConstantF(SDF_SAMPLER_SLOT, resetControl, 1);
            device->SetVertexShaderConstantF(SDF_SAMPLER_SLOT, resetControl, 1);
        }
        return;
    }

    IDirect3DDevice9* device = D3D::GetDevice();
    if (!device) return;

    for (uint32_t pageIdx = 0; pageIdx < fontHandle->GetAtlasPageCount(); ++pageIdx) {
        auto* atlasTexture = fontHandle->GetAtlasPage(pageIdx);
        if (atlasTexture && atlasTexture->texture) {
            uint32_t slot = (/* max d3d9 tex slots */ 15 - MAX_ATLAS_PAGES + 1) + pageIdx;
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
        is3d ? pThis->m_fontObj->m_rasterTargetSize : GetFontEffectiveHeight_orig(is3d, pThis->m_fontSizeMult),
        is3d ? 0.0f : ((flags & 8) ? 2.0f : ((flags & 1) ? 1.0f : 0.0f)),
        SDF_SPREAD, ATLAS_SIZE
    };
    device->SetPixelShaderConstantF(SDF_SAMPLER_SLOT, controlFlag, 1);
    device->SetVertexShaderConstantF(SDF_SAMPLER_SLOT, controlFlag, 1);

    return;
}

void __fastcall MSDF::IGxuFontRenderBatch_hk(IGxuFontObj* pThis) {
    IGxuFontRenderBatch_orig(pThis);

    IDirect3DDevice9* device = D3D::GetDevice();
    if (device) {
        // reset since font PS also handles UI elements
        const float resetControl[4] = { 0, 0, 0, 0 };
        device->SetPixelShaderConstantF(SDF_SAMPLER_SLOT, resetControl, 1);
        device->SetVertexShaderConstantF(SDF_SAMPLER_SLOT, resetControl, 1);
    }
    return;
}

void __fastcall MSDF::IGxuFontRenderBatch_hk_pregen(IGxuFontObj* pThis) {
    if ((GetAsyncKeyState(MSDF::PREGEN_START_KEY) & 0x8000) != 0) MSDFPregen::TryStartPreGen();
    IGxuFontRenderBatch_orig(pThis);
}

char __cdecl MSDF::IGxuFontGlyphRenderGlyph_hk(FT_Face fontFace, uint32_t fontSize, uint32_t codepoint, uint32_t pageInfo, CGlyphMetrics* resultBuffer, uint32_t outline_flag, uint32_t pad) {
    const char result = IGxuFontGlyphRenderGlyph_orig(fontFace, fontSize, codepoint, pageInfo, resultBuffer, outline_flag, pad);
    if (MSDFFont::Get(fontFace)) {
        // verAdv is distance from the top of the quad to the top of the glyph, bearingY is the quad's vert correction (descenders)
        resultBuffer->m_bearingY -= resultBuffer->m_verAdv;
    }
    return result;
}

CGlyphCacheEntry* __fastcall MSDF::CGxString__GetOrCreateGlyphEntry_hk(CFontObject* fontObj, void* edx, uint32_t codepoint) {
    CGlyphCacheEntry* result = CGxString__GetOrCreateGlyphEntry_orig(fontObj, codepoint);
    if (result && MSDFFont::Get(fontObj->m_fontResource->fontFace)) {
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

int __fastcall MSDF::CGxString__InitializeTextLine_hk(CGxString* pThis, void* edx, char* text, int textLength, int* a4, C3Vector* startPos, void* a6, int a7) {
    const int result = CGxString__InitializeTextLine_orig(pThis, text, textLength, a4, startPos, a6, a7);

    // 1-st pass - only collect codepoints
    // all of thes will then get sorted to ensure sequentiality
    // == minimal syscall churn @ GetGlyph -> cold cache path == less stutters
    uint32_t highByte = (pThis->m_flags >> 24) & 0xFF;
    bool isSet = (highByte & 0x80) != 0;
    if (!isSet) {
        if (pThis->m_flags & 0x40000000) return result;
        for (char* p = pThis->m_text; *p; ++p) {
            s_prefetchPayload.push_back(static_cast<uint8_t>(*p));
        }
        pThis->m_flags |= 0x40000000;
    }
    else {
        ProcessGeometry(pThis);
    }
    return result;
}

// run the original loop, then prefetch once per frame
// drop out back to loopstart and loop again (ebx preserved)
// this time, call processGeom to resolve, now with a warmed up cache
__declspec(naked) static void MSDF::CGxString__CheckGeometry_site_hk()
{
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
        call CGxString__CheckGeometry_hk;

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

__declspec(naked) static void MSDF::CGxString__CheckGeometry_call_hk()
{
    __asm {
        mov ecx, ebx;
        call ProcessGeometry;
        jmp CGxString__CheckGeometry_call_jmpback;
    }
}


static bool __cdecl MSDFFont_Get(FT_Face face) { return MSDFFont::Get(face); }
__declspec(naked) static void MSDF::CGxString_GetGlyphYMetrics_site_hk() // skip the orig baseline calc
{
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
__declspec(naked) static void MSDF::CGxDevice__AllocateFontIndexBuffer_site_hk() {
    __asm {
        mov ebx, 3FFFh;
        jmp CGxDevice__AllocateFontIndexBuffer_site_jmpback;
    }
}
__declspec(naked) static void MSDF::CGxDevice__InitFontIndexBuffer_site_hk() {
    __asm {
        push 30000h;
        push 0;
        push 1;
        call CGxDevice__PoolCreate_orig;
        mov ecx, dword ptr ds:[0C5DF88h]; // g_theGxDevicePtr
        push 0;
        push 1801Ah;
        jmp CGxDevice__InitFontIndexBuffer_site_jmpback;
    }
}
__declspec(naked) static void MSDF::IGxuFontProcessBatch_site_hk() {
    __asm {
        mov g_runtimeVBSize, 0;
        pop ebx;
        pop esi;
        mov esp, ebp;
        pop ebp;
        jmp IGxuFontProcessBatch_site_jmpback;
    }
}
__declspec(naked) static void MSDF::CGxDevice__BufStream_site_hk() { // clamp to [2048-65532]
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
__declspec(naked) static void MSDF::bufalloc_1_site_hk() {
    __asm {
        xor eax, eax;
        mov esi, 0B4h;
        mov ebx, g_runtimeVBSize;
        jmp bufalloc_1_site_jmpback;
    }
}
__declspec(naked) static void MSDF::bufalloc_2_site_hk() {
    __asm {
        cmp ebx, g_runtimeVBSize;
        jz orig_skip;
        mov ecx, g_runtimeVBSize;
        sub ecx, ebx;
        push ecx;
        lea edx, [ebp - 18h];
        push edx;
        call CGxDevice__FlushBuffer_orig;
        add esp, 8;
        mov edi, eax;
        mov ebx, g_runtimeVBSize;
        jmp bufalloc_2_site_jmpback;
    orig_skip:
        jmp bufalloc_2_site_jmpback;
    }
}
__declspec(naked) static void MSDF::bufalloc_3_site_hk() {
    __asm {
        push g_runtimeVBSize;
        push eax;
        call CGxDevice__FlushBuffer_orig;
        add esp, 8;
        mov edi, eax;
        mov ebx, g_runtimeVBSize;
        jmp bufalloc_3_site_jmpback;
    }
}


int __cdecl MSDF::FreeType_Init_hk(void* memory, FT_Library* alibrary) {
    if (!INITIALIZED) {
        const int mode = std::atoi(s_cvar_MSDFMode->vStr);

        std::string localeStr = GetGameLocale();
        const char* locale = localeStr.c_str();
        IS_CJK = locale && (strcmp(locale, "zhCN") == 0 ||
            strcmp(locale, "zhTW") == 0 ||
            strcmp(locale, "koKR") == 0);

        INITIALIZED = true;
        MODE = IS_CJK ? 0 : mode;
        ALLOW_UNSAFE_FONTS = mode > 1;

        if (mode < 1) return FreeType_Init_orig(memory, alibrary);

        ATLAS_SIZE = IS_CJK ? ATLAS_SIZE_CJK : ATLAS_SIZE;

        DetourTransactionBegin();
        DetourAttach(&(LPVOID&)FreeType_NewMemoryFace_orig, FreeType_NewMemoryFace_hk);
        DetourAttach(&(LPVOID&)FreeType_NewFace_orig, FreeType_NewFace_hk);
        DetourAttach(&(LPVOID&)FreeType_Done_Face_orig, FreeType_Done_Face_hk);
        DetourAttach(&(LPVOID&)FreeType_SetPixelSizes_orig, FreeType_SetPixelSizes_hk);
        DetourAttach(&(LPVOID&)FreeType_GetCharIndex_orig, FreeType_GetCharIndex_hk);
        DetourAttach(&(LPVOID&)FreeType_LoadGlyph_orig, FreeType_LoadGlyph_hk);
        DetourAttach(&(LPVOID&)FreeType_GetKerning_orig, FreeType_GetKerning_hk);
        DetourAttach(&(LPVOID&)FreeType_Done_FreeType_orig, FreeType_Done_FreeType_hk);
        if (IS_PREGEN) {
            DetourAttach(&(LPVOID&)IGxuFontRenderBatch_orig, IGxuFontRenderBatch_hk_pregen);
        }
        else {
            DetourAttach(&(LPVOID&)IGxuFontRenderBatch_orig, IGxuFontRenderBatch_hk);
            DetourAttach(&(LPVOID&)IGxuFontGlyphRenderGlyph_orig, IGxuFontGlyphRenderGlyph_hk);

            DetourAttach(&(LPVOID&)CGxDevice__AllocateFontIndexBuffer_site, CGxDevice__AllocateFontIndexBuffer_site_hk);
            DetourAttach(&(LPVOID&)CGxDevice__InitFontIndexBuffer_site, CGxDevice__InitFontIndexBuffer_site_hk);

            DetourAttach(&(LPVOID&)CGxString__GetGlyphYMetrics_site, CGxString_GetGlyphYMetrics_site_hk);

            DetourAttach(&(LPVOID&)CGxString__CheckGeometry_site, CGxString__CheckGeometry_site_hk);
            DetourAttach(&(LPVOID&)CGxString__CheckGeometry_call, CGxString__CheckGeometry_call_hk);

            DetourAttach(&(LPVOID&)IGxuFontProcessBatch_site, IGxuFontProcessBatch_site_hk);
            DetourAttach(&(LPVOID&)CGxDevice__BufStream_site, CGxDevice__BufStream_site_hk);
            DetourAttach(&(LPVOID&)bufalloc_1_site, bufalloc_1_site_hk);
            DetourAttach(&(LPVOID&)bufalloc_2_site, bufalloc_2_site_hk);
            DetourAttach(&(LPVOID&)bufalloc_3_site, bufalloc_3_site_hk);

            DetourAttach(&(LPVOID&)CGxString__CheckGeometry_orig, CGxString__CheckGeometry_hk);
            DetourAttach(&(LPVOID&)CGxString__WriteGeometry_orig, CGxString__WriteGeometry_hk);
            DetourAttach(&(LPVOID&)CGxString__GetOrCreateGlyphEntry_orig, CGxString__GetOrCreateGlyphEntry_hk);
            DetourAttach(&(LPVOID&)CGxString__InitializeTextLine_orig, CGxString__InitializeTextLine_hk);
        }
        DetourTransactionCommit();

        D3D::RegisterOnDestroy([]() {
            if (s_cachedPS) { s_cachedPS->Release(); s_cachedPS = nullptr; }
            if (s_cachedVS) { s_cachedVS->Release(); s_cachedVS = nullptr; }
            MSDFFont::ClearAllCache();
            });

        D3D::RegisterPixelShaderInit([](D3D::ShaderData* shaderData) {
            if (shaderData != g_FontPixelShader && g_FontPixelShader != nullptr) return;
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
            if (g_FontPixelShader->pixel_shader) {
                reinterpret_cast<IDirect3DPixelShader9*>(g_FontPixelShader->pixel_shader)->Release();
            }
            g_FontPixelShader->pixel_shader = s_cachedPS;
            g_FontPixelShader->compilation_flags = 1;
        }

        D3D::RegisterVertexShaderInit([](D3D::ShaderData* shaderData) {
            if (shaderData != g_FontVertexShader && g_FontVertexShader != nullptr) return;
            if (!s_cachedVS) {
                s_cachedVS = D3D::CompileVertexShader({
                    .shaderCode = vertexShaderHLSL,
                    .target = "vs_3_0"
                    });
            }
            if (s_cachedVS) {
                if (shaderData->vertex_shader) {
                    auto* original = reinterpret_cast<IDirect3DVertexShader9*>(shaderData->vertex_shader);
                    original->Release();
                }
                shaderData->vertex_shader = s_cachedVS;
                shaderData->compilation_flags = 1;
            }
            });

        s_cachedVS = D3D::CompileVertexShader({
            .shaderCode = vertexShaderHLSL,
            .target = "vs_3_0"
            });
        if (s_cachedVS) {
            if (g_FontVertexShader->vertex_shader) {
                reinterpret_cast<IDirect3DVertexShader9*>(g_FontVertexShader->vertex_shader)->Release();
            }
            g_FontVertexShader->vertex_shader = s_cachedVS;
            g_FontVertexShader->compilation_flags = 1;
        }

        CGxDevice__InitFontIndexBuffer_orig(); // engine has already run it at this point
    }
    else if (MODE < 1) {
        return FreeType_Init_orig(memory, alibrary);
    }

    const FT_Error error = FT_Init_FreeType(&g_realFtLibrary);
    if (error) return error;

    if (alibrary) *alibrary = static_cast<FT_Library>(g_realFtLibrary);

    g_msdfFreetype = msdfgen::initializeFreetype();
    if (!g_msdfFreetype) {
        FT_Done_FreeType(g_realFtLibrary);
        g_realFtLibrary = nullptr;
        return -1;
    }
    return 0;
}


void MSDF::initialize() {
    s_cvar_MSDFMode = Console::RegisterCVar("MSDFMode", NULL, (Console::CVarFlags)1, "1", CVarHandler_MSDFMode, 0, 0, 0, 0);;
    DetourAttach(&(LPVOID&)FreeType_Init_orig, FreeType_Init_hk);
};