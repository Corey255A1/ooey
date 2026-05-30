#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/image_control.hpp"
#include "ooey/renderer/image.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

// Helper to write a 128x128 BMP file with a gradient pattern to test format loading
void create_test_bmp_pattern(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return;

    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t file_type{0x4D42}; // BM
        uint32_t file_size{14 + 40 + 128 * 128 * 4};
        uint16_t reserved1{0};
        uint16_t reserved2{0};
        uint32_t offset_data{14 + 40};
    };

    struct BMPInfoHeader {
        uint32_t size{40};
        int32_t width{128};
        int32_t height{128};
        uint16_t planes{1};
        uint16_t bit_count{32};
        uint32_t compression{0};
        uint32_t size_image{128 * 128 * 4};
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

    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            uint8_t r = static_cast<uint8_t>(x * 2);
            uint8_t g = static_cast<uint8_t>(y * 2);
            uint8_t b = 128;
            uint8_t a = 255;
            // BGRA layout in BMP
            f.put(b);
            f.put(g);
            f.put(r);
            f.put(a);
        }
    }
}

int main() {
    std::cout << "Starting OOEY Image Loading & Rendering Demo...\n";

    Application app;

    auto backend = create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Image Demo")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));
    app.set_clear_color(Color{30, 30, 35});

    // Create the test BMP pattern first
    std::string bmp_path = "test_pattern.bmp";
    create_test_bmp_pattern(bmp_path);

    // Root layout
    auto root_layout = std::make_shared<Column>();
    root_layout->set_width(SizePolicy::MatchParent);
    root_layout->set_height(SizePolicy::MatchParent);
    root_layout->set_padding(20);

    // Title label
    auto title = std::make_shared<Label>("OOEY Image Control Demo", Font{"sans-serif", 24, FontWeight::Bold}, Point{0, 0}, Color{255, 255, 255});
    title->set_margin(0, 0, 0, 10);
    root_layout->add_child(title);

    // Subtitle label
    auto subtitle = std::make_shared<Label>("Decodes and renders images natively, preserving alpha channels.", Font{"sans-serif", 14}, Point{0, 0}, Color{170, 170, 180});
    subtitle->set_margin(0, 0, 0, 20);
    root_layout->add_child(subtitle);

    // Row to place multiple images side-by-side
    auto image_row = std::make_shared<Row>();
    image_row->set_width(SizePolicy::MatchParent);
    image_row->set_height(SizePolicy::WrapContent);
    image_row->set_margin(0, 0, 0, 20);

    // 1. Decoded BMP image panel
    auto bmp_panel = std::make_shared<Column>();
    bmp_panel->set_margin(0, 0, 20, 0);

    auto bmp_title = std::make_shared<Label>("1. Generated BMP (32-bit)", Font{"sans-serif", 14, FontWeight::Bold}, Point{0, 0}, Color{0, 180, 240});
    bmp_title->set_margin(0, 0, 0, 8);
    bmp_panel->add_child(bmp_title);

    auto bmp_ctrl = std::make_shared<ImageControl>(Rect{0, 0, 200, 200}, bmp_path);
    bmp_panel->add_child(bmp_ctrl);

    image_row->add_child(bmp_panel);

    // 2. PNG image panel (compiled conditionally if libpng is found)
#ifdef OOEY_HAS_PNG
    auto png_panel1 = std::make_shared<Column>();
    png_panel1->set_margin(0, 0, 20, 0);

    auto png_title1 = std::make_shared<Label>("2. Home Icon (PNG)", Font{"sans-serif", 14, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
    png_title1->set_margin(0, 0, 0, 8);
    png_panel1->add_child(png_title1);

    // Path to home icon relative to project root / run location
    std::string home_png_path = "examples/images/home.png";
    
    // Check local fallback path if run inside build dir
    std::ifstream check_png(home_png_path);
    if (!check_png) {
        home_png_path = "../examples/images/home.png";
    }

    auto png_ctrl1 = std::make_shared<ImageControl>(Rect{0, 0, 200, 200}, home_png_path);
    png_panel1->add_child(png_ctrl1);

    image_row->add_child(png_panel1);

    // 3. Back Icon PNG panel
    auto png_panel2 = std::make_shared<Column>();
    
    auto png_title2 = std::make_shared<Label>("3. Back Icon (PNG)", Font{"sans-serif", 14, FontWeight::Bold}, Point{0, 0}, Color{235, 160, 0});
    png_title2->set_margin(0, 0, 0, 8);
    png_panel2->add_child(png_title2);

    std::string back_png_path = "examples/images/backicon.png";
    std::ifstream check_png2(back_png_path);
    if (!check_png2) {
        back_png_path = "../examples/images/backicon.png";
    }

    auto png_ctrl2 = std::make_shared<ImageControl>(Rect{0, 0, 200, 200}, back_png_path);
    png_panel2->add_child(png_ctrl2);

    image_row->add_child(png_panel2);
#else
    auto no_png_panel = std::make_shared<Column>();
    auto no_png_label = std::make_shared<Label>("PNG support was disabled during compilation.\nCompile with libpng to show PNG icons.", Font{"sans-serif", 14}, Point{0, 0}, Color{240, 100, 100});
    no_png_panel->add_child(no_png_label);
    image_row->add_child(no_png_panel);
#endif

    root_layout->add_child(image_row);

    app.set_root_view(std::move(root_layout));
    app.run();

    // Clean up generated BMP on exit
    std::remove(bmp_path.c_str());
    return 0;
}
