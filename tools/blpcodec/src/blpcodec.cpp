#include "blpcodec.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <ranges>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "wu_quantizer.h"

#define STB_DXT_STATIC
#define STB_DXT_IMPLEMENTATION
#include "stb/stb_dxt.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define BCDEC_IMPLEMENTATION
#include "bcdec/bcdec.h"

#define BLPCODEC_ERROR_LEVEL _DEBUG

namespace {

struct PixelRgba {
    uint8_t r, g, b, a;
};

struct alignas(8) BcBlock16 {
    std::array<uint8_t, 8> alpha;
    std::array<uint8_t, 8> color;
};

using Bc3Block = BcBlock16;
using Bc5Block = BcBlock16;
using Bc1Block = std::array<uint8_t, 8>;

constexpr std::array<uint8_t, 4> kBLP2Magic = {'B', 'L', 'P', '2'};

auto mipRangeValid(uint32_t offset, uint64_t size, uint64_t file_size) {
    return static_cast<uint64_t>(offset) <= file_size && (file_size - static_cast<uint64_t>(offset)) >= size;
}

#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 1
auto isPowerOfTwo(uint32_t v) { return v != 0 && (v & (v - 1)) == 0; }
#endif

void classifyImage(const std::vector<uint8_t>& rgba_data, const std::vector<uint8_t>& classify_table,
    std::vector<uint8_t>& out_indices) {
    size_t pixel_count = rgba_data.size() / 4;
    out_indices.resize(pixel_count);

    for (auto [i, pixel] : rgba_data | std::views::chunk(4) | std::views::enumerate) {
        out_indices[i] = WuQuantizer::classify(classify_table, pixel[0], pixel[1], pixel[2]);
    }
}

void writeBytes(std::span<const uint8_t> data, std::vector<uint8_t>& out) {
    out.insert(out.end(), data.begin(), data.end());
}

template <typename Func>
void parallelFor(size_t count, const Func& func) {
    if (count == 0) { return; }
    if (count <= 64) {
        for (size_t i = 0; i < count; ++i) {
            func(i);
        }
        return;
    }

    unsigned int hw_threads = std::thread::hardware_concurrency();
    unsigned int num_threads =
        std::max(1u, std::min(hw_threads == 0 ? 4u : hw_threads, static_cast<unsigned int>(count)));

    if (num_threads <= 1) {
        for (size_t i = 0; i < count; ++i) {
            func(i);
        }
        return;
    }

    size_t chunk = (count + num_threads - 1) / num_threads;
    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    for (unsigned int t = 0; t < num_threads; ++t) {
        size_t begin = t * chunk;
        if (begin >= count) { break; }
        size_t end = std::min(count, begin + chunk);
        workers.emplace_back([begin, end, &func]() {
            for (size_t i = begin; i < end; ++i) {
                func(i);
            }
        });
    }
    for (auto& th : workers) {
        th.join();
    }
}

void primeDxtCompressor() {
    static std::once_flag once_flag;
    std::call_once(once_flag, []() {
        alignas(16) std::array<uint8_t, 64> dummy_src{};
        std::array<uint8_t, 8> dummy_dst{};
        stb_compress_dxt_block(dummy_dst.data(), dummy_src.data(), 0, STB_DXT_HIGHQUAL);
    });
}
}  // namespace

bool blpcodec::BLPCodec::decode(const std::string& path, BLPImage& out_image) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Decode] Error: Failed to open file '" << path << "'.\n";
#endif
        return false;
    }

    auto stream_size = file.tellg();
    if (stream_size < 0) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Decode] Error: Invalid stream size for '" << path << "'.\n";
#endif
        return false;
    }

    size_t file_size = stream_size;
    file.seekg(0, std::ios::beg);

    if (file_size < 4) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Decode] Error: File size too small (" << file_size << " bytes) for '" << path << ".\n";
#endif
        return false;
    }

    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();

    const uint8_t* fptr = file_data.data();

    if (std::memcmp(fptr, kBLP2Magic.data(), 4) != 0) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Decode] Error: Invalid BLP2 magic number in '" << path << ".\n";
