#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>

namespace ooey::emscripten {

class WindowBackend : public IWindowBackend {
public:
    WindowBackend();
    ~WindowBackend() override;

    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    void poll_input() override;
    renderer::IRenderTarget* get_render_target() override;

    void set_input_manager(InputManager* manager) override { input_manager_ = manager; }

private:
    std::unique_ptr<renderer::IRenderTarget> render_target_;
    InputManager* input_manager_{nullptr};
};

} // namespace ooey::emscripten
