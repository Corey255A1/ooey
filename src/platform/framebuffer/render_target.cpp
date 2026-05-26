#include "ooey/platform/framebuffer/render_target.hpp"
#include "ooey/bitmap_font.hpp"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace ooey::framebuffer {

RenderTarget::RenderTarget(int fd, int rotation)
    : fd_(fd), rotation_(rotation) {}

RenderTarget::~RenderTarget() {
    if (fb_mem_ && fb_mem_ != MAP_FAILED) {
        munmap(fb_mem_, fb_mem_size_);
    }
}

bool RenderTarget::initialize() {
    if (fd_ < 0) {
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
            return false;
        }

        if (ioctl(fd_, FBIOGET_FSCREENINFO, &finfo_) < 0) {
            std::cerr << "Framebuffer error: FBIOGET_FSCREENINFO failed\n";
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
        return false;
    }

    backbuffer_.resize(fb_mem_size_, 0);

    std::cout << "Framebuffer initialized: physical " << phys_w_ << "x" << phys_h_
              << ", logical " << logical_w_ << "x" << logical_h_
              << ", bpp " << vinfo_.bits_per_pixel
              << ", rotation " << rotation_ << "\n";

    return true;
}

void RenderTarget::clear(Color color) {
    if (backbuffer_.empty()) {
        return;
    }

    if (vinfo_.bits_per_pixel == 32) {
        uint32_t r = (color.r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
        uint32_t g = (color.g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
        uint32_t b = (color.b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
        uint32_t a = 0;
        if (vinfo_.transp.length > 0) {
            a = (color.a >> (8 - vinfo_.transp.length)) << vinfo_.transp.offset;
        }
        uint32_t pixel = r | g | b | a;

        for (int y = 0; y < phys_h_; ++y) {
            uint32_t* row = reinterpret_cast<uint32_t*>(backbuffer_.data() + y * finfo_.line_length);
            for (int x = 0; x < phys_w_; ++x) {
                row[x] = pixel;
            }
        }
    } else if (vinfo_.bits_per_pixel == 16) {
        uint32_t r = (color.r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
        uint32_t g = (color.g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
        uint32_t b = (color.b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
        uint16_t pixel = static_cast<uint16_t>(r | g | b);

        for (int y = 0; y < phys_h_; ++y) {
            uint16_t* row = reinterpret_cast<uint16_t*>(backbuffer_.data() + y * finfo_.line_length);
            for (int x = 0; x < phys_w_; ++x) {
                row[x] = pixel;
            }
        }
    } else {
        for (int y = 0; y < logical_h_; ++y) {
            for (int x = 0; x < logical_w_; ++x) {
                draw_pixel(x, y, color);
            }
        }
    }
}

void RenderTarget::draw_geometry(const Geometry& geometry) {
    if (geometry.vertices.empty()) {
        return;
    }

    if (geometry.type == PrimitiveType::Triangles) {
        if (!geometry.indices.empty()) {
            for (size_t i = 0; i + 2 < geometry.indices.size(); i += 3) {
                unsigned int i0 = geometry.indices[i];
                unsigned int i1 = geometry.indices[i + 1];
                unsigned int i2 = geometry.indices[i + 2];
                if (i0 < geometry.vertices.size() && i1 < geometry.vertices.size() && i2 < geometry.vertices.size()) {
                    draw_triangle(geometry.vertices[i0], geometry.vertices[i1], geometry.vertices[i2], geometry.vertices[i0].color);
                }
            }
        } else {
            for (size_t i = 0; i + 2 < geometry.vertices.size(); i += 3) {
                draw_triangle(geometry.vertices[i], geometry.vertices[i + 1], geometry.vertices[i + 2], geometry.vertices[i].color);
            }
        }
    } else if (geometry.type == PrimitiveType::Lines) {
        if (!geometry.indices.empty()) {
            for (size_t i = 0; i + 1 < geometry.indices.size(); i += 2) {
                unsigned int ia = geometry.indices[i];
                unsigned int ib = geometry.indices[i + 1];
                if (ia < geometry.vertices.size() && ib < geometry.vertices.size()) {
                    const auto& a = geometry.vertices[ia];
                    const auto& b = geometry.vertices[ib];
                    draw_line(static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x), static_cast<int>(b.y), a.color);
                }
            }
        } else {
            for (size_t i = 0; i + 1 < geometry.vertices.size(); i += 2) {
                const auto& a = geometry.vertices[i];
                const auto& b = geometry.vertices[i + 1];
                draw_line(static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x), static_cast<int>(b.y), a.color);
            }
        }
    }
}

Size RenderTarget::measure_text(const std::string& text, const Font& font) {
    return BitmapFont::measure_text(text, font.size);
}

void RenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    BitmapFont::draw_text(text, font.size, position, [this, color](int x, int y, int w, int h) {
        draw_filled_rect(x, y, w, h, color);
    });
}

void RenderTarget::present() {
    if (fb_mem_ && !backbuffer_.empty()) {
        std::memcpy(fb_mem_, backbuffer_.data(), fb_mem_size_);
    }
}

void RenderTarget::map_coords(int lx, int ly, int& px, int& py) const {
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

void RenderTarget::draw_pixel(int lx, int ly, Color color) {
    if (lx < 0 || lx >= logical_w_ || ly < 0 || ly >= logical_h_) {
        return;
    }
    int px = 0;
    int py = 0;
    map_coords(lx, ly, px, py);
    if (px < 0 || px >= phys_w_ || py < 0 || py >= phys_h_) {
        return;
    }

    size_t offset = py * finfo_.line_length + px * (vinfo_.bits_per_pixel / 8);
    if (vinfo_.bits_per_pixel == 32) {
        uint32_t r = (color.r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
        uint32_t g = (color.g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
        uint32_t b = (color.b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
        uint32_t a = 0;
        if (vinfo_.transp.length > 0) {
            a = (color.a >> (8 - vinfo_.transp.length)) << vinfo_.transp.offset;
        }
        uint32_t pixel = r | g | b | a;
        *reinterpret_cast<uint32_t*>(backbuffer_.data() + offset) = pixel;
    } else if (vinfo_.bits_per_pixel == 16) {
        uint32_t r = (color.r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
        uint32_t g = (color.g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
        uint32_t b = (color.b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
        uint16_t pixel = static_cast<uint16_t>(r | g | b);
        *reinterpret_cast<uint16_t*>(backbuffer_.data() + offset) = pixel;
    } else if (vinfo_.bits_per_pixel == 24) {
        uint32_t r = (color.r >> (8 - vinfo_.red.length)) << vinfo_.red.offset;
        uint32_t g = (color.g >> (8 - vinfo_.green.length)) << vinfo_.green.offset;
        uint32_t b = (color.b >> (8 - vinfo_.blue.length)) << vinfo_.blue.offset;
        uint32_t pixel = r | g | b;
        uint8_t* ptr = backbuffer_.data() + offset;
        ptr[0] = pixel & 0xFF;
        ptr[1] = (pixel >> 8) & 0xFF;
        ptr[2] = (pixel >> 16) & 0xFF;
    }
}

void RenderTarget::draw_filled_rect(int x, int y, int w, int h, Color color) {
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            draw_pixel(xx, yy, color);
        }
    }
}

void RenderTarget::draw_line(int start_x, int start_y, int end_x, int end_y, Color color) {
    int delta_x = std::abs(end_x - start_x);
    int step_x = start_x < end_x ? 1 : -1;
    int delta_y = -std::abs(end_y - start_y);
    int step_y = start_y < end_y ? 1 : -1;
    int error = delta_x + delta_y;

    while (true) {
        draw_pixel(start_x, start_y, color);
        if (start_x == end_x && start_y == end_y) {
            break;
        }
        int error2 = 2 * error;
        if (error2 >= delta_y) {
            error += delta_y;
            start_x += step_x;
        }
        if (error2 <= delta_x) {
            error += delta_x;
            start_y += step_y;
        }
    }
}

void RenderTarget::draw_flat_bottom_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color) {
    if (std::abs(v1.y - v0.y) < 1e-5f) {
        return;
    }
    float invslope1 = (v1.x - v0.x) / (v1.y - v0.y);
    float invslope2 = (v2.x - v0.x) / (v2.y - v0.y);

    float curx1 = v0.x;
    float curx2 = v0.x;

    int y_start = static_cast<int>(std::round(v0.y));
    int y_end = static_cast<int>(std::round(v1.y));

    for (int scanline_y = y_start; scanline_y < y_end; scanline_y++) {
        int x1 = static_cast<int>(std::round(curx1));
        int x2 = static_cast<int>(std::round(curx2));
        if (x1 > x2) {
            std::swap(x1, x2);
        }
        for (int x = x1; x <= x2; ++x) {
            draw_pixel(x, scanline_y, color);
        }
        curx1 += invslope1;
        curx2 += invslope2;
    }
}

void RenderTarget::draw_flat_top_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color) {
    if (std::abs(v2.y - v0.y) < 1e-5f) {
        return;
    }
    float invslope1 = (v2.x - v0.x) / (v2.y - v0.y);
    float invslope2 = (v2.x - v1.x) / (v2.y - v1.y);

    float curx1 = v2.x;
    float curx2 = v2.x;

    int y_start = static_cast<int>(std::round(v2.y));
    int y_end = static_cast<int>(std::round(v0.y));

    for (int scanline_y = y_start; scanline_y > y_end; scanline_y--) {
        int x1 = static_cast<int>(std::round(curx1));
        int x2 = static_cast<int>(std::round(curx2));
        if (x1 > x2) {
            std::swap(x1, x2);
        }
        for (int x = x1; x <= x2; ++x) {
            draw_pixel(x, scanline_y, color);
        }
        curx1 -= invslope1;
        curx2 -= invslope2;
    }
}

void RenderTarget::draw_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color) {
    Vertex sorted_v0 = v0;
    Vertex sorted_v1 = v1;
    Vertex sorted_v2 = v2;

    if (sorted_v0.y > sorted_v1.y) {
        std::swap(sorted_v0, sorted_v1);
    }
    if (sorted_v0.y > sorted_v2.y) {
        std::swap(sorted_v0, sorted_v2);
    }
    if (sorted_v1.y > sorted_v2.y) {
        std::swap(sorted_v1, sorted_v2);
    }

    if (std::abs(sorted_v1.y - sorted_v2.y) < 1e-5f) {
        draw_flat_bottom_triangle(sorted_v0, sorted_v1, sorted_v2, color);
    } else if (std::abs(sorted_v0.y - sorted_v1.y) < 1e-5f) {
        draw_flat_top_triangle(sorted_v0, sorted_v1, sorted_v2, color);
    } else {
        Vertex v3;
        v3.y = sorted_v1.y;
        v3.x = sorted_v0.x + ((sorted_v1.y - sorted_v0.y) / (sorted_v2.y - sorted_v0.y)) * (sorted_v2.x - sorted_v0.x);
        v3.color = color;
        draw_flat_bottom_triangle(sorted_v0, sorted_v1, v3, color);
        draw_flat_top_triangle(sorted_v1, v3, sorted_v2, color);
    }
}

} // namespace ooey::framebuffer