#endif
        return false;  // BLP2
    }

    BLP2Header h2{};
    uint32_t base_header_size = sizeof(BLP2Header) - sizeof(h2.palette);
    if (file_size < base_header_size) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Decode] Error: File size (" << file_size << ") smaller than base header size ("
                  << base_header_size << ") in '" << path << ".\n";
#endif
        return false;
    }
    std::memcpy(&h2, fptr, base_header_size);

    uint32_t current_offset = base_header_size;
    uint32_t jpeg_header_size = 0;
    std::vector<uint8_t> jpeg_header_data;

    if (h2.color_encoding == eColorPalette) {
        // paletted
        if (file_size < current_offset + sizeof(h2.palette)) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
            std::cerr << "[BLPCodec::Decode] Error: File truncated before "
                         "palette data in '"
                      << path << ".\n";
#endif
            return false;
        }
        auto palette = std::span(file_data).subspan(current_offset, sizeof(h2.palette));
        std::memcpy(std::data(h2.palette), palette.data(), palette.size());
        current_offset += sizeof(h2.palette);
    } else if (h2.color_encoding == eColorJpeg) {
        if (file_size < current_offset + 4) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
            std::cerr << "[BLPCodec::Decode] Error: File truncated before JPEG "
                         "header size in '"
                      << path << ".\n";
#endif
            return false;
        }
        auto header_size_bytes = std::span(file_data).subspan(current_offset, sizeof(uint32_t));
        auto dst_bytes =
            std::span<uint8_t, sizeof(uint32_t)>(reinterpret_cast<uint8_t*>(&jpeg_header_size), sizeof(uint32_t));

        std::ranges::copy(header_size_bytes, dst_bytes.begin());
        current_offset += 4;

        jpeg_header_size = std::min(jpeg_header_size, static_cast<uint32_t>(1020));
        if (file_size < current_offset + jpeg_header_size) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
            std::cerr << "[BLPCodec::Decode] Error: File truncated before JPEG "
                         "header data in '"
                      << path << ".\n";
#endif
            return false;
        }

        if (jpeg_header_size > 0) {
            auto header_span = std::span(file_data).subspan(current_offset, jpeg_header_size);
            jpeg_header_data.assign(header_span.begin(), header_span.end());
            current_offset += jpeg_header_size;
        }
    }

    if (h2.width == 0 || h2.width > 8192 || h2.height == 0 || h2.height > 8192) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Decode] Error: Invalid dimensions (" << h2.width << "x" << h2.height << ") in '"
                  << path << ".\n";
