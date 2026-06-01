#include "themes.hpp"

using namespace ooey;
using namespace gooey;

void register_sysinfo_themes(const std::shared_ptr<ThemeManager>& theme_manager) {
    // --- 1. DARK THEME DEFINITION ---
    auto dark_theme = std::make_shared<Theme>();
    dark_theme->name = "dark";
    dark_theme->set_style("window", Style{Color{18, 18, 22}});
    dark_theme->set_style("window-card", Style{Color{28, 28, 33}, Color{55, 55, 65}, 1.5f, Color{0,0,0,0}, 16});
    dark_theme->set_style("card-bg", Style{Color{23, 23, 27}, Color{46, 46, 54}, 1.0f, Color{0,0,0,0}, 10});
    dark_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}});
    dark_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{150, 150, 160}});
    dark_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 180, 240}});
    dark_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{235, 160, 0}});
    dark_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{180, 100, 240}});
    dark_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 200, 110}});
    dark_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{245, 175, 40}});
    dark_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{200, 120, 255}});
    dark_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{145, 145, 155}});
    dark_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{200, 200, 210}});
    dark_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{150, 150, 165}});
    dark_theme->set_style("list-box", Style{Color{20, 20, 24}, Color{50, 50, 60}, 1.5f, Color{210, 210, 215}, 8});
    dark_theme->set_style("scrollbar", Style{Color{25, 25, 30}, Color{70, 70, 80}, 0.0f, Color{0, 0, 0, 0}, 4});
    dark_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{120, 120, 130}});
    dark_theme->set_style("btn-dark", Style{Color{0, 120, 215}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}, 6});
    dark_theme->set_style("btn-light", Style{Color{45, 45, 52}, Color{75, 75, 85}, 1.5f, Color{200, 200, 205}, 6});
    dark_theme->set_style("btn-hacker", Style{Color{45, 45, 52}, Color{75, 75, 85}, 1.5f, Color{200, 200, 205}, 6});
    dark_theme->set_style("btn-lofi", Style{Color{45, 45, 52}, Color{75, 75, 85}, 1.5f, Color{200, 200, 205}, 6});
    theme_manager->add_theme("dark", dark_theme);

    // --- 2. LIGHT CLEAN THEME DEFINITION ---
    auto light_theme = std::make_shared<Theme>();
    light_theme->name = "light";
    light_theme->set_style("window", Style{Color{242, 242, 247}});
    light_theme->set_style("window-card", Style{Color{255, 255, 255}, Color{215, 215, 225}, 1.5f, Color{0,0,0,0}, 16});
    light_theme->set_style("card-bg", Style{Color{248, 248, 250}, Color{220, 220, 230}, 1.0f, Color{0,0,0,0}, 10});
    light_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{20, 20, 30}});
    light_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{100, 100, 115}});
    light_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 100, 200}});
    light_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{190, 110, 0}});
    light_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{120, 40, 180}});
    light_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 140, 70}});
    light_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{200, 120, 0}});
    light_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{130, 40, 190}});
    light_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 90, 105}});
    light_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{60, 60, 75}});
    light_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{110, 110, 125}});
    light_theme->set_style("list-box", Style{Color{255, 255, 255}, Color{210, 210, 220}, 1.5f, Color{50, 50, 60}, 8});
    light_theme->set_style("scrollbar", Style{Color{240, 240, 245}, Color{180, 180, 185}, 0.0f, Color{0, 0, 0, 0}, 4});
    light_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{120, 120, 135}});
    light_theme->set_style("btn-dark", Style{Color{230, 230, 235}, Color{195, 195, 205}, 1.0f, Color{50, 50, 60}, 6});
    light_theme->set_style("btn-light", Style{Color{0, 90, 180}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}, 6});
    light_theme->set_style("btn-hacker", Style{Color{230, 230, 235}, Color{195, 195, 205}, 1.0f, Color{50, 50, 60}, 6});
    light_theme->set_style("btn-lofi", Style{Color{230, 230, 235}, Color{195, 195, 205}, 1.0f, Color{50, 50, 60}, 6});
    theme_manager->add_theme("light", light_theme);

    // --- 3. HACKER MONOCHROME GREEN THEME DEFINITION ---
    auto hacker_theme = std::make_shared<Theme>();
    hacker_theme->name = "hacker";
    hacker_theme->set_style("window", Style{Color{0, 0, 0}});
    hacker_theme->set_style("window-card", Style{Color{0, 0, 0}, Color{0, 255, 0}, 2.0f, Color{0,0,0,0}, 0});
    hacker_theme->set_style("card-bg", Style{Color{0, 0, 0}, Color{0, 200, 0}, 1.5f, Color{0,0,0,0}, 0});
    hacker_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 200, 0}});
    hacker_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 200, 0}});
    hacker_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("list-box", Style{Color{0, 0, 0}, Color{0, 255, 0}, 2.0f, Color{0, 255, 0}, 0});
    hacker_theme->set_style("scrollbar", Style{Color{0, 0, 0}, Color{0, 255, 0}, 0.0f, Color{0, 0, 0, 0}, 0});
    hacker_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 180, 0}});
    hacker_theme->set_style("btn-dark", Style{Color{0, 0, 0}, Color{0, 255, 0}, 1.5f, Color{0, 255, 0}, 0});
    hacker_theme->set_style("btn-light", Style{Color{0, 0, 0}, Color{0, 255, 0}, 1.5f, Color{0, 255, 0}, 0});
    hacker_theme->set_style("btn-hacker", Style{Color{0, 255, 0}, Color{0,0,0,0}, 0.0f, Color{0, 0, 0}, 0});
    hacker_theme->set_style("btn-lofi", Style{Color{0, 0, 0}, Color{0, 255, 0}, 1.5f, Color{0, 255, 0}, 0});
    theme_manager->add_theme("hacker", hacker_theme);

    // --- 4. SOFT WARM LOFI THEME DEFINITION ---
    auto lofi_theme = std::make_shared<Theme>();
    lofi_theme->name = "lofi";
    lofi_theme->set_style("window", Style{Color{246, 238, 233}});
    lofi_theme->set_style("window-card", Style{Color{236, 224, 218}, Color{205, 190, 183}, 1.5f, Color{0,0,0,0}, 18});
    lofi_theme->set_style("card-bg", Style{Color{241, 231, 225}, Color{212, 198, 191}, 1.0f, Color{0,0,0,0}, 12});
    lofi_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{105, 82, 76}});
    lofi_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{135, 115, 110}});
    lofi_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{130, 92, 82}});
    lofi_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{145, 102, 75}});
    lofi_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{115, 90, 110}});
    lofi_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 75, 70}});
    lofi_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 75, 70}});
    lofi_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 75, 70}});
    lofi_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{130, 115, 110}});
    lofi_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{100, 80, 75}});
    lofi_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{125, 105, 100}});
    lofi_theme->set_style("list-box", Style{Color{242, 232, 226}, Color{200, 185, 178}, 1.5f, Color{100, 82, 76}, 12});
    lofi_theme->set_style("scrollbar", Style{Color{235, 224, 217}, Color{160, 140, 132}, 0.0f, Color{0, 0, 0, 0}, 6});
    lofi_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{140, 125, 120}});
    lofi_theme->set_style("btn-dark", Style{Color{228, 214, 208}, Color{195, 180, 173}, 1.0f, Color{110, 90, 85}, 12});
    lofi_theme->set_style("btn-light", Style{Color{228, 214, 208}, Color{195, 180, 173}, 1.0f, Color{110, 90, 85}, 12});
    lofi_theme->set_style("btn-hacker", Style{Color{228, 214, 208}, Color{195, 180, 173}, 1.0f, Color{110, 90, 85}, 12});
    lofi_theme->set_style("btn-lofi", Style{Color{130, 92, 82}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}, 12});
    theme_manager->add_theme("lofi", lofi_theme);
}
