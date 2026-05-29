#include <iostream>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform/wayland/window_backend.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/button.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"

int main() {
    std::cout << "Welcome to OOEY GUI Engine v" << ooey::get_version() << " (Wayland)\n";

    gooey::Application app;

    auto backend = std::make_unique<ooey::wayland::WindowBackend>();
    if (!backend->create({800, 600}, "OOEY Hello Wayland")) {
        std::cerr << "Failed to create Wayland window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    auto root_view = std::make_shared<gooey::View>();

    auto button = std::make_shared<gooey::Button>(ooey::Rect{300, 200, 200, 200}, ooey::Color{255, 0, 0});
    button->on_click = []() {
        std::cout << "Button clicked from Wayland Example!\n";
    };
    root_view->add_child(std::move(button));

    root_view->add_child(std::make_shared<ooey::LinePrimitive>(ooey::Point{300, 200}, ooey::Point{500, 400}, ooey::Color{0, 0, 255}));

    app.set_root_view(std::move(root_view));

    app.set_clear_color(ooey::Color{40, 40, 40});
    std::cout << "Starting to Run\n";
    app.run();

    return 0;
}