#endif
        return false;
    }

    out_image.width = h2.width;
    out_image.height = h2.height;
    out_image.color_encoding = h2.color_encoding;
    out_image.alpha_depth = h2.alpha_depth;
    out_image.format = h2.format;
    out_image.mip_flags = h2.mip_flags;
    out_image.mips.clear();

    if (out_image.color_encoding == eColorPalette) {
        // paletted
        for (int m = 0; m < 16; ++m) {
            if (h2.mip_offsets[m] == 0 || h2.mip_sizes[m] == 0) { break; }

            uint32_t mip_w = std::max(1u, out_image.width >> m);
            uint32_t mip_h = std::max(1u, out_image.height >> m);
            uint64_t total_pixels = static_cast<uint64_t>(mip_w) * mip_h;
            uint64_t alpha_size = (out_image.alpha_depth > 0) ? ((total_pixels * out_image.alpha_depth + 7) / 8) : 0;

            if (h2.mip_sizes[m] < total_pixels + alpha_size) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
                std::cerr << "[BLPCodec::Decode] Error: Mip " << m << " size (" << h2.mip_sizes[m]
                          << ") smaller than required (" << total_pixels + alpha_size << ") in '" << path << ".\n";
#endif
                if (m == 0) { return false; }
                break;
            }
            if (!mipRangeValid(h2.mip_offsets[m], total_pixels + alpha_size, file_size)) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
                std::cerr << "[BLPCodec::Decode] Warning: Invalid mip " << m << " range in '" << path
                          << "'. Stopping mip generation.\n";
#endif
                if (m == 0) { return false; }
                break;
            }

            const size_t offset = h2.mip_offsets[m];
            const size_t required_bytes = static_cast<size_t>(total_pixels) + alpha_size;

            auto raw_mip_span = std::span(file_data).subspan(offset, required_bytes);
            auto indices = raw_mip_span.subspan(0, total_pixels);
            auto alpha_data =
                (alpha_size > 0) ? raw_mip_span.subspan(total_pixels, alpha_size) : std::span<const uint8_t>{};

            MipLevel level;
            level.width = mip_w;
            level.height = mip_h;
            level.rgba.resize(static_cast<size_t>(total_pixels) * 4, 0);

            auto dst_pixels = std::span(reinterpret_cast<uint32_t*>(level.rgba.data()), total_pixels);

            for (size_t i = 0; i < total_pixels; ++i) {
                const uint32_t color = h2.palette[indices[i]];
                const uint32_t b = (color >> 0) & 0xFF;
                const uint32_t g = (color >> 8) & 0xFF;
                const uint32_t r = (color >> 16) & 0xFF;
                uint32_t a = 255;

                if (out_image.alpha_depth == 1) {
                    a = ((alpha_data[i / 8] & (1u << (i % 8))) != 0) ? 255 : 0;
                } else if (out_image.alpha_depth == 4) {
                    const uint8_t shift = (i % 2 == 0) ? 0 : 4;
                    a = static_cast<uint32_t>(((alpha_data[i / 2] >> shift) & 0x0F) * 17);
                } else if (out_image.alpha_depth == 8) {
                    a = alpha_data[i];
                }

                dst_pixels[i] = r | (g << 8) | (b << 16) | (a << 24);
            }
            out_image.mips.push_back(std::move(level));
        }
        return true;
    }

    if (out_image.color_encoding == eColorDxt) {
        // DXT
        if (out_image.format == ePixelUnspecified) { out_image.format = ePixelDxt5; }
        if (out_image.format != ePixelDxt1 && out_image.format != ePixelDxt3 && out_image.format != ePixelDxt5) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
            std::cerr << "[BLPCodec::Decode] Error: Unsupported DXT pixel "
                         "format value "
                      << static_cast<unsigned int>(out_image.format) << " in '" << path << ".\n";
#endif
            return false;
        }

        uint32_t block_size = (out_image.format == ePixelDxt1) ? 8 : 16;

        for (int m = 0; m < 16; ++m) {
            if (h2.mip_offsets[m] == 0 || h2.mip_sizes[m] == 0) { break; }
            uint32_t mip_w = std::max(1u, out_image.width >> m);
            uint32_t mip_h = std::max(1u, out_image.height >> m);

            uint32_t blocks_w = (mip_w + 3) / 4;
            uint32_t blocks_h = (mip_h + 3) / 4;
            uint32_t required_size = blocks_w * blocks_h * block_size;

#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 1
            if (h2.mip_sizes[m] < required_size) {
                std::cerr << "[BLPCodec::Decode] Warning: mip " << m << " declared size (" << h2.mip_sizes[m]
                          << ") is smaller than required block-aligned size (" << required_size << ") in '" << path
                          << "'; proceeding with calculated size.\n";
            }
#endif
            if (!mipRangeValid(h2.mip_offsets[m], required_size, file_size)) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
                std::cerr << "[BLPCodec::Decode] Warning: Invalid DXT mip " << m << " range in '" << path
                          << "'. Stopping mip generation.\n";
#endif
                if (m == 0) { return false; }
                break;
            }

            MipLevel level;
            level.width = mip_w;
            level.height = mip_h;
            level.rgba.resize(mip_w * mip_h * 4, 0);

            auto input_blocks = std::span(file_data).subspan(h2.mip_offsets[m], blocks_w * blocks_h * block_size);
            auto dst_pixels = std::span(reinterpret_cast<uint32_t*>(level.rgba.data()), mip_w * mip_h);

            for (uint32_t y = 0; y < blocks_h; ++y) {
                for (uint32_t x = 0; x < blocks_w; ++x) {
                    const uint32_t bw = std::min(4u, mip_w - x * 4);
                    const uint32_t bh = std::min(4u, mip_h - y * 4);

                    const size_t block_idx = y * blocks_w + x;
                    auto cur_block = input_blocks.subspan(block_idx * block_size, block_size);

                    std::array<uint32_t, 16> temp_block{};
                    auto* temp_block_bytes = reinterpret_cast<uint8_t*>(temp_block.data());

                    if (out_image.format == ePixelDxt1) {
                        bcdec_bc1(cur_block.data(), temp_block_bytes, 4 * 4);
                    } else if (out_image.format == ePixelDxt3) {
                        bcdec_bc2(cur_block.data(), temp_block_bytes, 4 * 4);
                    } else if (out_image.format == ePixelDxt5) {
                        bcdec_bc3(cur_block.data(), temp_block_bytes, 4 * 4);
                    }

                    for (uint32_t row = 0; row < bh; ++row) {
                        const size_t dst_offset = (y * 4 + row) * mip_w + (x * 4);
                        const size_t src_offset = row * 4;

                        auto src_row = std::span(temp_block).subspan(src_offset, bw);
                        auto dst_row = dst_pixels.subspan(dst_offset, bw);

                        std::ranges::copy(src_row, dst_row.begin());
                    }
                }
            }
            out_image.mips.push_back(std::move(level));
        }
        return true;
    }

    if (out_image.color_encoding == eColorArgb8888 || out_image.color_encoding == eColorArgb8888Dup) {
        // ARGB8888
        for (int m = 0; m < 16; ++m) {
            if (h2.mip_offsets[m] == 0 || h2.mip_sizes[m] == 0) { break; }

            uint32_t mip_w = std::max(1u, out_image.width >> m);
            uint32_t mip_h = std::max(1u, out_image.height >> m);
            uint64_t total_pixels = static_cast<uint64_t>(mip_w) * mip_h;

            if (!mipRangeValid(h2.mip_offsets[m], total_pixels * 4, file_size)) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
                std::cerr << "[BLPCodec::Decode] Warning: Invalid ARGB8888 mip " << m << " range in '" << path
                          << "'. Stopping mip generation.\n";
#endif
                if (m == 0) { return false; }
                break;
            }

            MipLevel level;
            level.width = mip_w;
            level.height = mip_h;
            level.rgba.resize(static_cast<size_t>(total_pixels * 4));

            auto src_bytes = std::span(file_data).subspan(h2.mip_offsets[m], total_pixels * 4);
            std::ranges::copy(src_bytes, level.rgba.begin());

            for (auto pixel : level.rgba | std::views::chunk(4)) {
                std::swap(pixel[0], pixel[2]);
            }
            out_image.mips.push_back(std::move(level));
        }
        return true;
    }

    if (out_image.color_encoding == eColorJpeg) {
        for (int m = 0; m < 16; ++m) {
            if (h2.mip_offsets[m] == 0 || h2.mip_sizes[m] == 0) { break; }
            uint32_t mip_w = std::max(1u, out_image.width >> m);
            uint32_t mip_h = std::max(1u, out_image.height >> m);

            if (!mipRangeValid(h2.mip_offsets[m], h2.mip_sizes[m], file_size)) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
                std::cerr << "[BLPCodec::Decode] Warning: Invalid JPEG mip " << m << " range in '" << path
                          << "'. Stopping mip generation.\n";
#endif
                if (m == 0) { return false; }
                break;
            }

            auto raw_mip_span = std::span(file_data).subspan(h2.mip_offsets[m], h2.mip_sizes[m]);

            std::vector<uint8_t> full_jpeg;
            full_jpeg.reserve(jpeg_header_data.size() + raw_mip_span.size());
            full_jpeg.insert(full_jpeg.end(), jpeg_header_data.begin(), jpeg_header_data.end());
            full_jpeg.insert(full_jpeg.end(), raw_mip_span.begin(), raw_mip_span.end());

            int j_w = 0, j_h = 0, j_c = 0;
            unsigned char* jpeg_decoded =
                stbi_load_from_memory(full_jpeg.data(), std::ssize(full_jpeg), &j_w, &j_h, &j_c, 4);

            MipLevel level;
            level.width = mip_w;
            level.height = mip_h;
            level.rgba.resize(mip_w * mip_h * 4, 0);

            if (jpeg_decoded != nullptr) {
                const uint32_t copy_h = std::min(static_cast<uint32_t>(j_h), mip_h);
                const uint32_t copy_w = std::min(static_cast<uint32_t>(j_w), mip_w);

                auto src_span =
                    std::span(reinterpret_cast<const uint32_t*>(jpeg_decoded), static_cast<size_t>(j_w) * j_h);
                auto dst_span = std::span(reinterpret_cast<uint32_t*>(level.rgba.data()), mip_w * mip_h);

                for (uint32_t y = 0; y < copy_h; ++y) {
                    auto src_row = src_span.subspan(y * j_w, copy_w);
                    auto dst_row = dst_span.subspan(y * mip_w, copy_w);
                    std::ranges::copy(src_row, dst_row.begin());
                }

                stbi_image_free(jpeg_decoded);

                if (out_image.alpha_depth > 0) {
                    const uint32_t expected_alpha_bytes = mip_w * mip_h;

                    if (raw_mip_span.size() >= expected_alpha_bytes) {
                        auto alpha_span =
                            raw_mip_span.subspan(raw_mip_span.size() - expected_alpha_bytes, expected_alpha_bytes);
                        auto dst_pixels =
                            std::span(reinterpret_cast<PixelRgba*>(level.rgba.data()), expected_alpha_bytes);

                        for (size_t i = 0; i < expected_alpha_bytes; ++i) {
                            dst_pixels[i].a = alpha_span[i];
                        }
                    }
                }
            }
            out_image.mips.push_back(std::move(level));
        }
        return true;
    }

