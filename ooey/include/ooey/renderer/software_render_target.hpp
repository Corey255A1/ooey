#pragma once

#include "ooey/renderer/i_render_target.hpp"
#include <cstdint>

namespace ooey {

class SoftwareRenderTarget : public IRenderTarget {
public:
    SoftwareRenderTarget() = default;
    SoftwareRenderTarget(uint8_t* data, int width, int height, int stride);
    ~SoftwareRenderTarget() override = default;

    void initialize_buffer(uint8_t* data, int width, int height, int stride);

    // IRenderTarget implementation
    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

protected:
    uint8_t* data_{nullptr};
    int width_{0};
    int height_{0};
    int stride_{0};

    void draw_pixel(int x, int y, Color color);
    void draw_line(int start_x, int start_y, int end_x, int end_y, Color color);
    void draw_filled_rect(int x, int y, int w, int h, Color color);
    void draw_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color);
    void draw_flat_bottom_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color);
    void draw_flat_top_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color);
};

} // namespace ooey
