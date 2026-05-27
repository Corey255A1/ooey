#include "ooey/platform/wayland/render_target.hpp"
#include "ooey/bitmap_font.hpp"
#include <wayland-client.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <climits>
#include <cmath>
#include <algorithm>

namespace ooey::wayland {

// Helper to create anonymous shared memory file (memfd or shm_open fallback)
static int create_shm_file(size_t size) {
#if defined(__linux__)
    int fd = -1;
#ifdef MFD_CLOEXEC
    fd = memfd_create("ooey_shm", MFD_CLOEXEC);
#endif
    if (fd >= 0) {
        if (ftruncate(fd, size) < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }
#endif
    // Fallback to shm_open
    static int counter = 0;
    std::string name = "/ooey_shm_" + std::to_string(getpid()) + "_" + std::to_string(counter++);
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        return -1;
    }
    shm_unlink(name.c_str());
    // Round size up to page size to avoid 'data too big for buffer' edge cases
    long page = sysconf(_SC_PAGESIZE);
    if (page > 0) {
        size_t pages = (size + page - 1) / page;
        size = pages * page;
    }
    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

RenderTarget::RenderTarget(wl_display* display, wl_shm* shm, wl_surface* surface, int width, int height)
    : shm_(shm), surface_(surface), display_(display), width_(width), height_(height) {
    stride_ = width_ * 4;
    size_t size = stride_ * height_;
    int fd = create_shm_file(size);
    if (fd < 0) {
        std::cerr << "Failed to create shm file\n";
        data_ = nullptr;
        return;
    }
    data_ = static_cast<uint8_t*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (data_ == MAP_FAILED) {
        std::cerr << "mmap failed\n";
        data_ = nullptr;
        close(fd);
        return;
    }

    wl_shm_pool* pool = wl_shm_create_pool(shm_, fd, static_cast<int>(size));
    buffer_ = wl_shm_pool_create_buffer(pool, 0, width_, height_, stride_, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    // Setup buffer release listener
    released_ = false;
    static const wl_buffer_listener buffer_listener = {
        [](void* data, wl_buffer* /*buffer*/) {
            RenderTarget* self = static_cast<RenderTarget*>(data);
            self->released_ = true;
        }
    };
    wl_buffer_add_listener(buffer_, &buffer_listener, this);
}

RenderTarget::~RenderTarget() {
    if (data_) {
        size_t size = stride_ * height_;
        munmap(data_, size);
        data_ = nullptr;
    }
    if (buffer_) {
        wl_buffer_destroy(buffer_);
        buffer_ = nullptr;
    }
}

void RenderTarget::clear(Color color) {
    if (!data_) {
        return;
    }
    uint8_t r = color.r;
    uint8_t g = color.g;
    uint8_t b = color.b;
    uint8_t a = color.a;
    uint32_t pixel = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(b));
    for (int y = 0; y < height_; ++y) {
        uint32_t* row = reinterpret_cast<uint32_t*>(data_ + y * stride_);
        for (int x = 0; x < width_; ++x) {
            row[x] = pixel;
        }
    }
}

void RenderTarget::draw_geometry(const Geometry& geometry) {
    if (!data_) {
        return;
    }
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
        // Draw each line pair (indices or implicit pairs)
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
            // fallback: draw sequential pairs
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
    if (!surface_ || !buffer_ || !display_) {
        return;
    }
    released_ = false;
    wl_surface_attach(surface_, buffer_, 0, 0);
    wl_surface_damage(surface_, 0, 0, width_, height_);
    wl_surface_commit(surface_);
    wl_display_flush(display_);

    // Wait for the compositor to release the buffer before returning
    while (!released_) {
        if (wl_display_dispatch(display_) < 0) {
            break;
        }
    }
}

void RenderTarget::draw_filled_rect(int x, int y, int w, int h, Color color) {
    if (!data_) {
        return;
    }
    uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));
    for (int yy = y; yy < y + h; ++yy) {
        if (yy < 0 || yy >= height_) {
            continue;
        }
        uint32_t* row = reinterpret_cast<uint32_t*>(data_ + yy * stride_);
        for (int xx = x; xx < x + w; ++xx) {
            if (xx < 0 || xx >= width_) {
                continue;
            }
            row[xx] = pixel;
        }
    }
}

void RenderTarget::draw_line(int start_x, int start_y, int end_x, int end_y, Color color) {
    if (!data_) {
        return;
    }
    // Bresenham's line algorithm (integer)
    int delta_x = abs(end_x - start_x);
    int step_x = start_x < end_x ? 1 : -1;
    int delta_y = -abs(end_y - start_y);
    int step_y = start_y < end_y ? 1 : -1;
    int error = delta_x + delta_y;
    uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));

    while (true) {
        if (start_x >= 0 && start_x < width_ && start_y >= 0 && start_y < height_) {
            uint32_t* row = reinterpret_cast<uint32_t*>(data_ + start_y * stride_);
            row[start_x] = pixel;
        }
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

void RenderTarget::draw_pixel(int x, int y, Color color) {
    if (!data_) {
        return;
    }
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        uint32_t* row = reinterpret_cast<uint32_t*>(data_ + y * stride_);
        uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));
        row[x] = pixel;
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

} // namespace ooey::wayland