#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
    std::cerr << "[BLPCodec::Decode] Error: Unsupported color encoding ("
              << static_cast<unsigned int>(out_image.color_encoding) << ") in '" << path << ".\n";
#endif
    return false;
}

bool blpcodec::BLPCodec::encode(const std::string& path, const BLPImage& image) {
    if (image.mips.empty() || image.mips[0].rgba.empty() || image.width == 0 || image.height == 0 ||
        image.width > 8192 || image.height > 8192) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Encode] Error: Invalid image dimensions (" << image.width << "x" << image.height
                  << ") or missing mip data for '" << path << ".\n";
#endif
        return false;
    }

    if (image.color_encoding != eColorPalette && image.color_encoding != eColorDxt &&
        image.color_encoding != eColorArgb8888) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Encode] Error: Unsupported color encoding ("
                  << static_cast<unsigned int>(image.color_encoding) << ") for '" << path << "'.\n";
#endif
        return false;
    }

    if (image.alpha_depth != 0 && image.alpha_depth != 1 && image.alpha_depth != 4 && image.alpha_depth != 8) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Encode] Error: Unsupported alpha depth (" << static_cast<int>(image.alpha_depth)
                  << ") for '" << path << ".\n";
#endif
        return false;
    }
    if (image.color_encoding == eColorDxt && image.format != ePixelDxt1 && image.format != ePixelDxt3 &&
        image.format != ePixelDxt5) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Encode] Error: Unsupported DXT format (" << static_cast<unsigned int>(image.format)
                  << ") for '" << path << ".\n";
#endif
        return false;
    }
    if (static_cast<uint64_t>(image.mips[0].rgba.size()) != static_cast<uint64_t>(image.width) * image.height * 4) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Encode] Error: Mip 0 data size mismatch for '" << path << ".\n";
