#pragma once

#include "ooey/types.hpp"
#include "ooey/input.hpp"
#include <memory>

namespace ooey {

namespace renderer {
class IRenderTarget;
}
using renderer::IRenderTarget;

class WindowChrome;

class IWindowBackend : public IInputProvider {
public:
    virtual ~IWindowBackend() = default;

    // Initialize window
    virtual bool create(const Size& size, const char* title) = 0;

    // Destroy the window
    virtual void destroy() = 0;

    // Poll for events (keyboard, mouse, window close, etc.)
    // Returns true if the application should keep running, false if it should exit
    virtual bool poll_events() = 0;

    // Get the render target associated with this window
    virtual renderer::IRenderTarget* get_render_target() = 0;

    // Window chrome interface
    virtual void set_window_chrome(std::shared_ptr<WindowChrome> chrome) = 0;
    virtual std::shared_ptr<WindowChrome> get_window_chrome() const = 0;
    virtual void start_interactive_move() = 0;
    virtual void start_interactive_resize(WindowResizeEdge edge) = 0;
    virtual void request_close() = 0;

    // Get the window size
    virtual Size get_size() const = 0;
};

} // namespace ooey

