#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/grid.hpp"
#include "gooey/controls/flow_layout.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"

using namespace ooey;
using namespace gooey;

int main() {
    std::cout << "Starting OOEY Layout & Responsive Design Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Reactive Layout Engine")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));
    app.set_clear_color(Color{24, 24, 28}); // Sleek dark grey background

    // Create the root layout container (Column)
    auto root_column = std::make_shared<Column>();
    root_column->set_width(SizePolicy::MatchParent);
    root_column->set_height(SizePolicy::MatchParent);
    root_column->set_padding(25);

    // 1. Header Section
    auto header = std::make_shared<Column>();
    header->set_width(SizePolicy::MatchParent);
    header->set_height(SizePolicy::WrapContent);
    header->set_margin(0, 0, 0, 15);

    auto title = std::make_shared<Label>(
        "Responsive Dashboard",
        Font{"sans-serif", 28, FontWeight::Bold},
        Point{0, 0},
        Color{255, 255, 255}
    );
    title->set_margin(0, 0, 0, 5);
    header->add_child(title);

    auto subtitle = std::make_shared<Label>(
        "Resize this window to watch the Row, Column, and Grid dynamically flow",
        Font{"sans-serif", 14},
        Point{0, 0},
        Color{150, 150, 160}
    );
    subtitle->set_margin(0, 0, 0, 15);
    header->add_child(subtitle);

    root_column->add_child(header);

    // 2. Buttons Row Section (wrapping layout)
    auto button_row = std::make_shared<FlowLayout>();
    button_row->set_width(SizePolicy::MatchParent);
    button_row->set_height(SizePolicy::WrapContent);
    button_row->set_margin(0, 0, 0, 20);

    auto btn1 = std::make_shared<Button>(
        Rect{0, 0, 140, 40},
        Color{0, 120, 215},
        Color{0, 0, 0, 0},
        0.0f,
        8,
        "Primary Action",
        Color{255, 255, 255}
    );
    btn1->set_margin(0, 5, 10, 5);
    btn1->on_click = []() {
        std::cout << "Primary Action Button Clicked!\n";
    };
    button_row->add_child(btn1);

    auto btn2 = std::make_shared<Button>(
        Rect{0, 0, 140, 40},
        Color{50, 50, 55},
        Color{80, 80, 90},
        1.5f,
        8,
        "Secondary Action",
        Color{220, 220, 230}
    );
    btn2->set_margin(0, 5, 10, 5);
    btn2->on_click = []() {
        std::cout << "Secondary Action Button Clicked!\n";
    };
    button_row->add_child(btn2);

    auto btn3 = std::make_shared<Button>(
        Rect{0, 0, 120, 40},
        Color{200, 50, 50},
        Color{0, 0, 0, 0},
        0.0f,
        8,
        "Delete / Alert",
        Color{255, 255, 255}
    );
    btn3->set_margin(0, 5, 10, 5);
    btn3->on_click = []() {
        std::cout << "Alert Action Clicked!\n";
    };
    button_row->add_child(btn3);

    auto btn4 = std::make_shared<Button>(
        Rect{0, 0, 110, 40},
        Color{50, 50, 55},
        Color{80, 80, 90},
        1.5f,
        8,
        "Option A",
        Color{220, 220, 230}
    );
    btn4->set_margin(0, 5, 10, 5);
    button_row->add_child(btn4);

    auto btn5 = std::make_shared<Button>(
        Rect{0, 0, 110, 40},
        Color{50, 50, 55},
        Color{80, 80, 90},
        1.5f,
        8,
        "Option B",
        Color{220, 220, 230}
    );
    btn5->set_margin(0, 5, 10, 5);
    button_row->add_child(btn5);

    auto btn6 = std::make_shared<Button>(
        Rect{0, 0, 130, 40},
        Color{40, 160, 100},
        Color{0, 0, 0, 0},
        0.0f,
        8,
        "More Actions...",
        Color{255, 255, 255}
    );
    btn6->set_margin(0, 5, 0, 5);
    button_row->add_child(btn6);

    root_column->add_child(button_row);

    // 3. Grid Section (2x2)
    auto grid = std::make_shared<Grid>(2, 2);
    grid->set_width(SizePolicy::MatchParent);
    grid->set_height(SizePolicy::Fixed, 200.0f);
    grid->set_margin(0, 0, 0, 20);

    // Cell 0,0: Label card
    auto cell1 = std::make_shared<Column>();
    cell1->set_padding(15);
    cell1->set_margin(5);
    cell1->set_align_self(Align::Stretch);
    auto cell1_bg = std::make_shared<RectPrimitive>(Rect{0, 0, 0, 0}, Color{35, 35, 40});
    cell1->add_child(cell1_bg);
    auto cell1_lbl = std::make_shared<Label>("System Analytics", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 180, 240});
    cell1_lbl->set_margin(0, 0, 0, 5);
    cell1->add_child(cell1_lbl);
    auto cell1_body = std::make_shared<Label>("CPU Load: 42%\nRAM usage: 5.4 GB / 16 GB", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
    cell1->add_child(cell1_body);
    grid->add_child(cell1);

    // Cell 0,1: TextBox card
    auto cell2 = std::make_shared<Column>();
    cell2->set_padding(15);
    cell2->set_margin(5);
    cell2->set_align_self(Align::Stretch);
    auto cell2_bg = std::make_shared<RectPrimitive>(Rect{0, 0, 0, 0}, Color{35, 35, 40});
    cell2->add_child(cell2_bg);
    auto cell2_lbl = std::make_shared<Label>("Interactive Console", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
    cell2_lbl->set_margin(0, 0, 0, 10);
    cell2->add_child(cell2_lbl);

    auto console_input = std::make_shared<TextBox>(
        Rect{0, 0, 100, 36},
        Font{"sans-serif", 14},
        Color{240, 240, 240},
        Color{45, 45, 50}
    );
    console_input->set_width(SizePolicy::MatchParent);
    cell2->add_child(console_input);
    grid->add_child(cell2);

    // Cell 1,0: Status Card
    auto cell3 = std::make_shared<Column>();
    cell3->set_padding(15);
    cell3->set_margin(5);
    cell3->set_align_self(Align::Stretch);
    auto cell3_bg = std::make_shared<RectPrimitive>(Rect{0, 0, 0, 0}, Color{35, 35, 40});
    cell3->add_child(cell3_bg);
    auto cell3_lbl = std::make_shared<Label>("Network Status", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{235, 160, 0});
    cell3_lbl->set_margin(0, 0, 0, 5);
    cell3->add_child(cell3_lbl);
    auto cell3_body = std::make_shared<Label>("Latency: 12ms\nStatus: Connected", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
    cell3->add_child(cell3_body);
    grid->add_child(cell3);

    // Cell 1,1: Options Card
    auto cell4 = std::make_shared<Column>();
    cell4->set_padding(15);
    cell4->set_margin(5);
    cell4->set_align_self(Align::Stretch);
    auto cell4_bg = std::make_shared<RectPrimitive>(Rect{0, 0, 0, 0}, Color{35, 35, 40});
    cell4->add_child(cell4_bg);
    auto cell4_lbl = std::make_shared<Label>("Configuration", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{180, 100, 240});
    cell4_lbl->set_margin(0, 0, 0, 5);
    cell4->add_child(cell4_lbl);
    auto cell4_body = std::make_shared<Label>("Mode:Retained\nTheme:Dark Modern", Font{"sans-serif", 12}, Point{0, 0}, Color{180, 180, 190});
    cell4->add_child(cell4_body);
    grid->add_child(cell4);

    root_column->add_child(grid);

    // 4. Greeting input flow
    auto footer_row = std::make_shared<Row>();
    footer_row->set_width(SizePolicy::MatchParent);
    footer_row->set_height(SizePolicy::WrapContent);
    footer_row->set_margin(0, 0, 0, 0);

    auto name_lbl = std::make_shared<Label>("Enter Name:", Font{"sans-serif", 16}, Point{0, 0}, Color{200, 200, 200});
    name_lbl->set_margin(0, 5, 10, 0);
    footer_row->add_child(name_lbl);

    auto name_box = std::make_shared<TextBox>(
        Rect{0, 0, 250, 36},
        Font{"sans-serif", 16},
        Color{255, 255, 255},
        Color{35, 35, 40}
    );
    footer_row->add_child(name_box);

    auto greeting_lbl = std::make_shared<Label>("", Font{"sans-serif", 16, FontWeight::Bold}, Point{0, 0}, Color{0, 200, 100});
    greeting_lbl->set_margin(0, 5, 0, 15);
    greeting_lbl->set_width(SizePolicy::MatchParent);
    
    name_box->on_text_changed = [greeting_lbl](const std::string& name) {
        if (name.empty()) {
            greeting_lbl->set_text("");
        } else {
            greeting_lbl->set_text("Hello, " + name + "! Welcome to OOEY.");
        }
    };

    root_column->add_child(footer_row);
    root_column->add_child(greeting_lbl);

    // Synchronize cell background rectangles to the laid-out sizes of the columns
    app.set_before_render_callback([cell1_bg, cell1, cell2_bg, cell2, cell3_bg, cell3, cell4_bg, cell4](IRenderTarget*) {
        cell1_bg->set_rect(Rect{cell1->layout_bounds.x, cell1->layout_bounds.y, cell1->layout_bounds.width, cell1->layout_bounds.height});
        cell2_bg->set_rect(Rect{cell2->layout_bounds.x, cell2->layout_bounds.y, cell2->layout_bounds.width, cell2->layout_bounds.height});
        cell3_bg->set_rect(Rect{cell3->layout_bounds.x, cell3->layout_bounds.y, cell3->layout_bounds.width, cell3->layout_bounds.height});
        cell4_bg->set_rect(Rect{cell4->layout_bounds.x, cell4->layout_bounds.y, cell4->layout_bounds.width, cell4->layout_bounds.height});
    });

    app.set_root_view(std::move(root_column));

    app.run();

    return 0;
}
