#include "ooey/application.hpp"

namespace ooey {

Application::Application() = default;

Application::~Application() {
    if (window_backend_) {
        window_backend_->destroy();
    }
}

void Application::set_window_backend(std::unique_ptr<IWindowBackend> backend) {
    window_backend_ = std::move(backend);
}

void Application::set_render_callback(std::function<void(IRenderTarget*)> callback) {
    render_callback_ = std::move(callback);
}

void Application::run() {
    running_ = true;

    while (running_) {
        if (window_backend_) {
            if (!window_backend_->poll_events()) {
                running_ = false;
            }

            if (running_ && render_callback_) {
                render_callback_(window_backend_->get_render_target());
            }
        } else {
            // Without a window backend, we'll just exit for now to avoid an infinite busy loop.
            // In the future, this could run a headless simulation loop or memory renderer loop.
            running_ = false; 
        }
    }
}

void Application::quit() {
    running_ = false;
}

} // namespace ooey
