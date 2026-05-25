#pragma once

#include "ooey/i_render_target.hpp"
#include <cstdint>

struct wl_display;
struct wl_shm;
struct wl_surface;
struct wl_buffer;

namespace ooey::wayland {

class RenderTarget : public IRenderTarget {
public:
    RenderTarget(wl_display* display, wl_shm* shm, wl_surface* surface, int width, int height);
    ~RenderTarget() override;

    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

private:
    void draw_filled_rect(int x, int y, int w, int h, Color color);
    void draw_line(int x0, int y0, int x1, int y1, Color color);

    wl_shm* shm_{nullptr};
    wl_surface* surface_{nullptr};
    wl_buffer* buffer_{nullptr};
    wl_display* display_{nullptr};
    uint8_t* data_{nullptr};
    int width_{};
    int height_{};
    int stride_{};
    bool released_{false};
};

} // namespace ooey::wayland
