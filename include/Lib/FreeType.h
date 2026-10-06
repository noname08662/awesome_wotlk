#pragma once

#include FT_FREETYPE_H
#include "Graphics/GxuFontUtil.h"
#include "freetype/fttypes.h"

namespace freetype {
inline static auto& ft_library = *reinterpret_cast<FT_Library*>(0x00C7D2B4);
inline static auto& ft_memory_interface = *reinterpret_cast<font_util::FreetypeMemInterface*>(0x00AD9960);

static constexpr FT_Int32 kLoadFlagsMono =
    FT_LOAD_LINEAR_DESIGN | FT_LOAD_PEDANTIC | FT_LOAD_CROP_BITMAP | FT_LOAD_NO_HINTING;
static constexpr FT_Int32 kLoadFlagsGray =
    FT_LOAD_LINEAR_DESIGN | FT_LOAD_PEDANTIC | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING;

// memory, alibrary
HOOKKIT_HOOK_HANDLE(init, 0x00991320, hookkit::Conv::eCdecl, int, void*, FT_Library*);
// library, file base, file size, face idx, aface
HOOKKIT_HOOK_HANDLE(
    newMemoryFace, 0x00993370, hookkit::Conv::eCdecl, int, FT_Library, const FT_Byte*, FT_Long, FT_Long, FT_Face*);
HOOKKIT_HOOK_HANDLE(doneFace, 0x00992610, hookkit::Conv::eCdecl, int, FT_Face);
// face, width, height
HOOKKIT_HOOK_HANDLE(setPixelSizes, 0x00992780, hookkit::Conv::eCdecl, int, FT_Face, FT_UInt, FT_UInt);
// face, charcode
HOOKKIT_HOOK_HANDLE(getCharIndex, 0x009911A0, hookkit::Conv::eCdecl, FT_UInt, FT_Face, FT_ULong);
// face, glyph idx, load flags
HOOKKIT_HOOK_HANDLE(loadGlyph, 0x00992DA0, hookkit::Conv::eCdecl, int, FT_Face, FT_ULong, FT_Int32);
// face left glyph, right glyph, kern mode, akerning
HOOKKIT_HOOK_HANDLE(
    getKerning, 0x00991050, ::hookkit::Conv::eCdecl, int, FT_Face, FT_UInt, FT_UInt, FT_UInt, FT_Vector*);
HOOKKIT_HOOK_HANDLE(doneFreeType, 0x00992CB0, hookkit::Conv::eCdecl, int, FT_Library);
// library, face desc
HOOKKIT_HOOK_HANDLE(newFace, 0x009931A0, hookkit::Conv::eCdecl, int, int*, int);
}  // namespace freetype
