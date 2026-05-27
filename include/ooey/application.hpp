#pragma once

#include <memory>
#include <functional>

#include "ooey/i_window_backend.hpp"
#include "ooey/i_render_target.hpp"
#include "ooey/input.hpp"
#include "ooey/view.hpp"
#include "ooey/i_controller.hpp"
#include "ooey/i_drawable.hpp"

namespace ooey {

class Application {
public:
    Application();
    ~Application();

    // Set the window backend to use
    void set_window_backend(std::unique_ptr<IWindowBackend>&& backend);

    // Get the global input manager
    InputManager& get_input_manager() { return input_manager_; }

    // Set the root view for the scene graph
    void set_root_view(std::shared_ptr<View>&& root_view);

    // Set a custom controller
    void set_controller(std::unique_ptr<IController>&& controller);

    // Get the global controller
    IController* get_controller() { return controller_.get(); }

    // Set the default clear color
    void set_clear_color(Color color);

    // Optional callback executed before the scene graph is rendered
    void set_before_render_callback(std::function<void(IRenderTarget*)>&& callback);

    // Optional callback executed after the scene graph is rendered but before presentation
    void set_after_render_callback(std::function<void(IRenderTarget*)>&& callback);

    // Run the main application loop
    void run();

    // Quit the application
    void quit();

private:
    std::unique_ptr<IWindowBackend> window_backend_;
    std::shared_ptr<View> root_view_;
    std::unique_ptr<IController> controller_;
    Color clear_color_{0, 0, 0, 255};
    std::function<void(IRenderTarget*)> before_render_callback_;
    std::function<void(IRenderTarget*)> after_render_callback_;
    InputManager input_manager_;
    bool running_{false};
};

} // namespace ooey
