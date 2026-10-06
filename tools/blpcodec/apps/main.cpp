#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <utility>

#include "blpcodec.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using blpcodec::BLPCodec;
using blpcodec::BLPImage;
using blpcodec::eColorArgb8888;
using blpcodec::eColorDxt;
using blpcodec::eColorPalette;
using blpcodec::eMipsGenerated;
using blpcodec::ePixelArgb8888;
using blpcodec::ePixelDxt1;
using blpcodec::ePixelDxt3;
using blpcodec::ePixelDxt5;
using blpcodec::ePixelUnspecified;
using blpcodec::MipLevel;

namespace {

int run(int argc, char** argv) {
    const std::span<char* const> args(argv, static_cast<size_t>(argc));

    if (argc < 4) {
        std::cerr << "Usage:\n";
        std::cerr << "  " << args[0] << " decode <input.blp> <output.(png|tga|bmp|jpg)>\n";
        std::cerr
            << "  " << args[0]
            << " encode <input.(png|tga|bmp|jpg)> <output.blp> [format_type (dxt5|dxt1|dxt3|argb8888|paletted)]\n";
        return 1;
    }

    std::string mode = args[1];
    std::string input = args[2];
    std::string output = args[3];

    if (mode == "decode") {
        BLPImage img;
        if (!BLPCodec::decode(input, img)) {
            std::cerr << "Failed to decode BLP: " << input << "\n";
            return 1;
        }
        if (img.mips.empty() || img.mips[0].rgba.empty()) {
            std::cerr << "Decoded BLP contains no data.\n";
            return 1;
        }

        bool success = false;
        std::string ext;
        const size_t dot_pos = output.find_last_of('.');
        if (dot_pos != std::string::npos) {
            ext = output.substr(dot_pos + 1);
            for (auto& c : ext) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }

        const int w = static_cast<int>(img.width);
        const int h = static_cast<int>(img.height);
        const void* rgba = img.mips[0].rgba.data();

        if (ext == "tga") {
            success = stbi_write_tga(output.c_str(), w, h, 4, rgba) != 0;
        } else if (ext == "bmp") {
            success = stbi_write_bmp(output.c_str(), w, h, 4, rgba) != 0;
        } else if (ext == "jpg" || ext == "jpeg") {
            success = stbi_write_jpg(output.c_str(), w, h, 4, rgba, 90) != 0;
        } else {
            success = stbi_write_png(output.c_str(), w, h, 4, rgba, w * 4) != 0;
        }

        if (!success) {
            std::cerr << "Failed to write output image: " << output << "\n";
            return 1;
        }
        std::cout << "Successfully decoded " << input << " to " << output << "\n";
    } else if (mode == "encode") {
        int w = 0;
        int h = 0;
        int channels = 0;
        unsigned char* data = stbi_load(input.c_str(), &w, &h, &channels, 4);
        if (data == nullptr) {
            std::cerr << "Failed to load image: " << input << "\n";
            return 1;
        }

        BLPImage img;
        img.width = static_cast<uint32_t>(w);
        img.height = static_cast<uint32_t>(h);
        img.mip_flags = eMipsGenerated;

        const auto pixel_count = static_cast<size_t>(w * h);
        const std::span<const unsigned char> pixel_data(data, pixel_count * 4);

        bool has_any_transparency = false;
        for (size_t i = 3; i < pixel_data.size(); i += 4) {
            if (pixel_data[i] < 255) {
                has_any_transparency = true;
                break;
            }
        }

        std::string format = (argc >= 5) ? args[4] : "dxt5";
        if (format == "dxt1") {
            img.color_encoding = eColorDxt;
            img.format = ePixelDxt1;
            img.alpha_depth = has_any_transparency ? 1 : 0;
        } else if (format == "dxt3") {
            img.color_encoding = eColorDxt;
            img.format = ePixelDxt3;
            img.alpha_depth = 8;
        } else if (format == "dxt5") {
            img.color_encoding = eColorDxt;
            img.format = ePixelDxt5;
            img.alpha_depth = 8;
        } else if (format == "argb8888") {
            img.color_encoding = eColorArgb8888;
            img.format = ePixelArgb8888;
            img.alpha_depth = 8;
        } else if (format == "paletted") {
            img.color_encoding = eColorPalette;
            img.format = ePixelUnspecified;
            img.alpha_depth = has_any_transparency ? 8 : 0;
        } else {
            std::cerr << "Unknown format: " << format << "\n";
            stbi_image_free(data);
            return 1;
        }

        MipLevel mip;
        mip.width = static_cast<uint32_t>(w);
        mip.height = static_cast<uint32_t>(h);
        mip.rgba.assign(pixel_data.begin(), pixel_data.end());
        img.mips.push_back(std::move(mip));

        stbi_image_free(data);

        if (!BLPCodec::encode(output, img)) {
            std::cerr << "Failed to encode BLP: " << output << "\n";
            return 1;
        }
        std::cout << "Successfully encoded " << input << " to " << output << "\n";
    } else {
        std::cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}

}  // namespace

int main(int argc, char** argv) noexcept {
    try {
        return run(argc, argv);
    } catch (...) {
        (void)std::fputs("Unexpected exception in main.\n", stderr);
        return 1;
    }
}
