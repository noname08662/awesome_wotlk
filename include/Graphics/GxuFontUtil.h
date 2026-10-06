#pragma once

#include <freetype/freetype.h>

#include <cstdint>

#include "CGxFont.h"

namespace font_util {
struct FreetypeMemInterface {
    unk_t pad;
    void*(__thiscall* alloc)(int unused, uint32_t bytes);
    void*(__thiscall* free)(int unused, uint32_t bytes);
};

// face, font size, codepoint, baseline, out, monochrome, outline pad
HOOKKIT_HOOK_HANDLE(initGlyph, 0x006C8CC0, hookkit::Conv::eCdecl, bool, FT_Face, uint32_t, uint32_t, uint32_t,
    GLYPHBITMAPDATA*, uint32_t, uint32_t);
HOOKKIT_HOOK_HANDLE(getFontEffectiveWidth, 0x006C0B60, hookkit::Conv::eCdecl, double, int, float);
HOOKKIT_HOOK_HANDLE(getFontEffectiveHeight, 0x006C0B20, hookkit::Conv::eCdecl, double, int, float);
HOOKKIT_HOOK_HANDLE(init, 0x006BE230, hookkit::Conv::eCdecl, void);
HOOKKIT_HOOK_HANDLE(onWindowsSizeChanged, 0x006BE020, hookkit::Conv::eCdecl, void);
HOOKKIT_HOOK_HANDLE(createTextureQuads, 0x006BD160, hookkit::Conv::eCdecl, void);
HOOKKIT_HOOK_HANDLE(registerFontFaces, 0x00990650, hookkit::Conv::eCdecl, void, FT_Library);
}  // namespace font_util
