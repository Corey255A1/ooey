#include <iostream>
#include <memory>
#include <string>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/platform.hpp"
#include "ooey/view.hpp"
#include "ooey/controls/text_box.hpp"
#include "ooey/controls/label.hpp"
#include "ooey/mvvmc/property.hpp"

// ---------------------------------------------------------
// 1. The ViewModel (Logic & State)
// ---------------------------------------------------------
class TextViewModel {
public:
    // Property that the View will observe
    ooey::Property<std::string> user_text{"Type here..."};

    void on_text_input(const std::string& new_text) {
        // We can add validation or business logic here
        user_text.set(new_text);
    }
};

// ---------------------------------------------------------
// 2. The View (UI Construction & Binding)
// ---------------------------------------------------------
class TextView : public ooey::View {
public:
    TextView(std::shared_ptr<TextViewModel> view_model) : view_model_(std::move(view_model)) {
        
        // Construct visual elements
        ooey::Font default_font{"sans-serif", 16};
        
        // Editable TextBox
        auto text_box = std::make_shared<ooey::TextBox>(
            ooey::Rect{50, 50, 300, 30}, 
            default_font, 
            ooey::Color{0, 0, 0},     // Text color (Black)
            ooey::Color{255, 255, 255} // Background color (White)
        );

        // Display Label
        auto label = std::make_shared<ooey::Label>(
            "Will reflect text here", 
            default_font, 
            ooey::Point{50, 100}, 
            ooey::Color{200, 200, 200} // Text color (Light Grey)
        );

        // -- Data Binding: State (ViewModel) -> UI (View) --
        // Update the Label and TextBox when the view model's property changes.
        // We capture the controls by weak/shared ptr to manipulate them safely.
        bind(view_model_->user_text, [label, text_box](const std::string& text) {
            label->set_text("You typed: " + text);
            // In a more complex setup, you'd ensure you don't infinite loop by 
            // only updating the text_box if the text is actually different.
            // TextBox::set_text already does this check.
            text_box->set_text(text);
        });

        // -- Command Binding: UI (View) -> Action (ViewModel) --
        auto vm = view_model_;
        text_box->on_text_changed = [vm](const std::string& text) {
            vm->on_text_input(text);
        };

        // Initialize state to initial ViewModel state
        text_box->set_text(view_model_->user_text.get());

        // Add to the visual tree
        add_child(std::move(text_box));
        add_child(std::move(label));
    }

private:
    std::shared_ptr<TextViewModel> view_model_;
};

// ---------------------------------------------------------
// 3. Application Bootstrapper
// ---------------------------------------------------------
int main() {
    std::cout << "Starting OOEY Text Example...\n";

    ooey::Application app;

    auto backend = ooey::create_default_window_backend();
    if (!backend || !backend->create({400, 200}, "OOEY Text & Binding")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    // Initialize the MVVM stack
    auto view_model = std::make_shared<TextViewModel>();
    auto root_view = std::make_shared<TextView>(view_model);

    // Provide the UI to the engine
    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{40, 40, 40});

    // Run the engine
    app.run();

    return 0;
}