#include "ooey/application.hpp"

namespace ooey {

Application::Application() = default;

Application::~Application() {
    if (window_backend_) {
        window_backend_->destroy();
    }
}

void Application::set_window_backend(std::unique_ptr<IWindowBackend>&& backend) {
    window_backend_ = std::move(backend);
    
    // Attempt to configure the backend to feed our input manager
    auto* input_provider = dynamic_cast<IInputProvider*>(window_backend_.get());
    if (input_provider) {
        input_provider->set_input_manager(&input_manager_);
    }
}

void Application::set_root_view(std::shared_ptr<View>&& root_view) {
    root_view_ = std::move(root_view);
    controller_ = std::make_unique<Controller>(input_manager_, root_view_);
}

void Application::set_clear_color(Color color) {
    clear_color_ = color;
}

void Application::set_before_render_callback(std::function<void(IRenderTarget*)>&& callback) {
    before_render_callback_ = std::move(callback);
}

void Application::set_after_render_callback(std::function<void(IRenderTarget*)>&& callback) {
    after_render_callback_ = std::move(callback);
}

void Application::run() {
    running_ = true;

    while (running_) {
        if (window_backend_) {
            if (!window_backend_->poll_events()) {
                running_ = false;
            }

            if (running_) {
                if (controller_) {
                    controller_->process_events();
                }

                input_manager_.update(); // clear transient states

                auto* target = window_backend_->get_render_target();
                if (target) {
                    if (before_render_callback_) {
                        before_render_callback_(target);
                    }

                    target->clear(clear_color_);

                    if (root_view_) {
                        auto geometries = root_view_->generate_geometry();
                        for (const auto& geo : geometries) {
                            target->draw_geometry(geo);
                        }
                    }

                    if (after_render_callback_) {
                        after_render_callback_(target);
                    }

                    target->present();
                }
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
