namespace ooey {}

#include "gooey/application.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/logging.hpp"
#include "ooey/renderer/window_chrome.hpp"

namespace gooey {
    using namespace ooey;

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

void Application::set_root_view(std::shared_ptr<mvvmc::View>&& root_view) {
    root_view_ = std::move(root_view);
    controller_ = std::make_unique<mvvmc::Controller>(input_manager_, root_view_);
    if (root_view_ && theme_manager_) {
        root_view_->set_theme_manager(theme_manager_);
    }
    OOEY_LOG_INFO("Application", "Root view and controller initialized");
}

void Application::set_controller(std::unique_ptr<mvvmc::IController>&& controller) {
    controller_ = std::move(controller);
    OOEY_LOG_INFO("Application", "Custom controller set");
}

void Application::set_theme_manager(std::shared_ptr<mvvmc::ThemeManager> manager) {
    theme_manager_ = manager;
    if (manager) {
        theme_subscription_ = manager->active_theme.subscribe([this](const std::shared_ptr<mvvmc::Theme>& theme) {
            if (theme) {
                mvvmc::Style window_style;
                if (theme->get_style("window", window_style)) {
                    set_clear_color(window_style.fill_color);
                }
            }
        });
        if (root_view_) {
            root_view_->set_theme_manager(manager);
        }
    } else {
        theme_subscription_ = {};
    }
}

void Application::set_clear_color(Color color) {
    clear_color_ = color;
    OOEY_LOG_DEBUG("Application", "Clear color set to RGBA(" << static_cast<int>(color.r) << ", " 
                                   << static_cast<int>(color.g) << ", " << static_cast<int>(color.b) << ", " 
                                   << static_cast<int>(color.a) << ")");
}

void Application::set_before_render_callback(std::function<void(ooey::IRenderTarget*)>&& callback) {
    before_render_callback_ = std::move(callback);
    OOEY_LOG_DEBUG("Application", "Before render callback set");
}

void Application::set_after_render_callback(std::function<void(ooey::IRenderTarget*)>&& callback) {
    after_render_callback_ = std::move(callback);
    OOEY_LOG_DEBUG("Application", "After render callback set");
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

void Application::run() {
    OOEY_LOG_INFO("Application", "Starting application run loop");
    running_ = true;
    frame_count_ = 0;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* arg) {
        static_cast<Application*>(arg)->run_iteration();
    }, this, 0, 1);
#else
    while (running_) {
        run_iteration();
    }
    OOEY_LOG_INFO("Application", "Application run loop ended after " << frame_count_ << " frames");
#endif
}

void Application::run_iteration() {
    if (!running_) {
        return;
    }

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
                    Size size = window_backend_->get_size();
                    if (auto chrome = window_backend_->get_window_chrome()) {
                        size.width -= 2 * chrome->get_border_width();
                        size.height -= (2 * chrome->get_border_width() + chrome->get_title_bar_height());
                    }
                    root_view_->measure(size);
                    root_view_->layout(Rect{0, 0, size.width, size.height});
                    root_view_->draw(*target);
                }

                if (after_render_callback_) {
                    after_render_callback_(target);
                }

                target->present();
                
                frame_count_++;
                if (frame_count_ % 300 == 0) {
                    OOEY_LOG_DEBUG("Application", "Rendered " << frame_count_ << " frames");
                }
            }
        }
    } else {
        // Without a window backend, we'll just exit for now to avoid an infinite busy loop.
        OOEY_LOG_WARNING("Application", "No window backend available, exiting run loop");
        running_ = false; 
    }
}

void Application::quit() {
    OOEY_LOG_INFO("Application", "Quit requested");
    running_ = false;
}

} // namespace gooey
