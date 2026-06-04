#include "themes.hpp"

using namespace ooey;
using namespace gooey;

void register_sysinfo_themes(const std::shared_ptr<ThemeManager>& theme_manager) {
    // --- 1. DARK THEME DEFINITION ---
    auto dark_theme = std::make_shared<Theme>();
    dark_theme->name = "dark";
    dark_theme->set_style("window", Style{.fill_color=Color{18, 18, 22}});
    dark_theme->set_style("window-card", Style{.fill_color=Color{28, 28, 33}, .stroke_color=Color{55, 55, 65}, .stroke_thickness=1.5f, .text_color=Color{0,0,0,0}, .corner_radius=16});
    dark_theme->set_style("card-bg", Style{.fill_color=Color{23, 23, 27}, .stroke_color=Color{46, 46, 54}, .stroke_thickness=1.0f, .text_color=Color{0,0,0,0}, .corner_radius=10});
    dark_theme->set_style("title-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{255, 255, 255}});
    dark_theme->set_style("subtitle-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{150, 150, 160}});
    dark_theme->set_style("card-header-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 180, 240}});
    dark_theme->set_style("card-header-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{235, 160, 0}});
    dark_theme->set_style("card-header-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{180, 100, 240}});
    dark_theme->set_style("card-value-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 200, 110}});
    dark_theme->set_style("card-value-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{245, 175, 40}});
    dark_theme->set_style("card-value-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{200, 120, 255}});
    dark_theme->set_style("card-desc-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{145, 145, 155}});
    dark_theme->set_style("section-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{200, 200, 210}});
    dark_theme->set_style("theme-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{150, 150, 165}});
    dark_theme->set_style("list-box", Style{.fill_color=Color{20, 20, 24}, .stroke_color=Color{50, 50, 60}, .stroke_thickness=1.5f, .text_color=Color{210, 210, 215}, .corner_radius=8});
    dark_theme->set_style("scrollbar", Style{.fill_color=Color{25, 25, 30}, .stroke_color=Color{70, 70, 80}, .stroke_thickness=0.0f, .text_color=Color{0, 0, 0, 0}, .corner_radius=4});
    dark_theme->set_style("footnote-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{120, 120, 130}});
    dark_theme->set_style("btn-dark", Style{.fill_color=Color{0, 120, 215}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{255, 255, 255}, .corner_radius=6});
    dark_theme->set_style("btn-light", Style{.fill_color=Color{45, 45, 52}, .stroke_color=Color{75, 75, 85}, .stroke_thickness=1.5f, .text_color=Color{200, 200, 205}, .corner_radius=6});
    dark_theme->set_style("btn-hacker", Style{.fill_color=Color{45, 45, 52}, .stroke_color=Color{75, 75, 85}, .stroke_thickness=1.5f, .text_color=Color{200, 200, 205}, .corner_radius=6});
    dark_theme->set_style("btn-lofi", Style{.fill_color=Color{45, 45, 52}, .stroke_color=Color{75, 75, 85}, .stroke_thickness=1.5f, .text_color=Color{200, 200, 205}, .corner_radius=6});
    theme_manager->add_theme("dark", dark_theme);

    // --- 2. LIGHT CLEAN THEME DEFINITION ---
    auto light_theme = std::make_shared<Theme>();
    light_theme->name = "light";
    light_theme->set_style("window", Style{.fill_color=Color{242, 242, 247}});
    light_theme->set_style("window-card", Style{.fill_color=Color{255, 255, 255}, .stroke_color=Color{215, 215, 225}, .stroke_thickness=1.5f, .text_color=Color{0,0,0,0}, .corner_radius=16});
    light_theme->set_style("card-bg", Style{.fill_color=Color{248, 248, 250}, .stroke_color=Color{220, 220, 230}, .stroke_thickness=1.0f, .text_color=Color{0,0,0,0}, .corner_radius=10});
    light_theme->set_style("title-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{20, 20, 30}});
    light_theme->set_style("subtitle-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{100, 100, 115}});
    light_theme->set_style("card-header-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 100, 200}});
    light_theme->set_style("card-header-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{190, 110, 0}});
    light_theme->set_style("card-header-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{120, 40, 180}});
    light_theme->set_style("card-value-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 140, 70}});
    light_theme->set_style("card-value-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{200, 120, 0}});
    light_theme->set_style("card-value-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{130, 40, 190}});
    light_theme->set_style("card-desc-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{90, 90, 105}});
    light_theme->set_style("section-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{60, 60, 75}});
    light_theme->set_style("theme-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{110, 110, 125}});
    light_theme->set_style("list-box", Style{.fill_color=Color{255, 255, 255}, .stroke_color=Color{210, 210, 220}, .stroke_thickness=1.5f, .text_color=Color{50, 50, 60}, .corner_radius=8});
    light_theme->set_style("scrollbar", Style{.fill_color=Color{240, 240, 245}, .stroke_color=Color{180, 180, 185}, .stroke_thickness=0.0f, .text_color=Color{0, 0, 0, 0}, .corner_radius=4});
    light_theme->set_style("footnote-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{120, 120, 135}});
    light_theme->set_style("btn-dark", Style{.fill_color=Color{230, 230, 235}, .stroke_color=Color{195, 195, 205}, .stroke_thickness=1.0f, .text_color=Color{50, 50, 60}, .corner_radius=6});
    light_theme->set_style("btn-light", Style{.fill_color=Color{0, 90, 180}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{255, 255, 255}, .corner_radius=6});
    light_theme->set_style("btn-hacker", Style{.fill_color=Color{230, 230, 235}, .stroke_color=Color{195, 195, 205}, .stroke_thickness=1.0f, .text_color=Color{50, 50, 60}, .corner_radius=6});
    light_theme->set_style("btn-lofi", Style{.fill_color=Color{230, 230, 235}, .stroke_color=Color{195, 195, 205}, .stroke_thickness=1.0f, .text_color=Color{50, 50, 60}, .corner_radius=6});
    theme_manager->add_theme("light", light_theme);

    // --- 3. HACKER MONOCHROME GREEN THEME DEFINITION ---
    auto hacker_theme = std::make_shared<Theme>();
    hacker_theme->name = "hacker";
    hacker_theme->set_style("window", Style{.fill_color=Color{0, 0, 0}});
    hacker_theme->set_style("window-card", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 255, 0}, .stroke_thickness=2.0f, .text_color=Color{0,0,0,0}, .corner_radius=0});
    hacker_theme->set_style("card-bg", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 200, 0}, .stroke_thickness=1.5f, .text_color=Color{0,0,0,0}, .corner_radius=0});
    hacker_theme->set_style("title-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("subtitle-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 200, 0}});
    hacker_theme->set_style("card-header-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("card-header-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("card-header-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("card-value-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("card-value-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("card-value-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("card-desc-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 200, 0}});
    hacker_theme->set_style("section-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("theme-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 255, 0}});
    hacker_theme->set_style("list-box", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 255, 0}, .stroke_thickness=2.0f, .text_color=Color{0, 255, 0}, .corner_radius=0});
    hacker_theme->set_style("scrollbar", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 255, 0}, .stroke_thickness=0.0f, .text_color=Color{0, 0, 0, 0}, .corner_radius=0});
    hacker_theme->set_style("footnote-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 180, 0}});
    hacker_theme->set_style("btn-dark", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 255, 0}, .stroke_thickness=1.5f, .text_color=Color{0, 255, 0}, .corner_radius=0});
    hacker_theme->set_style("btn-light", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 255, 0}, .stroke_thickness=1.5f, .text_color=Color{0, 255, 0}, .corner_radius=0});
    hacker_theme->set_style("btn-hacker", Style{.fill_color=Color{0, 255, 0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{0, 0, 0}, .corner_radius=0});
    hacker_theme->set_style("btn-lofi", Style{.fill_color=Color{0, 0, 0}, .stroke_color=Color{0, 255, 0}, .stroke_thickness=1.5f, .text_color=Color{0, 255, 0}, .corner_radius=0});
    theme_manager->add_theme("hacker", hacker_theme);

    // --- 4. SOFT WARM LOFI THEME DEFINITION ---
    auto lofi_theme = std::make_shared<Theme>();
    lofi_theme->name = "lofi";
    lofi_theme->set_style("window", Style{.fill_color=Color{246, 238, 233}});
    lofi_theme->set_style("window-card", Style{.fill_color=Color{236, 224, 218}, .stroke_color=Color{205, 190, 183}, .stroke_thickness=1.5f, .text_color=Color{0,0,0,0}, .corner_radius=18});
    lofi_theme->set_style("card-bg", Style{.fill_color=Color{241, 231, 225}, .stroke_color=Color{212, 198, 191}, .stroke_thickness=1.0f, .text_color=Color{0,0,0,0}, .corner_radius=12});
    lofi_theme->set_style("title-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{105, 82, 76}});
    lofi_theme->set_style("subtitle-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{135, 115, 110}});
    lofi_theme->set_style("card-header-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{130, 92, 82}});
    lofi_theme->set_style("card-header-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{145, 102, 75}});
    lofi_theme->set_style("card-header-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{115, 90, 110}});
    lofi_theme->set_style("card-value-cpu", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{90, 75, 70}});
    lofi_theme->set_style("card-value-ram", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{90, 75, 70}});
    lofi_theme->set_style("card-value-disk", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{90, 75, 70}});
    lofi_theme->set_style("card-desc-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{130, 115, 110}});
    lofi_theme->set_style("section-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{100, 80, 75}});
    lofi_theme->set_style("theme-header", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{125, 105, 100}});
    lofi_theme->set_style("list-box", Style{.fill_color=Color{242, 232, 226}, .stroke_color=Color{200, 185, 178}, .stroke_thickness=1.5f, .text_color=Color{100, 82, 76}, .corner_radius=12});
    lofi_theme->set_style("scrollbar", Style{.fill_color=Color{235, 224, 217}, .stroke_color=Color{160, 140, 132}, .stroke_thickness=0.0f, .text_color=Color{0, 0, 0, 0}, .corner_radius=6});
    lofi_theme->set_style("footnote-text", Style{.fill_color=Color{0,0,0,0}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{140, 125, 120}});
    lofi_theme->set_style("btn-dark", Style{.fill_color=Color{228, 214, 208}, .stroke_color=Color{195, 180, 173}, .stroke_thickness=1.0f, .text_color=Color{110, 90, 85}, .corner_radius=12});
    lofi_theme->set_style("btn-light", Style{.fill_color=Color{228, 214, 208}, .stroke_color=Color{195, 180, 173}, .stroke_thickness=1.0f, .text_color=Color{110, 90, 85}, .corner_radius=12});
    lofi_theme->set_style("btn-hacker", Style{.fill_color=Color{228, 214, 208}, .stroke_color=Color{195, 180, 173}, .stroke_thickness=1.0f, .text_color=Color{110, 90, 85}, .corner_radius=12});
    lofi_theme->set_style("btn-lofi", Style{.fill_color=Color{130, 92, 82}, .stroke_color=Color{0,0,0,0}, .stroke_thickness=0.0f, .text_color=Color{255, 255, 255}, .corner_radius=12});
    theme_manager->add_theme("lofi", lofi_theme);
}
