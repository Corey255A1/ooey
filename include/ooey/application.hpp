#pragma once

#include <memory>
#include <functional>

#include "ooey/i_window_backend.hpp"
#include "ooey/i_render_target.hpp"

namespace ooey {

class Application {
public:
    Application();
    ~Application();

    // Set the window backend to use
    void set_window_backend(std::unique_ptr<IWindowBackend> backend);

    // Set a callback to execute rendering logic per frame
    void set_render_callback(std::function<void(IRenderTarget*)> callback);

    // Run the main application loop
    void run();

    // Quit the application
    void quit();

private:
    std::unique_ptr<IWindowBackend> window_backend_;
    std::function<void(IRenderTarget*)> render_callback_;
    bool running_{false};
};

} // namespace ooey
