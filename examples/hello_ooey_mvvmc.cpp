#include <iostream>
#include <memory>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/x11/x11_window_backend.hpp"
#include "ooey/view.hpp"
#include "ooey/controls/button.hpp"
#include "ooey/mvvmc/property.hpp"

// ---------------------------------------------------------
// 1. The ViewModel (Logic & State)
// ---------------------------------------------------------
// This class contains NO reference to UI objects. It solely
// manages the application state and business logic.
class MainViewModel {
public:
    // Properties that the View will observe
    ooey::Property<ooey::Color> color_a{ooey::Color{255, 0, 0}};   // Starts Red
    ooey::Property<ooey::Color> color_b{ooey::Color{0, 255, 0}}; // Starts Green

    // Commands/Methods triggered by the UI
    void on_box_a_clicked() {
        std::cout << "ViewModel: Box A was clicked! Changing Box B's color...\n";
        if (color_b.get().r == 0) {
            color_b.set(ooey::Color{255, 255, 0}); // Change to Yellow
        } else {
            color_b.set(ooey::Color{0, 255, 0}); // Change back to Green
        }
    }

    void on_box_b_clicked() {
        std::cout << "ViewModel: Box B was clicked! Changing Box A's color...\n";
        if (color_a.get().b == 0) {
            color_a.set(ooey::Color{0, 0, 255}); // Change to Blue
        } else {
            color_a.set(ooey::Color{255, 0, 0}); // Change back to Red
        }
    }
};

// ---------------------------------------------------------
// 2. The View (UI Construction & Binding)
// ---------------------------------------------------------
// This class composes the visual elements and binds them
// to the ViewModel properties and commands.
class MainView : public ooey::View {
public:
    MainView(std::shared_ptr<MainViewModel> view_model) : view_model_(std::move(view_model)) {
        // Construct visual elements
        auto box_a = std::make_shared<ooey::Button>(ooey::Rect{100, 200, 200, 200}, ooey::Color{0, 0, 0});
        auto box_b = std::make_shared<ooey::Button>(ooey::Rect{500, 200, 200, 200}, ooey::Color{0, 0, 0});

        // -- Data Binding: State (ViewModel) -> UI (View) --
        // When color_a changes, update box_a
        bind(view_model_->color_a, [box_a](const ooey::Color& c) {
            box_a->set_color(c);
        });

        // When color_b changes, update box_b
        bind(view_model_->color_b, [box_b](const ooey::Color& c) {
            box_b->set_color(c);
        });

        // -- Command Binding: UI (View) -> Action (ViewModel) --
        // Capture the view_model by value (shared_ptr) so it lives as long as the callbacks
        auto vm = view_model_;
        
        box_a->on_click = [vm]() {
            vm->on_box_a_clicked();
        };

        box_b->on_click = [vm]() {
            vm->on_box_b_clicked();
        };

        // Add to the visual tree
        add_child(std::move(box_a));
        add_child(std::move(box_b));
    }

private:
    std::shared_ptr<MainViewModel> view_model_;
};

// ---------------------------------------------------------
// 3. Application Bootstrapper
// ---------------------------------------------------------
int main() {
    std::cout << "Starting OOEY MVVM-C Example...\n";

    ooey::Application app;

    auto backend = std::make_unique<ooey::X11WindowBackend>();
    if (!backend->create({800, 600}, "OOEY MVVM-C Interaction")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    // Initialize the MVVM stack
    auto view_model = std::make_shared<MainViewModel>();
    auto root_view = std::make_shared<MainView>(view_model);

    // Provide the UI to the engine
    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{30, 30, 30});

    // Run the engine
    app.run();

    return 0;
}