#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/list_control.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"

int main() {
    std::cout << "Starting OOEY List Control Demo...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY List Control Demo")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    auto root_view = std::make_shared<gooey::View>();

    // Background Card
    auto frame = std::make_shared<ooey::RoundedRectPrimitive>(
        ooey::Rect{50, 50, 700, 480},
        16,
        ooey::Color{30, 30, 35},
        ooey::Color{60, 60, 70},
        2.0f
    );
    root_view->add_child(std::move(frame));

    // Title
    auto title = std::make_shared<gooey::Label>(
        "Scrollable List View",
        ooey::Font{"sans-serif", 24, ooey::FontWeight::Bold},
        ooey::Point{100, 100},
        ooey::Color{255, 255, 255}
    );
    root_view->add_child(std::move(title));

    // Create 200 items
    std::vector<std::string> items;
    for (int i = 1; i <= 200; ++i) {
        items.push_back("Element " + std::to_string(i));
    }

    // List Control
    auto list_control = std::make_shared<gooey::ListControl>(
        ooey::Rect{100, 160, 350, 250},
        50,
        ooey::Font{"sans-serif", 16},
        ooey::Color{200, 200, 200},        // unselected text
        ooey::Color{45, 45, 50},            // list background
        ooey::Color{0, 120, 215},           // selected background (blue)
        ooey::Color{255, 255, 255}          // selected text
    );
    list_control->set_items(items);
    list_control->set_selected_index(0);
    root_view->add_child(list_control);

    // Selected Info Label
    auto selected_info = std::make_shared<gooey::Label>(
        "Selected: Element 1 (Index 0 / 199)",
        ooey::Font{"sans-serif", 16},
        ooey::Point{100, 430},
        ooey::Color{0, 200, 100}
    );
    root_view->add_child(selected_info);

    // Update Label callback on selection change
    list_control->on_selected_changed = [selected_info](int index) {
        selected_info->set_text("Selected: Element " + std::to_string(index + 1) + " (Index " + std::to_string(index) + " / 199)");
    };

    // Scroll Up Button
    auto scroll_up_btn = std::make_shared<gooey::Button>(
        ooey::Rect{480, 200, 180, 40},
        ooey::Color{45, 45, 50},          // fill
        ooey::Color{100, 100, 110},       // border
        1.5f,
        6,
        "Scroll Up",
        ooey::Color{240, 240, 240}
    );
    scroll_up_btn->on_click = [list_control]() {
        list_control->select_previous();
    };
    root_view->add_child(std::move(scroll_up_btn));

    // Scroll Down Button
    auto scroll_down_btn = std::make_shared<gooey::Button>(
        ooey::Rect{480, 260, 180, 40},
        ooey::Color{45, 45, 50},          // fill
        ooey::Color{100, 100, 110},       // border
        1.5f,
        6,
        "Scroll Down",
        ooey::Color{240, 240, 240}
    );
    scroll_down_btn->on_click = [list_control]() {
        list_control->select_next();
    };
    root_view->add_child(std::move(scroll_down_btn));

    // Info text for keyboard
    auto keyboard_info = std::make_shared<gooey::Label>(
        "Use Up / Down Arrow Keys on the list to navigate",
        ooey::Font{"sans-serif", 14},
        ooey::Point{100, 465},
        ooey::Color{150, 150, 150}
    );
    root_view->add_child(keyboard_info);

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{15, 15, 17});

    app.run();

    return 0;
}
