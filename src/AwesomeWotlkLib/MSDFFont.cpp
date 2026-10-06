#include "MSDFFont.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ranges>
#include <span>

#include "D3D.h"
#include "MSDFCache.h"
#include "MSDFUtils.h"
#include "MSDFValidator.h"

const void* MSDFFont::AtlasPage::pixels(int32_t, int32_t mip, uint32_t& pitch) {
    if (mip != 0) { return nullptr; }
    if (upload != nullptr) {
        const Recti& dirty = texture->dirty_rect_;
        if (dirty.min_x != upload_rect.x0 || dirty.min_y != upload_rect.y0 || dirty.max_x != upload_rect.x1 ||
            dirty.max_y != upload_rect.y1) {
            return nullptr;
        }
        pitch = upload_pitch;
        return upload;
    }
    pitch = 0;
    return kZeroRow->data();
}

MSDFFont::MSDFFont(FT_Face face, const FT_Byte* font_data, FT_Long data_size)
    : font_data_(font_data), data_size_(data_size) {
    if (face == nullptr) { return; }

    const char* family = face->family_name != nullptr ? face->family_name : "Unknown";
    const char* style = face->style_name != nullptr ? face->style_name : "";
    if (MSDFCache::isFontBlacklisted(family, style, font_data, static_cast<size_t>(data_size))) { return; }

    msdf_font_ = createMsdfHandle(font_data, data_size);
    if (msdf_font_ == nullptr) { return; }

    msdfgen::FontMetrics font_metrics{};
    if (!msdfgen::getFontMetrics(font_metrics, msdf_font_) || !(font_metrics.emSize > 0.0)) { return; }
    em_size_ = font_metrics.emSize;

    cache_ = std::make_unique<MSDFCache>(font_data, data_size, family, style, msdf::kSdfRenderSize, msdf::kSdfSpread);

    MSDFCache::FaceRecord record = cache_->getFaceRecord();
    if (record.verdict == MSDFCache::FaceVerdict::eUnknown) {
        record = evaluateFace(msdf_font_);
        cache_->setFaceRecord(record);
    }
    face_metrics_ = record.metrics;

    is_valid_ = (msdf::allow_unsafe_fonts || record.verdict == MSDFCache::FaceVerdict::eCompatible) &&
        (!msdf::is_cjk || cache_->getFaceRecord().pregen_complete);
    if (is_valid_) { glyph_pool_.reserve(kGlyphPoolReserve); }
}

MSDFFont::~MSDFFont() {
    glyph_pool_.clear();
    atlas_pages_.clear();
    cache_.reset();
    if (msdf_font_ != nullptr) {
        msdfgen::destroyFont(msdf_font_);
        msdf_font_ = nullptr;
    }
}

MSDFFont::AtlasPage* MSDFFont::getAtlasPage(size_t index) const {
    if (index < atlas_pages_.size()) { return atlas_pages_[index].get(); }
    return nullptr;
}

const GlyphMetrics* MSDFFont::findGlyph(uint32_t codepoint) const {
    const auto it = glyph_pool_.find(codepoint);
    return it != glyph_pool_.end() ? &it->second : nullptr;
}

const GlyphMetrics* MSDFFont::getGlyph(uint32_t codepoint) {
    const auto pit = glyph_pool_.find(codepoint);
    if (pit != glyph_pool_.end()) { return &pit->second; }

    auto [it, inserted] = glyph_pool_.try_emplace(codepoint);
    GlyphMetrics& metrics = it->second;

    if (cache_->tryLoadGlyph(codepoint, metrics)) {
        const GlyphMetrics* placed = uploadGlyphToAtlas(metrics, codepoint);
        if (placed == nullptr) { glyph_pool_.erase(codepoint); }
        return placed;
    }

    GlyphMetricsToStore storage;
    if (!generateGlyphData(codepoint, storage)) {
        glyph_pool_.erase(it);
        return nullptr;
    }

    metrics.width = storage.width;
    metrics.height = storage.height;
    metrics.bitmap_left = storage.bitmap_left;
    metrics.bitmap_top = storage.bitmap_top;

    const GlyphMetrics* placed = &metrics;
    if (storage.width > 0 && storage.height > 0) {
        metrics.pixel_data = storage.owned_pixel_data.data();
        placed = uploadGlyphToAtlas(metrics, codepoint);
        if (placed == nullptr) {
            glyph_pool_.erase(codepoint);
            return nullptr;
        }
    }

    cache_->storeGlyph(std::move(storage));

    return placed;
}

