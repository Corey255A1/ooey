#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>
#include <string>

namespace ooey::framebuffer {

class WindowBackend : public IWindowBackend {
public:
    WindowBackend();
    explicit WindowBackend(int rotation, const std::string& device_path = "/dev/fb0");
    ~WindowBackend() override;

    bool create(const Size& size, const char* title) override;
    void destroy() override;
    bool poll_events() override;
    void poll_input() override;
    IRenderTarget* get_render_target() override;

    void set_input_manager(InputManager* manager) override { input_manager_ = manager; }

private:
    int fd_{-1};
    int rotation_{0};
    std::string device_path_;
    std::unique_ptr<IRenderTarget> render_target_;
    InputManager* input_manager_{nullptr};
};

} // namespace ooey::framebuffer
