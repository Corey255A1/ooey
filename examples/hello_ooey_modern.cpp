#include <iostream>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/platform.hpp"
#include "ooey/mvvmc/view.hpp"
#include "ooey/controls/button.hpp"
#include "ooey/controls/label.hpp"
#include "ooey/controls/text_box.hpp"
#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/curve_primitive.hpp"

int main() {
    std::cout << "Starting OOEY Modern GUI Demo...\n";

    ooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({800, 600}, "OOEY Modern Shapes & Input")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    auto root_view = std::make_shared<ooey::View>();

    // 1. Add a beautiful decorative dark frame using RoundedRectPrimitive
    auto frame = std::make_shared<ooey::RoundedRectPrimitive>(
        ooey::Rect{60, 60, 680, 480},
        16,                              // corner radius
        ooey::Color{30, 30, 35},         // dark gray fill
        ooey::Color{60, 60, 70},         // subtle border
        2.0f                             // border thickness
    );
    root_view->add_child(std::move(frame));

    // 2. Add some decorative colorful shapes in the background to show off the primitives
    auto glow_circle = std::make_shared<ooey::CirclePrimitive>(
        ooey::Point{650, 450},
        60,
        ooey::Color{0, 120, 215, 60},    // transparent accent blue
        ooey::Color{0, 120, 215, 180},   // border
        3.0f
    );
    root_view->add_child(std::move(glow_circle));

    auto curve_deco = std::make_shared<ooey::CurvePrimitive>(
        ooey::Point{100, 400},
        ooey::Point{250, 500},
        ooey::Point{400, 450},
        ooey::Color{100, 50, 200, 150},
        4.0f
    );
    root_view->add_child(std::move(curve_deco));

    // 3. Add a Title Label
    auto title = std::make_shared<ooey::Label>(
        "User Onboarding",
        ooey::Font{"sans-serif", 24, ooey::FontWeight::Bold},
        ooey::Point{100, 100},
        ooey::Color{255, 255, 255}
    );
    root_view->add_child(std::move(title));

    // 4. Add the Name Label
    auto label_name = std::make_shared<ooey::Label>(
        "Name:",
        ooey::Font{"sans-serif", 16},
        ooey::Point{100, 180},
        ooey::Color{200, 200, 200}
    );
    root_view->add_child(std::move(label_name));

    // 5. Add the TextBox for entering the name
    auto name_input = std::make_shared<ooey::TextBox>(
        ooey::Rect{180, 172, 300, 36},
        ooey::Font{"sans-serif", 16},
        ooey::Color{240, 240, 240},      // text color
        ooey::Color{45, 45, 50}          // background color
    );
    root_view->add_child(name_input);

    // 6. Add a Label to display the greeting (initially empty)
    auto greeting = std::make_shared<ooey::Label>(
        "",
        ooey::Font{"sans-serif", 20, ooey::FontWeight::Bold},
        ooey::Point{180, 280},
        ooey::Color{0, 200, 100}
    );
    root_view->add_child(greeting);

    // 7. Add the Modern Button with rounded corners (6px) and text says "Submit"
    auto submit_btn = std::make_shared<ooey::Button>(
        ooey::Rect{500, 172, 120, 36},
        ooey::Color{0, 120, 215},        // Accent blue fill
        ooey::Color{0, 0, 0, 0},         // Transparent stroke
        0.0f,                            // 0px border
        6,                               // Corner radius
        "Submit",                        // Button text
        ooey::Color{255, 255, 255}       // Text color
    );

    // 8. Event handler: When button is clicked, greet the user
    submit_btn->on_click = [name_input, greeting]() {
        std::string name = name_input->get_text();
        if (name.empty()) {
            greeting->set_text("Please enter a name!");
            greeting->set_color(ooey::Color{255, 80, 80}); // Red error
        } else {
            greeting->set_text("Hello, " + name + "!");
            greeting->set_color(ooey::Color{0, 200, 100}); // Green success
        }
    };
    root_view->add_child(std::move(submit_btn));

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{18, 18, 20}); // Deep dark canvas bg

    app.run();

    return 0;
}
