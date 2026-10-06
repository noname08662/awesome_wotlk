#include "MSDFFaceSweep.h"

#include <hookkit/accessor.h>
#include <msdfgen-ext.h>
#include <msdfgen.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "MSDF.h"
#include "MSDFCache.h"
#include "MSDFFont.h"

namespace {
constexpr utils::Accessor<std::jthread, struct SweepThreadTag> kSweepThread;

bool isFontFile(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".ttf" || ext == ".otf";
}
}  // namespace

void MSDFFaceSweep::start() {
    if (kSweepThread->joinable()) { return; }
    *kSweepThread = std::jthread(&MSDFFaceSweep::run);
}

void MSDFFaceSweep::stop() {
    if (!kSweepThread->joinable()) { return; }
    kSweepThread->request_stop();
    kSweepThread->join();
}

void MSDFFaceSweep::forEachAddonFace(
    FT_Library library, std::vector<uint8_t>& buffer, const std::stop_token& stop, const AddonFaceVisitor& visit) {
    ankerl::unordered_dense::set<FontHash> seen;
    std::error_code ec;
    const std::filesystem::path root = std::filesystem::current_path() / "Interface" / "AddOns";
    for (auto it = std::filesystem::recursive_directory_iterator(
             root, std::filesystem::directory_options::skip_permission_denied, ec);
        !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (stop.stop_requested()) { break; }
        if (!it->is_regular_file(ec) || !isFontFile(it->path())) { continue; }
        try {
            visitFile(library, it->path(), buffer, seen, visit);
        } catch (...) {}
    }
}

void MSDFFaceSweep::run(const std::stop_token& stop) {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) { return; }

    Scratch scratch;
    forEachAddonFace(library, scratch.font_data, stop, [&scratch](const AddonFace& font) { sweepFace(font, scratch); });
    FT_Done_FreeType(library);
}

void MSDFFaceSweep::visitFile(FT_Library library, const std::filesystem::path& path, std::vector<uint8_t>& buffer,
    ankerl::unordered_dense::set<FontHash>& seen, const AddonFaceVisitor& visit) {
    if (!readFontFile(path, buffer)) { return; }
    const auto size = static_cast<FT_Long>(buffer.size());

    const FontHash hash = hashFont(buffer.data(), size);
    if (!seen.insert(hash).second) { return; }

    FT_Face face = nullptr;
    if (FT_New_Memory_Face(library, buffer.data(), size, 0, &face) != 0) { return; }
    const FinalAction done_face([face] { FT_Done_Face(face); });

    const char* family = face->family_name != nullptr ? face->family_name : "Unknown";
    const char* style = face->style_name != nullptr ? face->style_name : "";
    if (MSDFCache::isFontBlacklisted(family, style, buffer.data(), buffer.size())) { return; }

    msdfgen::FontHandle* handle = msdfgen::adoptFreetypeFont(face);
    if (handle == nullptr) { return; }
    const FinalAction destroy_handle([handle] { msdfgen::destroyFont(handle); });

    msdfgen::FontMetrics metrics{};
    if (!msdfgen::getFontMetrics(metrics, handle) || !(metrics.emSize > 0.0)) { return; }

    visit({
        .kPath = path,
        .data = buffer,
        .hash = hash,
        .face = face,
        .handle = handle,
        .family = family,
        .style = style
    });
}

void MSDFFaceSweep::sweepFace(const AddonFace& font, Scratch& scratch) {
    constexpr MSDFCache::CacheKey kKey{.sdf_render_size = msdf::kSdfRenderSize, .sdf_spread = msdf::kSdfSpread};
    const std::filesystem::path base =
        MSDFCache::getCacheBasePath(font.family, font.style, kKey.sdf_render_size, kKey.sdf_spread, font.hash);
    const std::filesystem::path manifest_path = base / MSDFCache::kManifestFile;
    const std::filesystem::path journal_path = base / MSDFCache::kManifestJournalFile;

    MSDFCache::ManifestMap& map = scratch.manifest;
    MSDFCache::FaceRecord record;
    if (MSDFCache::readManifest(manifest_path, journal_path, kKey, font.hash, map, record, scratch.entries) &&
        record.verdict != MSDFCache::FaceVerdict::eUnknown) {
        return;
    }
    record = MSDFFont::evaluateFace(font.handle);

    std::error_code ec;
    std::filesystem::create_directories(base, ec);

    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
    const FinalAction resume_background([] { SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN); });
    ScopedFileLock lock;
    if (!lock.acquireExclusive(base / MSDFCache::kManifestLockFile, 1000)) { return; }

    MSDFCache::FaceRecord on_disk;
    if (!MSDFCache::readManifest(manifest_path, journal_path, kKey, font.hash, map, on_disk, scratch.entries)) {
        return;
    }
    record.pregen_complete = on_disk.pregen_complete;
    static_cast<void>(
        MSDFCache::writeManifest(manifest_path, journal_path, kKey, font.hash, map, record, scratch.entries));
}
