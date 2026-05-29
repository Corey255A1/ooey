#pragma once

#include "ooey/i_window_backend.hpp"
#include <memory>
#include <string>
#include <vector>
#include <linux/fb.h>

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
    int get_logical_width() const { return logical_w_; }
    int get_logical_height() const { return logical_h_; }

    void set_input_manager(InputManager* manager) override { input_manager_ = manager; }

private:
    int fd_{-1};
    int rotation_{0};
    std::string device_path_;
    std::unique_ptr<IRenderTarget> render_target_;
    InputManager* input_manager_{nullptr};

    struct fb_var_screeninfo vinfo_{};
    struct fb_fix_screeninfo finfo_{};
    uint8_t* fb_mem_{nullptr};
    size_t fb_mem_size_{0};
    std::vector<uint8_t> logical_backbuffer_;

    int phys_w_{0};
    int phys_h_{0};
    int logical_w_{0};
    int logical_h_{0};

    void map_coords(int lx, int ly, int& px, int& py) const;
    void present_framebuffer();
};

} // namespace ooey::framebuffer
