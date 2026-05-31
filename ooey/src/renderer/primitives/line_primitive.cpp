#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <cmath>

namespace ooey {

LinePrimitive::LinePrimitive(Point start, Point end, Color color, float thickness, LineStyle style)
    : start_(start), end_(end), color_(color), thickness_(thickness), style_(style), is_dirty_(true) {}

void LinePrimitive::set_start(Point start) {
    if (start_ != start) {
        start_ = start;
        is_dirty_ = true;
    }
}

Point LinePrimitive::get_start() const {
    return start_;
}

void LinePrimitive::set_end(Point end) {
    if (end_ != end) {
        end_ = end;
        is_dirty_ = true;
    }
}

Point LinePrimitive::get_end() const {
    return end_;
}

void LinePrimitive::set_color(Color color) {
    if (color_ != color) {
        color_ = color;
        is_dirty_ = true;
    }
}

Color LinePrimitive::get_color() const {
    return color_;
}

void LinePrimitive::set_thickness(float thickness) {
    if (thickness_ != thickness) {
        thickness_ = thickness;
        is_dirty_ = true;
    }
}

float LinePrimitive::get_thickness() const {
    return thickness_;
}

void LinePrimitive::set_style(LineStyle style) {
    if (style_ != style) {
        style_ = style;
        is_dirty_ = true;
    }
}

LineStyle LinePrimitive::get_style() const {
    return style_;
}

bool LinePrimitive::is_dirty() const {
    return is_dirty_;
}

void LinePrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();

    float dx = static_cast<float>(end_.x - start_.x);
    float dy = static_cast<float>(end_.y - start_.y);
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-5f) {
        return;
    }
    float ux = dx / len;
    float uy = dy / len;

    if (style_ == LineStyle::Solid) {
        if (thickness_ <= 1.0f) {
            cached_geometry_.type = PrimitiveType::Lines;
            cached_geometry_.vertices.push_back({static_cast<float>(start_.x), static_cast<float>(start_.y), color_});
            cached_geometry_.vertices.push_back({static_cast<float>(end_.x), static_cast<float>(end_.y), color_});
            cached_geometry_.indices = {0, 1};
        } else {
            cached_geometry_.type = PrimitiveType::Triangles;
            float nx = -uy;
            float ny = ux;
            float half_t = thickness_ * 0.5f;
            float ox = nx * half_t;
            float oy = ny * half_t;

            cached_geometry_.vertices.push_back({static_cast<float>(start_.x) + ox, static_cast<float>(start_.y) + oy, color_});
            cached_geometry_.vertices.push_back({static_cast<float>(start_.x) - ox, static_cast<float>(start_.y) - oy, color_});
            cached_geometry_.vertices.push_back({static_cast<float>(end_.x) - ox, static_cast<float>(end_.y) - oy, color_});
            cached_geometry_.vertices.push_back({static_cast<float>(end_.x) + ox, static_cast<float>(end_.y) + oy, color_});

            cached_geometry_.indices = {0, 1, 2, 0, 2, 3};
        }
    } else {
        // Dashed or Dotted
        float dash_len = 8.0f;
        float gap_len = 4.0f;
        if (style_ == LineStyle::Dotted) {
            dash_len = thickness_ > 1.0f ? thickness_ : 2.0f;
            gap_len = thickness_ > 1.0f ? thickness_ * 2.0f : 4.0f;
        }

        if (thickness_ <= 1.0f) {
            cached_geometry_.type = PrimitiveType::Lines;
            float t = 0.0f;
            bool drawing = true;
            uint32_t vert_idx = 0;
            while (t < len) {
                float next_t = t + (drawing ? dash_len : gap_len);
                if (next_t > len) {
                    next_t = len;
                }
                if (drawing) {
                    float sx = start_.x + ux * t;
                    float sy = start_.y + uy * t;
                    float ex = start_.x + ux * next_t;
                    float ey = start_.y + uy * next_t;
                    cached_geometry_.vertices.push_back({sx, sy, color_});
                    cached_geometry_.vertices.push_back({ex, ey, color_});
                    cached_geometry_.indices.push_back(vert_idx);
                    cached_geometry_.indices.push_back(vert_idx + 1);
                    vert_idx += 2;
                }
                t = next_t;
                drawing = !drawing;
            }
        } else {
            cached_geometry_.type = PrimitiveType::Triangles;
            float nx = -uy;
            float ny = ux;
            float half_t = thickness_ * 0.5f;
            float ox = nx * half_t;
            float oy = ny * half_t;

            float t = 0.0f;
            bool drawing = true;
            uint32_t vert_idx = 0;
            while (t < len) {
                float next_t = t + (drawing ? dash_len : gap_len);
                if (next_t > len) {
                    next_t = len;
                }
                if (drawing) {
                    float sx = start_.x + ux * t;
                    float sy = start_.y + uy * t;
                    float ex = start_.x + ux * next_t;
                    float ey = start_.y + uy * next_t;

                    cached_geometry_.vertices.push_back({sx + ox, sy + oy, color_});
                    cached_geometry_.vertices.push_back({sx - ox, sy - oy, color_});
                    cached_geometry_.vertices.push_back({ex - ox, ey - oy, color_});
                    cached_geometry_.vertices.push_back({ex + ox, ey + oy, color_});

                    cached_geometry_.indices.push_back(vert_idx + 0);
                    cached_geometry_.indices.push_back(vert_idx + 1);
                    cached_geometry_.indices.push_back(vert_idx + 2);
                    cached_geometry_.indices.push_back(vert_idx + 0);
                    cached_geometry_.indices.push_back(vert_idx + 2);
                    cached_geometry_.indices.push_back(vert_idx + 3);
                    vert_idx += 4;
                }
                t = next_t;
                drawing = !drawing;
            }
        }
    }
}

void LinePrimitive::draw(IRenderTarget& target) const {
    bool dirty = is_dirty_;
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_, this, dirty);
    }
}

} // namespace ooey
