#include "ooey/platform/framebuffer/window_backend.hpp"
#include "ooey/renderer/software_render_target.hpp"
#include "ooey/renderer/window_chrome.hpp"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <cmath>
#include <algorithm>
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

    struct stat st;
    bool is_reg = false;
    if (fstat(fd_, &st) == 0 && S_ISREG(st.st_mode)) {
        is_reg = true;
    }

    if (is_reg) {
        vinfo_.xres = 480;
        vinfo_.yres = 800;
        vinfo_.bits_per_pixel = 32;
        vinfo_.red.offset = 16;
        vinfo_.red.length = 8;
        vinfo_.green.offset = 8;
        vinfo_.green.length = 8;
        vinfo_.blue.offset = 0;
        vinfo_.blue.length = 8;
        vinfo_.transp.offset = 24;
        vinfo_.transp.length = 8;

        finfo_.line_length = 480 * 4;
    } else {
        if (ioctl(fd_, FBIOGET_VSCREENINFO, &vinfo_) < 0) {
            std::cerr << "Framebuffer error: FBIOGET_VSCREENINFO failed\n";
            close(fd_);
            fd_ = -1;
            return false;
        }

        if (ioctl(fd_, FBIOGET_FSCREENINFO, &finfo_) < 0) {
            std::cerr << "Framebuffer error: FBIOGET_FSCREENINFO failed\n";
            close(fd_);
            fd_ = -1;
            return false;
        }
    }

    phys_w_ = static_cast<int>(vinfo_.xres);
    phys_h_ = static_cast<int>(vinfo_.yres);

    if (rotation_ == 90 || rotation_ == 270) {
        logical_w_ = phys_h_;
        logical_h_ = phys_w_;
    } else {
        logical_w_ = phys_w_;
        logical_h_ = phys_h_;
    }

    fb_mem_size_ = finfo_.line_length * vinfo_.yres;
    fb_mem_ = static_cast<uint8_t*>(mmap(nullptr, fb_mem_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (fb_mem_ == MAP_FAILED) {
        std::cerr << "Framebuffer error: mmap failed\n";
        fb_mem_ = nullptr;
        close(fd_);
        fd_ = -1;
        return false;
    }

    logical_backbuffer_.resize(logical_w_ * logical_h_ * 4, 0);

    render_target_ = std::make_unique<SoftwareRenderTarget>(logical_backbuffer_.data(), logical_w_, logical_h_, logical_w_ * 4, [this]() {
        present_framebuffer();
    });

    if (window_chrome_) {
        decorated_render_target_ = std::make_unique<ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{logical_w_, logical_h_});
    }

    std::cout << "Framebuffer initialized: physical " << phys_w_ << "x" << phys_h_
              << ", logical " << logical_w_ << "x" << logical_h_
              << ", bpp " << vinfo_.bits_per_pixel
              << ", rotation " << rotation_ << "\n";

    return true;
}

void WindowBackend::destroy() {
    render_target_.reset();
    if (fb_mem_ && fb_mem_ != MAP_FAILED) {
        munmap(fb_mem_, fb_mem_size_);
        fb_mem_ = nullptr;
    }
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool WindowBackend::poll_events() {
    if (should_close_) {
        return false;
    }
    return true;
}

void WindowBackend::poll_input() {
    // Stub: Framebuffer-only has no input provider implemented.
}

IRenderTarget* WindowBackend::get_render_target() {
    if (decorated_render_target_) {
        return decorated_render_target_.get();
    }
    return render_target_.get();
}

void WindowBackend::set_window_chrome(std::shared_ptr<WindowChrome> chrome) {
    window_chrome_ = chrome;
    if (window_chrome_ && render_target_) {
        decorated_render_target_ = std::make_unique<ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{logical_w_, logical_h_});
    } else {
        decorated_render_target_.reset();
    }
}

void WindowBackend::map_coords(int lx, int ly, int& px, int& py) const {
    switch (rotation_) {
        case 90:
            px = phys_w_ - 1 - ly;
            py = lx;
            break;
        case 180:
            px = phys_w_ - 1 - lx;
            py = phys_h_ - 1 - ly;
            break;
        case 270:
            px = ly;
            py = phys_h_ - 1 - lx;
            break;
        case 0:
        default:
            px = lx;
            py = ly;
            break;
    }
}

void WindowBackend::present_framebuffer() {
    if (!fb_mem_ || logical_backbuffer_.empty()) {
        return;
    }

    for (int ly = 0; ly < logical_h_; ++ly) {
        const uint32_t* src_row = reinterpret_cast<const uint32_t*>(logical_backbuffer_.data() + ly * logical_w_ * 4);
        for (int lx = 0; lx < logical_w_; ++lx) {
            uint32_t pixel = src_row[lx]; // ARGB format

            int px = 0;
            int py = 0;
            map_coords(lx, ly, px, py);

            if (px < 0 || px >= phys_w_ || py < 0 || py >= phys_h_) {
                continue;
            }

            size_t offset = py * finfo_.line_length + px * (vinfo_.bits_per_pixel / 8);

            if (vinfo_.bits_per_pixel == 32) {
                uint8_t a = (pixel >> 24) & 0xFF;
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;

                uint32_t pr = (r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
                uint32_t pg = (g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
                uint32_t pb = (b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
                uint32_t pa = 0;
                if (vinfo_.transp.length > 0) {
                    pa = (a >> (8 - vinfo_.transp.length)) << vinfo_.transp.offset;
                }
                *reinterpret_cast<uint32_t*>(fb_mem_ + offset) = pr | pg | pb | pa;
            } else if (vinfo_.bits_per_pixel == 16) {
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;

                uint32_t pr = (r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
                uint32_t pg = (g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
                uint32_t pb = (b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
                *reinterpret_cast<uint16_t*>(fb_mem_ + offset) = static_cast<uint16_t>(pr | pg | pb);
            } else if (vinfo_.bits_per_pixel == 24) {
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;

                uint32_t pr = (r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
                uint32_t pg = (g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
                uint32_t pb = (b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
                uint32_t p = pr | pg | pb;
                uint8_t* ptr = fb_mem_ + offset;
                ptr[0] = p & 0xFF;
                ptr[1] = (p >> 8) & 0xFF;
                ptr[2] = (p >> 16) & 0xFF;
            }
        }
    }
}

} // namespace ooey::framebuffer
