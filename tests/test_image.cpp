#include <gtest/gtest.h>
#include "ooey/renderer/image.hpp"
#include "ooey/renderer/image_decoder_registry.hpp"
#include "gooey/controls/image_control.hpp"
#include "ooey/renderer/software_render_target.hpp"
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstdio>

#ifdef OOEY_HAS_PNG
#include <png.h>
#endif

namespace {

// Helper to write a 2x2 BMP with alpha channel (32-bit BMP)
void write_test_bmp(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t file_type{0x4D42}; // BM
        uint32_t file_size{14 + 40 + 16};
        uint16_t reserved1{0};
        uint16_t reserved2{0};
        uint32_t offset_data{14 + 40};
    };

    struct BMPInfoHeader {
        uint32_t size{40};
        int32_t width{2};
        int32_t height{2};
        uint16_t planes{1};
        uint16_t bit_count{32};
        uint32_t compression{0};
        uint32_t size_image{16};
        int32_t x_pixels_per_meter{0};
        int32_t y_pixels_per_meter{0};
        uint32_t colors_used{0};
        uint32_t colors_important{0};
    };
    #pragma pack(pop)

    BMPFileHeader fh;
    BMPInfoHeader ih;

    f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
    f.write(reinterpret_cast<const char*>(&ih), sizeof(ih));

    // Write 4 pixels (BGRA format):
    // Pixel 0: Red, fully opaque (B=0, G=0, R=255, A=255)
    // Pixel 1: Green, semi-transparent (B=0, G=255, R=0, A=128)
    // Pixel 2: Blue, fully opaque (B=255, G=0, R=0, A=255)
    // Pixel 3: Yellow, fully transparent (B=0, G=255, R=255, A=0)
    uint8_t pixel_data[16] = {
        0, 0, 255, 255,      // Red
        0, 255, 0, 128,      // Green
        255, 0, 0, 255,      // Blue
        0, 255, 255, 0       // Yellow
    };
    f.write(reinterpret_cast<const char*>(pixel_data), sizeof(pixel_data));
}

#ifdef OOEY_HAS_PNG
// Helper to write a 2x2 PNG with alpha channel using libpng
void write_test_png(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) return;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    png_set_IHDR(
        png, info, 2, 2, 8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    uint8_t pixel_data[16] = {
        255, 0, 0, 255,      // Red
        0, 255, 0, 128,      // Green
        0, 0, 255, 255,      // Blue
        255, 255, 0, 0       // Yellow
    };

    png_bytep row_pointers[2] = {
        pixel_data,
        pixel_data + 8
    };

    png_write_image(png, row_pointers);
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
}
#endif

class MockRenderTarget : public ooey::IRenderTarget {
public:
    mutable bool draw_image_called{false};
    mutable const ooey::Image* last_image{nullptr};
    mutable ooey::Rect last_dest{0, 0, 0, 0};

    void clear(ooey::Color /*color*/) override {}
    void draw_geometry(const ooey::Geometry& /*geometry*/) override {}
    
    void draw_image(const ooey::Image& image, const ooey::Rect& dest_rect) override {
        draw_image_called = true;
        last_image = &image;
        last_dest = dest_rect;
    }

    ooey::Size measure_text(const std::string& /*text*/, const ooey::Font& /*font*/) override {
        return ooey::Size{0, 0};
    }
    void draw_text(const std::string& /*text*/, const ooey::Font& /*font*/, const ooey::Point& /*position*/, ooey::Color /*color*/) override {}
    void present() override {}
};

} // namespace

TEST(OoeyImage, BmpDecoding) {
    std::string bmp_path = "test_image_generated.bmp";
    write_test_bmp(bmp_path);

    auto img = ooey::Image::load_from_file(bmp_path);
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->width(), 2);
    EXPECT_EQ(img->height(), 2);

    const auto& pixels = img->data();
    ASSERT_EQ(pixels.size(), 16);

    // Test Image y=0, x=0 (Pixel 2 - Blue)
    EXPECT_EQ(pixels[0], 0);
    EXPECT_EQ(pixels[1], 0);
    EXPECT_EQ(pixels[2], 255);
    EXPECT_EQ(pixels[3], 255);

    // Test Image y=0, x=1 (Pixel 3 - Yellow)
    EXPECT_EQ(pixels[4], 255);
    EXPECT_EQ(pixels[5], 255);
    EXPECT_EQ(pixels[6], 0);
    EXPECT_EQ(pixels[7], 0);

    // Test Image y=1, x=0 (Pixel 0 - Red)
    EXPECT_EQ(pixels[8], 255);
    EXPECT_EQ(pixels[9], 0);
    EXPECT_EQ(pixels[10], 0);
    EXPECT_EQ(pixels[11], 255);

    // Test Image y=1, x=1 (Pixel 1 - Green)
    EXPECT_EQ(pixels[12], 0);
    EXPECT_EQ(pixels[13], 255);
    EXPECT_EQ(pixels[14], 0);
    EXPECT_EQ(pixels[15], 128);

    std::remove(bmp_path.c_str());
}

#ifdef OOEY_HAS_PNG
TEST(OoeyImage, PngDecoding) {
    std::string png_path = "test_image_generated.png";
    write_test_png(png_path);

    auto img = ooey::Image::load_from_file(png_path);
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->width(), 2);
    EXPECT_EQ(img->height(), 2);

    const auto& pixels = img->data();
    ASSERT_EQ(pixels.size(), 16);

    // Image y=0, x=0 (Red)
    EXPECT_EQ(pixels[0], 255);
    EXPECT_EQ(pixels[1], 0);
    EXPECT_EQ(pixels[2], 0);
    EXPECT_EQ(pixels[3], 255);

    // Image y=0, x=1 (Green)
    EXPECT_EQ(pixels[4], 0);
    EXPECT_EQ(pixels[5], 255);
    EXPECT_EQ(pixels[6], 0);
    EXPECT_EQ(pixels[7], 128);

    // Image y=1, x=0 (Blue)
    EXPECT_EQ(pixels[8], 0);
    EXPECT_EQ(pixels[9], 0);
    EXPECT_EQ(pixels[10], 255);
    EXPECT_EQ(pixels[11], 255);

    // Image y=1, x=1 (Yellow)
    EXPECT_EQ(pixels[12], 255);
    EXPECT_EQ(pixels[13], 255);
    EXPECT_EQ(pixels[14], 0);
    EXPECT_EQ(pixels[15], 0);

    std::remove(png_path.c_str());
}
#endif

TEST(GooeyControls, ImageControlRendering) {
    auto img = std::make_shared<ooey::Image>(2, 2, std::vector<uint8_t>(16, 255));
    gooey::ImageControl img_ctrl{ooey::Rect{10, 20, 100, 200}, img};

    EXPECT_EQ(img_ctrl.bounds().x, 10);
    EXPECT_EQ(img_ctrl.bounds().y, 20);
    EXPECT_EQ(img_ctrl.bounds().width, 100);
    EXPECT_EQ(img_ctrl.bounds().height, 200);

    MockRenderTarget target;
    img_ctrl.draw(target);

    EXPECT_TRUE(target.draw_image_called);
    EXPECT_EQ(target.last_image, img.get());
    EXPECT_EQ(target.last_dest.x, 10);
    EXPECT_EQ(target.last_dest.y, 20);
    EXPECT_EQ(target.last_dest.width, 100);
    EXPECT_EQ(target.last_dest.height, 200);
}
