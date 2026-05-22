#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>

// Forward declarations to avoid including Xlib.h in the header
typedef struct _XDisplay Display;

namespace ooey {

class X11WindowBackend : public IWindowBackend {
public:
    X11WindowBackend();
    ~X11WindowBackend() override;

    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    IRenderTarget* get_render_target() override;

private:
    Display* display_{nullptr};
    unsigned long window_{0}; // Window
    void* glc_{nullptr}; // GLXContext
    unsigned long wm_delete_window_{0}; // Atom
    std::unique_ptr<IRenderTarget> render_target_;
};

} // namespace ooey