#include <iostream>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/platform.hpp"
#include "ooey/mvvmc/view.hpp"
#include "ooey/controls/button.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"

int main() {
    std::cout << "Welcome to OOEY GUI Engine v" << ooey::get_version() << "!\n";

    ooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Hello World")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    // Construct a scene graph
    auto root_view = std::make_shared<ooey::View>();

    // Add an interactive red button background
    auto button = std::make_shared<ooey::Button>(ooey::Rect{300, 200, 200, 200}, ooey::Color{255, 0, 0});
    button->on_click = []() {
        std::cout << "Button clicked from Example!\n";
    };
    root_view->add_child(std::move(button));

    // Add a blue line across it
    root_view->add_child(std::make_shared<ooey::LinePrimitive>(ooey::Point{300, 200}, ooey::Point{500, 400}, ooey::Color{0, 0, 255}));

    // Inject the root view into the application
    app.set_root_view(std::move(root_view));

    // Set our background clear color
    app.set_clear_color(ooey::Color{40, 40, 40});

    app.run();

    return 0;
    }