#endif
        return false;
    }

    for (size_t i = 1; i < image.mips.size(); ++i) {
        const auto& [width, height, rgba] = image.mips[i];
        if (width == 0 || height == 0) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
            std::cerr << "[BLPCodec::Encode] Error: Invalid dimensions for mip " << i << " in '" << path << ".\n";
#endif
            return false;
        }
        if (static_cast<uint64_t>(rgba.size()) != static_cast<uint64_t>(width) * height * 4) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
            std::cerr << "[BLPCodec::Encode] Error: Data size mismatch for mip " << i << " in '" << path << ".\n";
#endif
            return false;
        }
    }

    if (image.mips.size() > 1) {
        uint32_t exp_w = image.width;
        uint32_t exp_h = image.height;
        for (size_t i = 1; i < image.mips.size(); ++i) {
            exp_w = std::max(1u, exp_w / 2);
            exp_h = std::max(1u, exp_h / 2);
            if (image.mips[i].width != exp_w || image.mips[i].height != exp_h) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
                std::cerr << "[BLPCodec::Encode] Error: Pre-decoded mip " << i
                          << " dimensions do not match expected halved sizes.\n";
#endif
                return false;
            }
        }
    }

#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 1
    if (!isPowerOfTwo(image.width) || !isPowerOfTwo(image.height)) {
        std::cerr << "[BLPCodec::Encode] Warning: " << image.width << "x" << image.height
                  << " is not power-of-two; BLP2 requires POT dimensions and the "
                     "game will likely reject this file.\n";
    }
