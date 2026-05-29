#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/text_box.hpp"
#include "gooey/controls/list_control.hpp"
#include "gooey/renderer/primitives/circle_primitive.hpp"
#include "gooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "gooey/renderer/primitives/sinusoid_primitive.hpp"

int main() {
    std::cout << "Starting OOEY WebAssembly Application...\n";

    gooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY WebAssembly Application")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    auto root_view = std::make_shared<gooey::View>();

    // 1. Decorative background frame
    auto frame = std::make_shared<gooey::RoundedRectPrimitive>(
        ooey::Rect{50, 50, 700, 500},
        16,
        ooey::Color{30, 30, 35},
        ooey::Color{60, 60, 70},
        2.0f
    );
    root_view->add_child(std::move(frame));

    // 2. Title Label
    auto title = std::make_shared<gooey::Label>(
        "OOEY WebAssembly Canvas",
        ooey::Font{"sans-serif", 24, ooey::FontWeight::Bold},
        ooey::Point{100, 90},
        ooey::Color{255, 255, 255}
    );
    root_view->add_child(std::move(title));

    // 3. Name Input textbox
    auto label_name = std::make_shared<gooey::Label>(
        "Name:",
        ooey::Font{"sans-serif", 16},
        ooey::Point{100, 160},
        ooey::Color{200, 200, 200}
    );
    root_view->add_child(std::move(label_name));

    auto name_input = std::make_shared<gooey::TextBox>(
        ooey::Rect{180, 150, 280, 36},
        ooey::Font{"sans-serif", 16},
        ooey::Color{240, 240, 240},
        ooey::Color{45, 45, 50}
    );
    root_view->add_child(name_input);

    // 4. Greeting label
    auto greeting = std::make_shared<gooey::Label>(
        "",
        ooey::Font{"sans-serif", 18, ooey::FontWeight::Bold},
        ooey::Point{180, 210},
        ooey::Color{0, 200, 100}
    );
    root_view->add_child(greeting);

    // 5. Submit Button
    auto submit_btn = std::make_shared<gooey::Button>(
        ooey::Rect{480, 150, 100, 36},
        ooey::Color{0, 120, 215},
        ooey::Color{0, 0, 0, 0},
        0.0f,
        6,
        "Submit",
        ooey::Color{255, 255, 255}
    );
    submit_btn->on_click = [name_input, greeting]() {
        std::string name = name_input->get_text();
        if (name.empty()) {
            greeting->set_text("Enter a name first!");
            greeting->set_color(ooey::Color{255, 80, 80});
        } else {
            greeting->set_text("Hello, " + name + "!");
            greeting->set_color(ooey::Color{0, 200, 100});
        }
    };
    root_view->add_child(std::move(submit_btn));

    // 6. List Box Control
    auto label_list = std::make_shared<gooey::Label>(
        "Selection List:",
        ooey::Font{"sans-serif", 16},
        ooey::Point{100, 270},
        ooey::Color{200, 200, 200}
    );
    root_view->add_child(std::move(label_list));

    auto list_box = std::make_shared<gooey::ListControl>(
        ooey::Rect{100, 300, 240, 200},
        40,
        ooey::Font{"sans-serif", 14},
        ooey::Color{220, 220, 220},
        ooey::Color{45, 45, 50},
        ooey::Color{0, 120, 215},
        ooey::Color{255, 255, 255}
    );
    
    std::vector<std::string> items = {
        "Wasm Compilation",
        "Emscripten WebGL",
        "Legacy GL Emulation",
        "Retained Mode GUI",
        "HTML5 Event Handlers",
        "Outfit Typography",
        "Micro-animations",
        "Platform Independent"
    };
    list_box->set_items(items);
    root_view->add_child(list_box);

    // 7. Decorative Circle shape
    auto circle = std::make_shared<gooey::CirclePrimitive>(
        ooey::Point{480, 340},
        50,
        ooey::Color{100, 50, 200, 60},
        ooey::Color{100, 50, 200, 180},
        2.5f
    );
    root_view->add_child(std::move(circle));

    // 8. Sinusoid Wave animation
    auto sinusoid = std::make_shared<gooey::SinusoidPrimitive>(
        ooey::Rect{400, 420, 300, 80},
        4.0f, // frequency
        20.0f, // amplitude
        ooey::Color{0, 200, 100, 180},
        2.0f // thickness
    );
    auto sinusoid_ptr = sinusoid.get();
    root_view->add_child(std::move(sinusoid));

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{15, 15, 17});

    // Handle high resolution ticking to animate sinusoid wave phase
    float elapsed_time = 0.0f;
    app.set_before_render_callback([sinusoid_ptr, &elapsed_time](ooey::renderer::IRenderTarget*) {
        elapsed_time += 0.016f; // approx 60fps frame delta
        sinusoid_ptr->set_phase(elapsed_time * 2.0f);
    });

    app.run();

    return 0;
}