void MSDFFont::loadGlyphs(std::span<const uint32_t> codepoints) {
    for (size_t first = 0; first < codepoints.size();) {
        const std::span<const uint32_t> run =
            codepoints.subspan(first, cache_->prefetchBlock(codepoints.subspan(first)));
        for (const uint32_t codepoint : run) {
            getGlyph(codepoint);
        }
        first += run.size();
    }
}

MSDFFont* MSDFFont::get(FT_Face face) {
    const auto it = kFontHandles->find(face);
    if (it != kFontHandles->end() && it->second->isValid()) { return it->second.get(); }
    return nullptr;
}

void MSDFFont::registerFace(FT_Face face, const FT_Byte* data, FT_Long size) {
    if (kFontHandles->contains(face)) { return; }
    auto font = std::make_unique<MSDFFont>(face, data, size);
    if (font->msdf_font_ != nullptr) { kFontHandles[face] = std::move(font); }
}

void MSDFFont::unregisterFace(FT_Face face) { kFontHandles->erase(face); }

void MSDFFont::clearAllCache() {
    for (auto& handle : *kFontHandles | std::views::values) {
        if (handle) {
            handle->glyph_pool_.clear();
            handle->atlas_pages_.clear();
            handle->oldest_page_ = 0;
            handle->eviction_count_++;
        }
    }
}

void MSDFFont::shutdown() { kFontHandles->clear(); }

bool MSDFFont::createAtlasPage() {
    auto page = std::make_unique<AtlasPage>(this, msdf::kAtlasGutter);
    GxTexFlags flags{};
    flags.unpacked.filter_mode = eGxTexFilterLinear;
    flags.unpacked.is_atlas_page = 1;  // pixels() points at the dirty rect, not the image base
    page->texture = CGxTexExt::create(
        {
            .width = msdf::kAtlasSize,
            .height = msdf::kAtlasSize,
            .format = eGxTexArgb8888,
            .data_format = eGxTexArgb8888,
            .flags = flags
        },
        page.get());
    if (page->texture == nullptr) { return false; }
    page->texture->markDirty();
    atlas_pages_.push_back(std::move(page));
    return true;
}

void MSDFFont::evictPage(AtlasPage& page) {
    for (const uint32_t cp : page.codepoints) {
        glyph_pool_.erase(cp);
    }
    page.clear();
    eviction_count_++;
}

