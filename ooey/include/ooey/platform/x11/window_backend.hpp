#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>

// Forward declarations to avoid including Xlib.h in the header
typedef struct _XDisplay Display;

namespace ooey {
class WindowChrome;
class ChromeRenderTarget;
}

namespace ooey::x11 {

class WindowBackend : public IWindowBackend {
public:
    WindowBackend();
    ~WindowBackend() override;

    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    void poll_input() override;
    IRenderTarget* get_render_target() override;

    void set_input_manager(InputManager* manager) override { input_manager_ = manager; }

    // Window chrome interface
    void set_window_chrome(std::shared_ptr<WindowChrome> chrome) override;
    std::shared_ptr<WindowChrome> get_window_chrome() const override { return window_chrome_; }
    void start_interactive_move() override;
    void start_interactive_resize(WindowResizeEdge edge) override;
    void request_close() override { should_close_ = true; }

private:
    Display* display_{nullptr};
    unsigned long window_{0}; // Window
    void* glc_{nullptr}; // GLXContext
    unsigned long wm_delete_window_{0}; // Atom
    std::unique_ptr<IRenderTarget> render_target_;
    InputManager* input_manager_{nullptr};

    std::shared_ptr<ooey::WindowChrome> window_chrome_;
    std::unique_ptr<ooey::ChromeRenderTarget> decorated_render_target_;
    int width_{0};
    int height_{0};
    bool should_close_{false};
};

} // namespace ooey::x11
