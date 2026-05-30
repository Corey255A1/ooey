#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/list_control.hpp"
#include "ooey/renderer/font_engine.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"

// Utility to keep dynamically created font family strings alive.
// Font struct family member is a const char* pointing to persistent storage.
const char* keep_alive_family(const std::string& family) {
    static std::unordered_set<std::string> families;
    return families.insert(family).first->c_str();
}

int main() {
    std::cout << "Starting OOEY Cross-Platform Font Rendering Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({850, 650}, "OOEY Font Handling Demo")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));
    app.set_clear_color(ooey::Color{18, 18, 22}); // Sleek dark aesthetic

    auto root_view = std::make_shared<gooey::View>();

    // Background modern card container
    auto card_frame = std::make_shared<ooey::RoundedRectPrimitive>(
        ooey::Rect{40, 40, 770, 570},
        16,
        ooey::Color{28, 28, 33}, // Card background
        ooey::Color{55, 55, 65}, // Subtle border
        1.5f
    );
    root_view->add_child(card_frame);

    // Title
    auto title = std::make_shared<gooey::Label>(
        "System Font Renderer",
        ooey::Font{"sans-serif", 24, ooey::FontWeight::Bold},
        ooey::Point{80, 70},
        ooey::Color{255, 255, 255}
    );
    root_view->add_child(title);

    // Subtitle
    auto subtitle = std::make_shared<gooey::Label>(
        "Cross-platform OS-specific TrueType font matching & rendering integration",
        ooey::Font{"sans-serif", 13},
        ooey::Point{80, 105},
        ooey::Color{150, 150, 165}
    );
    root_view->add_child(subtitle);

    // Left Column: Fonts list header
    auto list_header = std::make_shared<gooey::Label>(
        "Available Fonts",
        ooey::Font{"sans-serif", 14, ooey::FontWeight::Bold},
        ooey::Point{80, 145},
        ooey::Color{180, 180, 195}
    );
    root_view->add_child(list_header);

    // Query available fonts from the OS using our new backend framework
    std::vector<std::string> available_fonts = ooey::FontEngine::get_available_fonts();
    
    // Create ListControl with stylized item flag enabled
    auto font_list = std::make_shared<gooey::ListControl>(
        ooey::Rect{80, 175, 280, 360},
        45,                                 // Item height
        ooey::Font{"sans-serif", 15},
        ooey::Color{200, 200, 205},         // Unselected text
        ooey::Color{36, 36, 42},            // Background
        ooey::Color{0, 120, 215},           // Highlight BG
        ooey::Color{255, 255, 255}          // Highlight text
    );
    font_list->set_stylize_items(true);     // Each item renders using its own font family!
    font_list->set_items(available_fonts);
    root_view->add_child(font_list);

    // Right Column: Preview header
    auto preview_header = std::make_shared<gooey::Label>(
        "Lorem Ipsum Preview",
        ooey::Font{"sans-serif", 14, ooey::FontWeight::Bold},
        ooey::Point{390, 145},
        ooey::Color{180, 180, 195}
    );
    root_view->add_child(preview_header);

    // Preview area inner frame background
    auto preview_bg = std::make_shared<ooey::RoundedRectPrimitive>(
        ooey::Rect{390, 175, 380, 260},
        10,
        ooey::Color{20, 20, 24},
        ooey::Color{46, 46, 52},
        1.0f
    );
    root_view->add_child(preview_bg);

    // Lorem Ipsum text lines for multi-line block preview
    std::vector<std::string> preview_lines = {
        "Lorem ipsum dolor sit amet,",
        "consectetur adipiscing elit.",
        "Sed do eiusmod tempor incididunt",
        "ut labore et dolore magna aliqua.",
        "Ut enim ad minim veniam, quis",
        "nostrud exercitation ullamco."
    };

    std::vector<std::shared_ptr<gooey::Label>> preview_labels;
    for (size_t i = 0; i < preview_lines.size(); ++i) {
        auto line_lbl = std::make_shared<gooey::Label>(
            preview_lines[i],
            ooey::Font{"sans-serif", 16},
            ooey::Point{415, 200 + static_cast<int>(i) * 26},
            ooey::Color{230, 230, 235}
        );
        preview_labels.push_back(line_lbl);
        root_view->add_child(line_lbl);
    }

    // Details Status Bar
    auto details_label = std::make_shared<gooey::Label>(
        "Active Font: sans-serif | Size: 16px | Style: Normal",
        ooey::Font{"sans-serif", 13},
        ooey::Point{390, 455},
        ooey::Color{0, 200, 110}
    );
    root_view->add_child(details_label);

    // Font State variables
    std::string current_family = available_fonts.empty() ? "sans-serif" : available_fonts[0];
    int current_size = 18;
    ooey::FontWeight current_weight = ooey::FontWeight::Normal;
    ooey::FontStyle current_style = ooey::FontStyle::Normal;

    // Adjust button UI colors based on toggle states
    auto btn_bg_color = ooey::Color{45, 45, 52};
    auto btn_active_color = ooey::Color{0, 120, 215};
    auto btn_border_color = ooey::Color{75, 75, 85};

    // Update UI components when state shifts
    auto update_preview = [&]() {
        const char* alive_fam = keep_alive_family(current_family);
        ooey::Font preview_font{alive_fam, current_size, current_weight, current_style};

        int line_height = ooey::FontEngine::measure_text("A", preview_font).height;
        if (line_height < current_size) {
            line_height = current_size + 6;
        }

        // Layout the labels inside the card frame
        for (size_t i = 0; i < preview_labels.size(); ++i) {
            preview_labels[i]->set_font(preview_font);
            preview_labels[i]->set_position(ooey::Point{415, 200 + static_cast<int>(i) * line_height});
        }

        // Detail text
        std::string weight_str = (current_weight == ooey::FontWeight::Bold) ? "Bold " : "";
        std::string style_str = (current_style == ooey::FontStyle::Italic) ? "Italic" : "";
        std::string comb = weight_str + style_str;
        if (comb.empty()) comb = "Normal";

        details_label->set_text("Active Font: " + current_family + " | Size: " + std::to_string(current_size) + "px | Style: " + comb);
    };

    // Size controls layout
    auto size_lbl = std::make_shared<gooey::Label>(
        "Size: 18px",
        ooey::Font{"sans-serif", 14},
        ooey::Point{390, 500},
        ooey::Color{180, 180, 190}
    );
    root_view->add_child(size_lbl);

    auto dec_size_btn = std::make_shared<gooey::Button>(
        ooey::Rect{480, 492, 35, 32},
        btn_bg_color,
        btn_border_color,
        1.5f,
        6,
        "-",
        ooey::Color{240, 240, 240}
    );
    dec_size_btn->on_click = [&, size_lbl]() {
        current_size = std::max(10, current_size - 2);
        size_lbl->set_text("Size: " + std::to_string(current_size) + "px");
        update_preview();
    };
    root_view->add_child(dec_size_btn);

    auto inc_size_btn = std::make_shared<gooey::Button>(
        ooey::Rect{525, 492, 35, 32},
        btn_bg_color,
        btn_border_color,
        1.5f,
        6,
        "+",
        ooey::Color{240, 240, 240}
    );
    inc_size_btn->on_click = [&, size_lbl]() {
        current_size = std::min(32, current_size + 2);
        size_lbl->set_text("Size: " + std::to_string(current_size) + "px");
        update_preview();
    };
    root_view->add_child(inc_size_btn);

    // Style toggles
    auto bold_btn = std::make_shared<gooey::Button>(
        ooey::Rect{580, 492, 80, 32},
        btn_bg_color,
        btn_border_color,
        1.5f,
        6,
        "Bold",
        ooey::Color{230, 230, 230}
    );
    bold_btn->on_click = [&, bold_btn]() {
        if (current_weight == ooey::FontWeight::Bold) {
            current_weight = ooey::FontWeight::Normal;
            bold_btn->set_fill_color(btn_bg_color);
        } else {
            current_weight = ooey::FontWeight::Bold;
            bold_btn->set_fill_color(btn_active_color);
        }
        update_preview();
    };
    root_view->add_child(bold_btn);

    auto italic_btn = std::make_shared<gooey::Button>(
        ooey::Rect{670, 492, 80, 32},
        btn_bg_color,
        btn_border_color,
        1.5f,
        6,
        "Italic",
        ooey::Color{230, 230, 230}
    );
    italic_btn->on_click = [&, italic_btn]() {
        if (current_style == ooey::FontStyle::Italic) {
            current_style = ooey::FontStyle::Normal;
            italic_btn->set_fill_color(btn_bg_color);
        } else {
            current_style = ooey::FontStyle::Italic;
            italic_btn->set_fill_color(btn_active_color);
        }
        update_preview();
    };
    root_view->add_child(italic_btn);

    // Set list control selection callback
    font_list->on_selected_changed = [&](int index) {
        if (index >= 0 && index < static_cast<int>(available_fonts.size())) {
            current_family = available_fonts[index];
            update_preview();
        }
    };

    // Keyboard navigation instructions
    auto instruction_lbl = std::make_shared<gooey::Label>(
        "Use Up/Down Arrow keys or click to select fonts from the list.",
        ooey::Font{"sans-serif", 13},
        ooey::Point{80, 550},
        ooey::Color{130, 130, 140}
    );
    root_view->add_child(instruction_lbl);

    // Initial load
    if (!available_fonts.empty()) {
        font_list->set_selected_index(0);
        current_family = available_fonts[0];
    }
    update_preview();

    app.set_root_view(std::move(root_view));
    app.run();

    return 0;
}
