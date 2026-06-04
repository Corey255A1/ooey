namespace ooey {}

#include "gooey/application.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/logging.hpp"
#include "ooey/renderer/window_chrome.hpp"
#include "ooey/renderer/scaled_render_target.hpp"
#include "ooey/renderer/font_engine.hpp"
#include "ooey/renderer/glyph_atlas.hpp"
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>

namespace gooey {
    using namespace ooey;

Application* Application::instance_ = nullptr;

Application* Application::get_instance() {
    return instance_;
}

Application::Application() {
    instance_ = this;
    OOEY_LOG_DEBUG("Application", "Application constructed");
}

Application::~Application() {
    if (instance_ == this) {
        instance_ = nullptr;
    }
    OOEY_LOG_DEBUG("Application", "Destroying application");
    if (window_backend_) {
        window_backend_->destroy();
        OOEY_LOG_INFO("Application", "Window backend destroyed");
    }
    // Clean up global caches to prevent leaks on exit
    ooey::GlyphAtlasManager::clear();
    ooey::FontEngine::set_backend(nullptr);
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

void Application::set_root_view(std::shared_ptr<mvvmc::GooeyNode>&& root_view) {
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

void Application::set_theme_manager(const std::shared_ptr<mvvmc::ThemeManager>& manager) {
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

    std::vector<std::function<void()>> local_tasks;
    {
        std::lock_guard<std::mutex> lock(dispatcher_mutex_);
        local_tasks = std::move(dispatcher_tasks_);
    }
    for (auto& task : local_tasks) {
        if (task) {
            task();
        }
    }

    if (window_backend_) {
        float scale = get_dpi_scale();
        input_manager_.set_scale(scale);

        if (!window_backend_->poll_events(next_poll_timeout_ms_)) {
            OOEY_LOG_INFO("Application", "Poll events returned false, shutting down");
            running_ = false;
        }

        if (running_) {
            if (controller_) {
                controller_->process_events();
            }

            bool had_input = !input_manager_.get_pointer_events().empty() ||
                             !input_manager_.get_key_events().empty() ||
                             !input_manager_.get_text_events().empty();

            input_manager_.update(); // clear transient states

            auto* target = window_backend_->get_render_target();
            if (target) {
                Size physical_size = window_backend_->get_size();
                Size size{
                    static_cast<int>(physical_size.width / scale),
                    static_cast<int>(physical_size.height / scale)
                };
                if (auto chrome = window_backend_->get_window_chrome()) {
                    size.width -= static_cast<int>((2 * chrome->get_border_width()) / scale);
                    size.height -= static_cast<int>((2 * chrome->get_border_width() + chrome->get_title_bar_height()) / scale);
                }

                static Size last_size{0, 0};
                bool size_changed = (size.width != last_size.width || size.height != last_size.height);
                last_size = size;
                if (size_changed) {
                    last_resize_time_ = std::chrono::steady_clock::now();
                }

                bool layout_dirty = root_view_ && (!root_view_->is_layout_clean() || !root_view_->is_measure_clean());
                bool should_render = needs_render_ || size_changed || layout_dirty || had_input;

                if (before_render_callback_) {
                    before_render_callback_(target);
                }

                if (root_view_ && (!root_view_->is_layout_clean() || !root_view_->is_measure_clean())) {
                    should_render = true;
                }

                if (should_render) {
                    target->clear(clear_color_);

                    if (root_view_) {
                        root_view_->measure(size);
                        root_view_->layout(Rect{0, 0, size.width, size.height});
                        
                        ScaledRenderTarget scaled_target(target, scale);
                        root_view_->draw(scaled_target);
                    }

                    if (after_render_callback_) {
                        after_render_callback_(target);
                    }

                    target->present();
                    
                    frame_count_++;
                    if (frame_count_ % 300 == 0) {
                        OOEY_LOG_DEBUG("Application", "Rendered " << frame_count_ << " frames");
                    }
                    needs_render_ = false;
                } else {
#ifndef __EMSCRIPTEN__
                    if (next_poll_timeout_ms_ == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
#endif
                }
            }
        }

        // Determine timeout for the next iteration's poll_events
        bool has_dispatched_tasks = false;
        {
            std::lock_guard<std::mutex> lock(dispatcher_mutex_);
            has_dispatched_tasks = !dispatcher_tasks_.empty();
        }

        if (needs_render_ ||
            before_render_callback_ ||
            has_dispatched_tasks ||
            is_user_interacting()) {
            next_poll_timeout_ms_ = 0;
        } else {
            next_poll_timeout_ms_ = 100;
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

float Application::get_dpi_scale() const {
    if (!dpi_scale_enabled_) {
        return 1.0f;
    }

    // 1. Check environment variable override
    const char* scale_env = std::getenv("OOEY_SCALE");
    if (scale_env) {
        try {
            float env_scale = std::stof(scale_env);
            if (env_scale > 0.0f) {
                return env_scale;
            }
        } catch (...) {
            std::string s(scale_env);
            if (s == "false" || s == "off" || s == "disabled" || s == "0") {
                return 1.0f;
            }
        }
    }

    // 2. Query window backend if available
    if (window_backend_) {
        return window_backend_->get_content_scale();
    }

    return 1.0f;
}

bool Application::is_user_interacting() const {
    if (!input_manager_.get_active_pointers().empty()) {
        return true;
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - last_resize_time_).count();
    if (elapsed < 1.5f) {
        return true;
    }
    return false;
}

void Application::dispatch(std::function<void()>&& task) {
    {
        std::lock_guard<std::mutex> lock(dispatcher_mutex_);
        dispatcher_tasks_.push_back(std::move(task));
    }
    request_render();
}

void request_render() {
    if (Application::get_instance()) {
        Application::get_instance()->request_render();
    }
}

} // namespace gooey

