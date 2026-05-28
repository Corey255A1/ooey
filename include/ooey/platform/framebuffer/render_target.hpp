#pragma once

#include "ooey/renderer/i_render_target.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <linux/fb.h>

namespace ooey::framebuffer {

class RenderTarget : public IRenderTarget {
public:
    RenderTarget(int fd, int rotation);
    ~RenderTarget() override;

    bool initialize();

    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

    int get_logical_width() const { return logical_w_; }
    int get_logical_height() const { return logical_h_; }

private:
    void map_coords(int lx, int ly, int& px, int& py) const;
    void draw_pixel(int lx, int ly, Color color);
    void draw_filled_rect(int x, int y, int w, int h, Color color);
    void draw_line(int start_x, int start_y, int end_x, int end_y, Color color);
    void draw_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color);
    void draw_flat_bottom_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color);
    void draw_flat_top_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color);

    int fd_;
    int rotation_;

    struct fb_var_screeninfo vinfo_{};
    struct fb_fix_screeninfo finfo_{};

    uint8_t* fb_mem_{nullptr};
    size_t fb_mem_size_{0};

    std::vector<uint8_t> backbuffer_;

    int phys_w_{0};
    int phys_h_{0};
    int logical_w_{0};
    int logical_h_{0};
};

} // namespace ooey::framebuffer
