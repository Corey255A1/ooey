#include "ooey/application.hpp"
#include "ooey/controller.hpp"
#include "ooey/logging.hpp"

namespace ooey {

Application::Application() {
    OOEY_LOG_DEBUG("Application", "Application constructed");
}

Application::~Application() {
    OOEY_LOG_DEBUG("Application", "Destroying application");
    if (window_backend_) {
        window_backend_->destroy();
        OOEY_LOG_INFO("Application", "Window backend destroyed");
    }
}

void Application::set_window_backend(std::unique_ptr<IWindowBackend>&& backend) {
    window_backend_ = std::move(backend);
    OOEY_LOG_INFO("Application", "Window backend set");
    
    // Attempt to configure the backend to feed our input manager
    auto* input_provider = dynamic_cast<IInputProvider*>(window_backend_.get());
    if (input_provider) {
        input_provider->set_input_manager(&input_manager_);
        OOEY_LOG_DEBUG("Application", "Input provider configured");
    }
}

void Application::set_root_view(std::shared_ptr<View>&& root_view) {
    root_view_ = std::move(root_view);
    controller_ = std::make_unique<Controller>(input_manager_, root_view_);
    OOEY_LOG_INFO("Application", "Root view and controller initialized");
}

void Application::set_controller(std::unique_ptr<IController>&& controller) {
    controller_ = std::move(controller);
    OOEY_LOG_INFO("Application", "Custom controller set");
}

void Application::set_clear_color(Color color) {
    clear_color_ = color;
    OOEY_LOG_DEBUG("Application", "Clear color set to RGBA(" << static_cast<int>(color.r) << ", " 
                                   << static_cast<int>(color.g) << ", " << static_cast<int>(color.b) << ", " 
                                   << static_cast<int>(color.a) << ")");
}

void Application::set_before_render_callback(std::function<void(IRenderTarget*)>&& callback) {
    before_render_callback_ = std::move(callback);
    OOEY_LOG_DEBUG("Application", "Before render callback set");
}

void Application::set_after_render_callback(std::function<void(IRenderTarget*)>&& callback) {
    after_render_callback_ = std::move(callback);
    OOEY_LOG_DEBUG("Application", "After render callback set");
}

void Application::run() {
    OOEY_LOG_INFO("Application", "Starting application run loop");
    running_ = true;
    
    int frame_count = 0;

    while (running_) {
        if (window_backend_) {
            if (!window_backend_->poll_events()) {
                OOEY_LOG_INFO("Application", "Poll events returned false, shutting down");
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
                        root_view_->draw(*target);
                    }

                    if (after_render_callback_) {
                        after_render_callback_(target);
                    }

                    target->present();
                    
                    frame_count++;
                    if (frame_count % 300 == 0) {
                        OOEY_LOG_DEBUG("Application", "Rendered " << frame_count << " frames");
                    }
                }
            }
        } else {
            // Without a window backend, we'll just exit for now to avoid an infinite busy loop.
            // In the future, this could run a headless simulation loop or memory renderer loop.
            OOEY_LOG_WARNING("Application", "No window backend available, exiting run loop");
            running_ = false; 
        }
    }
    
    OOEY_LOG_INFO("Application", "Application run loop ended after " << frame_count << " frames");
}

void Application::quit() {
    OOEY_LOG_INFO("Application", "Quit requested");
    running_ = false;
}

} // namespace ooey
