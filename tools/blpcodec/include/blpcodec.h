#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace blpcodec {
enum BLPColorEncoding : uint8_t {
    // not supported
    eColorJpeg = 0,
    eColorPalette = 1,
    eColorDxt = 2,
    eColorArgb8888 = 3,
    eColorArgb8888Dup = 4,
};

enum BLPPixelFormat : uint8_t {
    ePixelDxt1 = 0,
    ePixelDxt3 = 1,
    ePixelArgb8888 = 2,
    ePixelArgb1555 = 3,
    ePixelArgb4444 = 4,
    ePixelRgb565 = 5,
    ePixelA8 = 6,
    ePixelDxt5 = 7,
    ePixelUnspecified = 8,
    ePixelArgb2565 = 9,
    ePixelBc5 = 11,
    eNumPixelFormats = 12,
};

enum BLPMipFlags : uint8_t {
    eMipsNone = 0x0,
    eMipsGenerated = 0x1,
    // not supported
    eMipsHandmade = 0x2,
    // level
    eFlagsMipmapMask = 0xF,
    eFlagsUnk0X10 = 0x10,
};

#pragma pack(push, 1)

struct BLP2Header {
    uint32_t magic;                   // "BLP2"
    uint32_t version;                 // must be 1
    BLPColorEncoding color_encoding;  // 0: JPEG, 1: paletted, 2: DXT, 3: ARGB8888
    uint8_t alpha_depth;              // [0, 1, 4, 8]
    BLPPixelFormat format;            // color_encoding = 2: [0: DXT1, 1: DXT3, 7: DXT5]; color_encoding = 1: [2, 4, 8]
    BLPMipFlags mip_flags;            // 0: no mips, >=1: has mips
    uint32_t width;
    uint32_t height;
    uint32_t mip_offsets[16];
    uint32_t mip_sizes[16];
    uint32_t palette[256];
};

#pragma pack(pop)

struct MipLevel {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> rgba;
};

struct BLPImage {
    BLPColorEncoding color_encoding = eColorJpeg;
    uint8_t alpha_depth = 0xFF;
    BLPPixelFormat format = ePixelUnspecified;
    BLPMipFlags mip_flags = eMipsNone;
    uint32_t width = 0;
    uint32_t height = 0;

    std::vector<MipLevel> mips;
};

class BLPCodec {
public:
    static bool decode(const std::string& path, BLPImage& out_image);
    static bool encode(const std::string& path, const BLPImage& image);
};
}  // namespace blpcodec