GlyphMetrics* MSDFFont::uploadGlyphToAtlas(GlyphMetrics& metrics, uint32_t codepoint) {
    if (metrics.pixel_data == nullptr || metrics.width == 0 || metrics.height == 0) { return &metrics; }

    GlyphMetrics* placed = &metrics;
    int16_t page_index = -1;
    AtlasPage* target_page = nullptr;

    for (size_t i = 0; i < atlas_pages_.size(); ++i) {
        AtlasPage* page = atlas_pages_[i].get();
        if (page->next_x + metrics.width + msdf::kAtlasGutter <= msdf::kAtlasSize &&
            page->next_y + metrics.height + msdf::kAtlasGutter <= msdf::kAtlasSize) {
            page_index = static_cast<int16_t>(i);
            target_page = page;
            break;
        }
        const int next_y = static_cast<int>(page->next_y + page->row_height + msdf::kAtlasGutter);
        if (next_y + metrics.height + msdf::kAtlasGutter <= msdf::kAtlasSize) {
            page->next_x = msdf::kAtlasGutter;
            page->next_y = next_y;
            page->row_height = 0;
            page_index = static_cast<int16_t>(i);
            target_page = page;
            break;
        }
    }
    if (page_index == -1) {
        if (atlas_pages_.size() >= msdf::kMaxAtlasPages) {
            page_index = static_cast<int16_t>(oldest_page_);
            target_page = atlas_pages_[oldest_page_].get();

            evictPage(*target_page);
            placed = &glyph_pool_.find(codepoint)->second;
            target_page->texture->markDirty();
            oldest_page_ = (oldest_page_ + 1) % msdf::kMaxAtlasPages;
        } else {
            if (!createAtlasPage()) { return nullptr; }
            page_index = static_cast<int16_t>(atlas_pages_.size() - 1);
            target_page = atlas_pages_.back().get();
        }
    }
    if (target_page->texture == nullptr) { return nullptr; }

    GlyphMetrics& glyph = *placed;
    if (target_page->texture->needs_create_ != 0) { target_page->texture->markDirty(); }
    target_page->upload = glyph.pixel_data;
    target_page->upload_pitch = static_cast<uint32_t>(glyph.width) * 4;
    target_page->upload_rect = {
        .x0 = target_page->next_x,
        .y0 = target_page->next_y,
        .x1 = target_page->next_x + static_cast<int32_t>(glyph.width),
        .y1 = target_page->next_y + static_cast<int32_t>(glyph.height)
    };
    target_page->texture->markDirty({target_page->upload_rect});
    target_page->upload = nullptr;

    constexpr auto kAtlasSize = static_cast<float>(msdf::kAtlasSize);
    glyph.u0 = static_cast<float>(target_page->next_x) / kAtlasSize;
    glyph.v0 = static_cast<float>(target_page->next_y) / kAtlasSize;
    glyph.u1 = static_cast<float>(target_page->next_x + glyph.width) / kAtlasSize;
    glyph.v1 = static_cast<float>(target_page->next_y + glyph.height) / kAtlasSize;
    glyph.atlas_page_index = page_index;

    target_page->next_x += static_cast<int>(glyph.width + msdf::kAtlasGutter);
    target_page->row_height = std::max(target_page->row_height, static_cast<int>(glyph.height));
    target_page->codepoints.push_back(codepoint);

    return placed;
}

void MSDFFont::generateMsdf(std::vector<uint8_t>& out_data, const msdfgen::Shape& shape,
    const msdfgen::Projection& projection, double scale, int sdf_w, int sdf_h) {
    const auto pixel_count = static_cast<size_t>(sdf_w) * sdf_h;
    auto msdf_buf = msdf_pool_.acquireSized(pixel_count * 3);
    auto sdf_buf = msdf_pool_.acquireSized(pixel_count);

    msdfgen::BitmapRef<float, 3> msdf_bitmap(msdf_buf.data(), sdf_w, sdf_h);
    msdfgen::BitmapRef sdf_bitmap(sdf_buf.data(), sdf_w, sdf_h);

    msdfgen::MSDFGeneratorConfig config;
    config.overlapSupport = true;

    const msdfgen::Range msdf_range(msdf::kSdfSpread / scale);
    msdfgen::generateMSDF(msdf_bitmap, shape, projection, msdf_range, config);
    const msdfgen::SDFTransformation msdf_transform(projection, msdf_range);
    msdfgen::distanceSignCorrection(msdf_bitmap, shape, msdf_transform, msdfgen::FillRule::FILL_NONZERO);

    const msdfgen::Range sdf_range(msdf::kSdfSpread / scale * 5.0);
    msdfgen::generateSDF(sdf_bitmap, shape, projection, sdf_range);
    const msdfgen::SDFTransformation sdf_transform(projection, sdf_range);
    msdfgen::distanceSignCorrection(sdf_bitmap, shape, sdf_transform, msdfgen::FillRule::FILL_NONZERO);

    out_data.resize(pixel_count * 4);
    const std::span dest(out_data);
    const std::span<const float> src_msdf(msdf_buf);
    const std::span<const float> src_sdf(sdf_buf);

    const auto to_byte = [](float v) { return static_cast<uint8_t>(std::clamp(v * 255.f, 0.f, 255.f)); };
    for (size_t i = 0; i < pixel_count; ++i) {
        dest[i * 4 + 0] = to_byte(src_msdf[i * 3 + 0]);
        dest[i * 4 + 1] = to_byte(src_msdf[i * 3 + 1]);
        dest[i * 4 + 2] = to_byte(src_msdf[i * 3 + 2]);
        dest[i * 4 + 3] = to_byte(src_sdf[i]);
    }
    msdf_pool_.release(std::move(msdf_buf));
    msdf_pool_.release(std::move(sdf_buf));
}

