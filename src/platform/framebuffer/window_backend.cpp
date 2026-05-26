#include "ooey/platform/framebuffer/window_backend.hpp"
#include "ooey/platform/framebuffer/render_target.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

namespace ooey::framebuffer {

WindowBackend::WindowBackend() {
    const char* rot_env = std::getenv("OOEY_FB_ROTATION");
    if (rot_env) {
        rotation_ = std::atoi(rot_env);
    }
    if (rotation_ != 0 && rotation_ != 90 && rotation_ != 180 && rotation_ != 270) {
        std::cerr << "Warning: Invalid OOEY_FB_ROTATION=" << rotation_ << ", defaulting to 0\n";
        rotation_ = 0;
    }

    const char* dev_env = std::getenv("OOEY_FB_DEVICE");
    device_path_ = dev_env ? dev_env : "/dev/fb0";
}

WindowBackend::WindowBackend(int rotation, const std::string& device_path)
    : rotation_(rotation), device_path_(device_path) {
    if (rotation_ != 0 && rotation_ != 90 && rotation_ != 180 && rotation_ != 270) {
        std::cerr << "Warning: Invalid rotation=" << rotation_ << ", defaulting to 0\n";
        rotation_ = 0;
    }
}

WindowBackend::~WindowBackend() {
    destroy();
}

bool WindowBackend::create(const Size& /*size*/, const char* /*title*/) {
    fd_ = open(device_path_.c_str(), O_RDWR);
    if (fd_ < 0) {
        std::cerr << "Framebuffer error: Failed to open " << device_path_ << "\n";
        return false;
    }

    auto target = std::make_unique<RenderTarget>(fd_, rotation_);
    if (!target->initialize()) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    render_target_ = std::move(target);
    return true;
}

void WindowBackend::destroy() {
    render_target_.reset();
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool WindowBackend::poll_events() {
    return true;
}

void WindowBackend::poll_input() {
    // Stub: Framebuffer-only has no input provider implemented.
}

IRenderTarget* WindowBackend::get_render_target() {
    return render_target_.get();
}

} // namespace ooey::framebuffer
