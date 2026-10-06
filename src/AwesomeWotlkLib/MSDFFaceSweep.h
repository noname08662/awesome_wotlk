#pragma once

#include <ankerl/unordered_dense.h>
#include <ft2build.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <stop_token>
#include <vector>

#include FT_FREETYPE_H

#include "MSDFCache.h"
#include "MSDFUtils.h"

class MSDFFaceSweep {
public:
    struct AddonFace {
        const std::filesystem::path& kPath;
        std::span<const uint8_t> data;
        FontHash hash;
        FT_Face face;
        msdfgen::FontHandle* handle;
        const char* family;
        const char* style;
    };

    using AddonFaceVisitor = std::function<void(const AddonFace&)>;

    static void start();
    static void stop();

    static void forEachAddonFace(
        FT_Library library, std::vector<uint8_t>& buffer, const std::stop_token& stop, const AddonFaceVisitor& visit);

private:
    struct Scratch {
        std::vector<uint8_t> font_data;
        std::vector<MSDFCache::ManifestEntry> entries;
        MSDFCache::ManifestMap manifest;
    };

    static void run(const std::stop_token& stop);
    static void visitFile(FT_Library library, const std::filesystem::path& path, std::vector<uint8_t>& buffer,
        ankerl::unordered_dense::set<FontHash>& seen, const AddonFaceVisitor& visit);
    static void sweepFace(const AddonFace& font, Scratch& scratch);
};
