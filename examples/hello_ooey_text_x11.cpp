#include <iostream>
#include <memory>
#include <string>
#include "ooey/ooey.hpp"
#include "ooey/application.hpp"
#include "ooey/platform/x11/window_backend.hpp"
#include "ooey/view.hpp"
#include "ooey/controls/text_box.hpp"
#include "ooey/controls/label.hpp"
#include "ooey/mvvmc/property.hpp"

class TextViewModel {
public:
    ooey::Property<std::string> user_text{"Type here..."};

    void on_text_input(const std::string& new_text) {
        user_text.set(new_text);
    }
};

class TextView : public ooey::View {
public:
    TextView(std::shared_ptr<TextViewModel> view_model) : view_model_(std::move(view_model)) {
        ooey::Font default_font{"sans-serif", 16};

        auto text_box = std::make_shared<ooey::TextBox>(
            ooey::Rect{50, 50, 300, 30},
            default_font,
            ooey::Color{0, 0, 0},
            ooey::Color{255, 255, 255}
        );

        auto label = std::make_shared<ooey::Label>(
            "Will reflect text here",
            default_font,
            ooey::Point{50, 100},
            ooey::Color{200, 200, 200}
        );

        bind(view_model_->user_text, [label, text_box](const std::string& text) {
            label->set_text("You typed: " + text);
            text_box->set_text(text);
        });

        auto vm = view_model_;
        text_box->on_text_changed = [vm](const std::string& text) {
            vm->on_text_input(text);
        };

        text_box->set_text(view_model_->user_text.get());

        add_child(std::move(text_box));
        add_child(std::move(label));
    }

private:
    std::shared_ptr<TextViewModel> view_model_;
};

int main() {
    std::cout << "Starting OOEY X11 Text Example...\n";

    ooey::Application app;

    auto backend = std::make_unique<ooey::x11::WindowBackend>();
    if (!backend->create({400, 200}, "OOEY X11 Text Example")) {
        std::cerr << "Failed to create X11 window\n";
        return 1;
    }

    app.set_window_backend(std::move(backend));

    auto view_model = std::make_shared<TextViewModel>();
    auto root_view = std::make_shared<TextView>(view_model);

    app.set_root_view(std::move(root_view));
    app.set_clear_color(ooey::Color{40, 40, 40});

    app.run();
    return 0;
}