#endif

    BLP2Header header{};
    std::memcpy(&header.magic, kBLP2Magic.data(), sizeof(header.magic));
    header.version = 1;
    header.color_encoding = image.color_encoding;
    header.alpha_depth = image.alpha_depth;
    header.format = image.format;
    header.mip_flags = (image.mip_flags == eMipsGenerated || image.mip_flags == eMipsHandmade || image.mips.size() > 1)
        ? eMipsGenerated
        : eMipsNone;
    header.width = image.width;
    header.height = image.height;

    uint32_t block_size;
    if (header.color_encoding == eColorArgb8888) {
        block_size = 4;
    } else {
        block_size = (header.format == ePixelDxt1) ? 8 : 16;
    }
    uint32_t header_size = sizeof(BLP2Header);
    int palette_color_count = 256;
    std::vector<uint8_t> base_mip_indices;
    std::vector<uint8_t> classify_table;

    if (header.color_encoding == eColorPalette) {
        WuQuantizer::quantize(
            image.mips[0].rgba, header.palette, palette_color_count, base_mip_indices, 256, &classify_table);
    }

    uint32_t current_offset = header_size;
    uint32_t current_w = image.width;
    uint32_t current_h = image.height;

    int max_mips = (header.mip_flags == eMipsHandmade || header.mip_flags == eMipsGenerated) ? 16 : 1;
    int num_mips = 0;
    while (num_mips < max_mips) {
        uint32_t size;
        if (header.color_encoding == eColorArgb8888) {
            size = current_w * current_h * 4;
        } else if (header.color_encoding == eColorPalette) {
            uint32_t pixels = current_w * current_h;
            uint32_t alpha_size = (header.alpha_depth > 0) ? ((pixels * header.alpha_depth + 7) / 8) : 0;
            size = pixels + alpha_size;
        } else {
            uint32_t blocks_w = (current_w + 3) / 4;
            uint32_t blocks_h = (current_h + 3) / 4;
            size = blocks_w * blocks_h * block_size;
        }

        header.mip_offsets[num_mips] = current_offset;
        header.mip_sizes[num_mips] = size;
        current_offset += size;
        num_mips++;

        if (current_w == 1 && current_h == 1) { break; }
        if (current_w > 1) { current_w /= 2; }
        if (current_h > 1) { current_h /= 2; }
    }

    std::vector<uint8_t> out_buffer;
    out_buffer.reserve(current_offset);

    writeBytes(
        std::span<const uint8_t, sizeof(BLP2Header)>(reinterpret_cast<const uint8_t*>(&header), sizeof(BLP2Header)),
        out_buffer);

    current_w = image.width;
    current_h = image.height;
    std::vector<uint8_t> current_rgba = image.mips[0].rgba;
    const bool has_predecoded_mips = (image.mips.size() > 1);

    for (int mip = 0; mip < num_mips; mip++) {
        if (mip > 0 && has_predecoded_mips && mip < std::ssize(image.mips)) {
            current_rgba = image.mips[mip].rgba;
            current_w = image.mips[mip].width;
            current_h = image.mips[mip].height;
        }

        if (header.color_encoding == eColorArgb8888) {
            std::vector<uint8_t> bgra_pixels = current_rgba;

            for (auto pixel : bgra_pixels | std::views::chunk(4)) {
                std::swap(pixel[0], pixel[2]);
            }

            writeBytes(bgra_pixels, out_buffer);
        } else if (header.color_encoding == eColorPalette) {
            if (mip == 0) {
                writeBytes(base_mip_indices, out_buffer);
            } else {
                std::vector<uint8_t> indices;
                classifyImage(current_rgba, classify_table, indices);
                writeBytes(indices, out_buffer);
            }

            if (header.alpha_depth > 0) {
                const uint32_t pixels = current_w * current_h;
                const uint32_t alpha_size = (pixels * header.alpha_depth + 7) / 8;
                std::vector<uint8_t> alpha_data(alpha_size, 0);

                auto rgba_pixels = std::span(reinterpret_cast<const PixelRgba*>(current_rgba.data()), pixels);

                if (header.alpha_depth == 8) {
                    for (size_t i = 0; i < pixels; ++i) {
                        alpha_data[i] = rgba_pixels[i].a;
                    }
                } else if (header.alpha_depth == 1) {
                    for (size_t i = 0; i < pixels; ++i) {
                        if (rgba_pixels[i].a > 127) { alpha_data[i / 8] |= static_cast<uint8_t>(1u << (i % 8)); }
                    }
                } else if (header.alpha_depth == 4) {
                    for (size_t i = 0; i < pixels; ++i) {
                        const auto a4 = static_cast<uint8_t>(rgba_pixels[i].a >> 4);
                        const uint8_t shift = (i % 2 == 0) ? 0 : 4;
                        alpha_data[i / 2] |= static_cast<uint8_t>((a4 & 0x0F) << shift);
                    }
                }

                writeBytes(alpha_data, out_buffer);
            }
        } else if (header.color_encoding == eColorDxt) {
            uint32_t blocks_w = (current_w + 3) / 4;
            uint32_t blocks_h = (current_h + 3) / 4;
            uint32_t num_blocks = blocks_w * blocks_h;

            std::vector<uint8_t> mip_block_data(num_blocks * block_size);

            primeDxtCompressor();

            const uint8_t* rgba_ptr = current_rgba.data();
            const uint32_t w = current_w;
            const uint32_t h = current_h;
            const uint8_t fmt = header.format;
            const uint8_t alpha_depth = header.alpha_depth;
            const uint32_t stride = block_size;

            parallelFor(num_blocks, [&, w, h, fmt, alpha_depth, stride](size_t block_idx) {
                uint32_t x = block_idx % blocks_w;
                uint32_t y = block_idx / blocks_w;

                std::array<uint8_t, 64> block_pixels_local{};

                auto src_pixels = std::span(reinterpret_cast<const uint32_t*>(rgba_ptr), static_cast<size_t>(w) * h);
                auto dst_pixels = std::span<uint32_t, 16>(reinterpret_cast<uint32_t*>(block_pixels_local.data()), 16);

                for (uint32_t by : std::views::iota(0u, 4u)) {
                    const uint32_t py = std::min(y * 4 + by, h - 1);
                    const uint32_t src_row_offset = py * w;
                    const uint32_t dst_row_offset = by * 4;

                    for (uint32_t bx : std::views::iota(0u, 4u)) {
                        const uint32_t px = std::min(x * 4 + bx, w - 1);
                        dst_pixels[dst_row_offset + bx] = src_pixels[src_row_offset + px];
                    }
                }

                std::array<uint8_t, 8> color_block{};
                auto out_span = std::span(mip_block_data).subspan(block_idx * stride, stride);

                if (fmt == ePixelDxt3) {
                    // DXT3
                    auto& [alpha, color] = *reinterpret_cast<Bc3Block*>(out_span.data());
                    alpha.fill(0);

                    for (size_t i = 0; i < 16; ++i) {
                        const uint8_t a4 = block_pixels_local[i * 4 + 3] >> 4;
                        const size_t byte_idx = i / 2;
                        const uint8_t shift = (i % 2 == 0) ? 0 : 4;
                        alpha[byte_idx] |= static_cast<uint8_t>((a4 & 0x0F) << shift);
                    }

                    stb_compress_dxt_block(color.data(), block_pixels_local.data(), 0, STB_DXT_HIGHQUAL);
                } else if (fmt == ePixelDxt5) {
                    // DXT5
                    auto& [alpha, color] = *reinterpret_cast<Bc5Block*>(out_span.data());

                    uint8_t a_min = 255;
                    uint8_t a_max = 0;
                    for (size_t i = 0; i < 16; ++i) {
                        const uint8_t a = block_pixels_local[i * 4 + 3];
                        a_min = std::min(a_min, a);
                        a_max = std::max(a_max, a);
                    }

                    alpha[0] = a_max;
                    alpha[1] = a_min;

                    uint64_t alpha_indices = 0;
                    if (a_max > a_min) {
                        const int dist = a_max - a_min;
                        const int half_dist = dist / 2;

                        for (size_t i = 0; i < 16; ++i) {
                            const uint8_t a = block_pixels_local[i * 4 + 3];
                            uint64_t idx;

                            if (a == a_max) {
                                idx = 0;
                            } else if (a == a_min) {
                                idx = 1;
                            } else {
                                const int step = (a_max - a) * 7;
                                const int step_idx = (step + half_dist) / dist;

                                if (step_idx <= 0) {
                                    idx = 0;
                                } else if (step_idx >= 7) {
                                    idx = 1;
                                } else {
                                    idx = static_cast<uint64_t>(step_idx) + 1;
                                }
                            }
                            alpha_indices |= (idx << (3 * i));
                        }
                    }

                    for (size_t i = 0; i < 6; ++i) {
                        alpha[2 + i] = static_cast<uint8_t>((alpha_indices >> (i * 8)) & 0xFF);
                    }

                    stb_compress_dxt_block(color.data(), block_pixels_local.data(), 0, STB_DXT_HIGHQUAL);
                } else {
                    // DXT1
                    auto& block = *reinterpret_cast<Bc1Block*>(out_span.data());
                    std::array<uint8_t, 64> block_pixels_mutable = block_pixels_local;

                    if (fmt == ePixelDxt1 && alpha_depth > 0) {
                        uint32_t r_sum = 0, g_sum = 0, b_sum = 0, valid_count = 0;
                        for (size_t i = 0; i < 16; ++i) {
                            if (block_pixels_mutable[i * 4 + 3] >= 128) {
                                r_sum += block_pixels_mutable[i * 4 + 0];
                                g_sum += block_pixels_mutable[i * 4 + 1];
                                b_sum += block_pixels_mutable[i * 4 + 2];
                                valid_count++;
                            }
                        }

                        if (valid_count > 0 && valid_count < 16) {
                            const auto avg_r = static_cast<uint8_t>(r_sum / valid_count);
                            const auto avg_g = static_cast<uint8_t>(g_sum / valid_count);
                            const auto avg_b = static_cast<uint8_t>(b_sum / valid_count);
                            for (size_t i = 0; i < 16; ++i) {
                                if (block_pixels_mutable[i * 4 + 3] < 128) {
                                    block_pixels_mutable[i * 4 + 0] = avg_r;
                                    block_pixels_mutable[i * 4 + 1] = avg_g;
                                    block_pixels_mutable[i * 4 + 2] = avg_b;
                                }
                            }
                        }
                    }

                    stb_compress_dxt_block(color_block.data(), block_pixels_mutable.data(), 0, STB_DXT_HIGHQUAL);

                    if (fmt == ePixelDxt1 && alpha_depth > 0) {
                        const bool has_transparent = std::ranges::any_of(std::views::iota(size_t{0}, size_t{16}),
                            [&](size_t i) { return block_pixels_mutable[i * 4 + 3] < 128; });

                        if (has_transparent) {
                            const auto max16 = static_cast<uint16_t>(color_block[0] | (color_block[1] << 8));
                            const auto min16 = static_cast<uint16_t>(color_block[2] | (color_block[3] << 8));
                            const auto mask = static_cast<uint32_t>(color_block[4] | (color_block[5] << 8) |
                                (color_block[6] << 16) | (color_block[7] << 24));
                            uint32_t new_mask = 0;

                            if (max16 > min16) {
                                // swap min/max colors to signal 1-bit alpha mode to the GPU
                                color_block[0] = min16 & 0xFF;
                                color_block[1] = (min16 >> 8) & 0xFF;
                                color_block[2] = max16 & 0xFF;
                                color_block[3] = (max16 >> 8) & 0xFF;

                                for (size_t i = 0; i < 16; ++i) {
                                    const uint32_t m = (mask >> (i * 2)) & 3;
                                    uint32_t new_m;
                                    if (block_pixels_mutable[i * 4 + 3] < 128) {
                                        new_m = 3;
                                    } else if (m == 0) {
                                        new_m = 1;
                                    } else if (m == 1) {
                                        new_m = 0;
                                    } else {
                                        new_m = 2;
                                    }
                                    new_mask |= (new_m << (i * 2));
                                }
                            } else {
                                for (size_t i = 0; i < 16; ++i) {
                                    uint32_t m = (mask >> (i * 2)) & 3;
                                    if (block_pixels_mutable[i * 4 + 3] < 128) { m = 3; }
                                    new_mask |= (m << (i * 2));
                                }
                            }

                            for (size_t i = 0; i < 4; ++i) {
                                color_block[4 + i] = static_cast<uint8_t>((new_mask >> (i * 8)) & 0xFF);
                            }
                        }
                    }

                    std::ranges::copy(color_block, block.begin());
                }
            });

            writeBytes(mip_block_data, out_buffer);
        }

        if (mip < num_mips - 1 && (!has_predecoded_mips || mip + 1 >= std::ssize(image.mips))) {
            uint32_t next_w = std::max(1u, current_w / 2);
            uint32_t next_h = std::max(1u, current_h / 2);
            std::vector<uint8_t> next_rgba(next_w * next_h * 4);

            for (uint32_t y = 0; y < next_h; y++) {
                for (uint32_t x = 0; x < next_w; x++) {
                    uint32_t src_x = x * 2;
                    uint32_t src_y = y * 2;

                    uint32_t r = 0, g = 0, b = 0, a = 0;
                    int count = 0;
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            uint32_t px = src_x + dx;
                            uint32_t py = src_y + dy;
                            if (px >= current_w || py >= current_h) { continue; }
                            uint32_t idx = (py * current_w + px) * 4;
                            r += current_rgba[idx + 0];
                            g += current_rgba[idx + 1];
                            b += current_rgba[idx + 2];
                            a += current_rgba[idx + 3];
                            count++;
                        }
                    }
                    uint32_t dst_idx = (y * next_w + x) * 4;
                    if (count > 0) {
                        next_rgba[dst_idx + 0] = static_cast<uint8_t>(r / count);
                        next_rgba[dst_idx + 1] = static_cast<uint8_t>(g / count);
                        next_rgba[dst_idx + 2] = static_cast<uint8_t>(b / count);
                        next_rgba[dst_idx + 3] = static_cast<uint8_t>(a / count);
                    }
                }
            }
            current_rgba = std::move(next_rgba);
            current_w = next_w;
            current_h = next_h;
        }
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
#if defined(BLPCODEC_ERROR_LEVEL) && BLPCODEC_ERROR_LEVEL > 0
        std::cerr << "[BLPCodec::Encode] Failed to open file '" << path << "' for writing.\n";
#endif
        return false;
    }
    file.write(reinterpret_cast<const char*>(out_buffer.data()), out_buffer.size());

    return true;
}
