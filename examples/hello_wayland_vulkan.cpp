#include <iostream>
#include <memory>
#include <string>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform/wayland/vulkan_window_backend.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/mvvmc/property.hpp"

class VulkanViewModel {
public:
    gooey::Property<ooey::Color> button_color{ooey::Color{0, 100, 200}};
    gooey::Property<std::string> status_text{"Vulkan Backend Ready"};

    void on_button_clicked() {
        if (button_color.get().g == 100) {
            button_color.set(ooey::Color{0, 200, 100});
            status_text.set("Running on Vulkan - State A");
        } else {
            button_color.set(ooey::Color{0, 100, 200});
            status_text.set("Running on Vulkan - State B");
        }
    }
};

class VulkanView : public gooey::View {
public:
    VulkanView(std::shared_ptr<VulkanViewModel> view_model) : view_model_(std::move(view_model)) {
        ooey::Font default_font{"sans-serif", 16};

        auto button = std::make_shared<gooey::Button>(
            ooey::Rect{50, 50, 300, 50},
            ooey::Color{0, 0, 0}
        );

        auto label = std::make_shared<gooey::Label>(
            "Vulkan Status",
            default_font,
            ooey::Point{50, 130},
            ooey::Color{220, 220, 220}
        );

        bind(view_model_->button_color, [button](const ooey::Color& c) {
            button->set_color(c);
        });

        bind(view_model_->status_text, [label](const std::string& text) {
            label->set_text(text);
        });

        auto vm = view_model_;
        button->on_click = [vm]() {
            vm->on_button_clicked();
        };

        add_child(std::move(button));
        add_child(std::move(label));
    }

private:
    std::shared_ptr<VulkanViewModel> view_model_;
};

int main() {
    std::cout << "Starting OOEY Wayland Vulkan Example...\n";

    try {
        gooey::Application app;

        auto backend = std::make_unique<ooey::wayland::VulkanWindowBackend>();
        if (!backend->create({400, 220}, "OOEY Wayland Vulkan Demo")) {
            std::cerr << "Failed to create Vulkan Wayland window\n";
            return 1;
        }

        app.set_window_backend(std::move(backend));

        auto view_model = std::make_shared<VulkanViewModel>();
        auto root_view = std::make_shared<VulkanView>(view_model);

        app.set_root_view(std::move(root_view));
        app.set_clear_color(ooey::Color{20, 20, 20});

        app.run();
    } catch (const std::exception& e) {
        std::cerr << "OOEY Wayland Vulkan Crash: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "OOEY Wayland Vulkan Crash: Unknown exception occurred\n";
        return 1;
    }
    return 0;
}
