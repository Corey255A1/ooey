#include "ooey/renderer/image/png_decoder.hpp"
#include "ooey/renderer/image.hpp"
#include <png.h>
#include <fstream>
#include <vector>
#include <iostream>

namespace ooey::renderer {

bool PngDecoder::can_decode(const std::string& /*path*/, const std::vector<uint8_t>& header) {
    if (header.size() < 8) return false;
    return png_sig_cmp(header.data(), 0, 8) == 0;
}

std::shared_ptr<Image> PngDecoder::decode(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) return nullptr;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return nullptr;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return nullptr;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return nullptr;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    std::vector<uint8_t> pixels(width * height * 4, 0);
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; ++y) {
        row_pointers[y] = pixels.data() + y * width * 4;
    }

    png_read_image(png, row_pointers.data());
    png_read_end(png, nullptr);

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);

    return std::make_shared<Image>(width, height, std::move(pixels));
}

} // namespace ooey::renderer
