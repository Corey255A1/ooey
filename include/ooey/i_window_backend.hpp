#pragma once

#include "ooey/types.hpp"

namespace ooey {

class IRenderTarget;

class IWindowBackend {
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
    virtual IRenderTarget* get_render_target() = 0;
};

} // namespace ooey
