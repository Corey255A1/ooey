#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>

// Forward declarations to avoid including Xlib.h in the header
typedef struct _XDisplay Display;

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

private:
    Display* display_{nullptr};
    unsigned long window_{0}; // Window
    void* glc_{nullptr}; // GLXContext
    unsigned long wm_delete_window_{0}; // Atom
    std::unique_ptr<IRenderTarget> render_target_;
    InputManager* input_manager_{nullptr};
};

} // namespace ooey::x11
