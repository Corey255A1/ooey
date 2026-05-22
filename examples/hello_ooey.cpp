#include <iostream>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/x11/x11_window_backend.hpp"

int main() {
    std::cout << "Welcome to OOEY GUI Engine v" << ooey::get_version() << "!\n";

    ooey::Application app;
    
    auto backend = std::make_unique<ooey::X11WindowBackend>();
    if (!backend->create({800, 600}, "OOEY Hello World")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    app.set_render_callback([](ooey::IRenderTarget* target) {
        if (!target) return;
        
        // Clear window with dark gray
        target->clear(ooey::Color{40, 40, 40});
        
        // Draw a red square in the center (assuming 800x600 size)
        target->draw_rect(ooey::Rect{300, 200, 200, 200}, ooey::Color{0, 255, 0});
        
        target->present();
    });

    app.run();

    return 0;
}