bool MSDFFont::generateGlyphData(uint32_t codepoint, GlyphMetricsToStore& out_storage) const {
    out_storage.codepoint = codepoint;
    out_storage.width = 0;
    out_storage.height = 0;
    out_storage.bitmap_left = 0;
    out_storage.bitmap_top = 0;
    out_storage.owned_pixel_data.clear();
    out_storage.data_size = 0;

    msdfgen::GlyphIndex index;
    msdfgen::Shape shape;
    if (em_size_ <= 0.0 || !msdfgen::getGlyphIndex(index, msdf_font_, codepoint) ||
        !msdfgen::loadGlyph(shape, msdf_font_, index)) {
        return false;
    }
    if (shape.contours.empty()) { return true; }

    msdfgen::resolveShapeGeometry(shape);
    msdfgen::edgeColoringInkTrap(shape, 3.0, 0);

    const double scale = msdf::kSdfRenderSize / em_size_;
    const auto [l, b, r, t] = shape.getBounds();
    if (r <= l || t <= b) { return true; }

    const auto x_min = static_cast<int>(std::floor(l * scale));
    const auto y_min = static_cast<int>(std::floor(b * scale));
    const auto x_max = static_cast<int>(std::ceil(r * scale));
    const auto y_max = static_cast<int>(std::ceil(t * scale));
    out_storage.bitmap_left = x_min;
    out_storage.bitmap_top = y_max;

    constexpr auto kSpread = static_cast<int>(msdf::kSdfSpread);
    const int sdf_w = x_max - x_min + 2 * kSpread;
    const int sdf_h = y_max - y_min + 2 * kSpread;
    if (sdf_w > kMaxSdfDim || sdf_h > kMaxSdfDim) { return true; }

    // pixel = scale * (coord + translate) = scale * coord + spread - min
    const msdfgen::Projection projection(
        msdfgen::Vector2(scale, scale), msdfgen::Vector2((kSpread - x_min) / scale, (kSpread - y_min) / scale));

    generateMsdf(out_storage.owned_pixel_data, shape, projection, scale, sdf_w, sdf_h);
    out_storage.width = static_cast<uint16_t>(sdf_w);
    out_storage.height = static_cast<uint16_t>(sdf_h);
    out_storage.data_size = out_storage.owned_pixel_data.size();
    return true;
}

msdfgen::FontHandle* MSDFFont::createMsdfHandle(const FT_Byte* data, FT_Long size) {
    return msdf::msdf_freetype == nullptr ? nullptr : msdfgen::loadFontData(msdf::msdf_freetype, data, size);
}

MSDFCache::FaceRecord MSDFFont::evaluateFace(msdfgen::FontHandle* font) {
    return {
        .verdict = msdf_validator::isFontMsdfCompatible(font) ? MSDFCache::FaceVerdict::eCompatible
                                                              : MSDFCache::FaceVerdict::eIncompatible,
        .metrics = msdf_validator::measureFace(font)
    };
}
