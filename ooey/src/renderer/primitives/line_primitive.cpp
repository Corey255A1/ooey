#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <cmath>

namespace ooey {

LinePrimitive::LinePrimitive(Point start, Point end, Color color, float thickness)
    : start_(start), end_(end), color_(color), thickness_(thickness), is_dirty_(true) {}

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

bool LinePrimitive::is_dirty() const {
    return is_dirty_;
}

void LinePrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();

    if (thickness_ <= 1.0f) {
        cached_geometry_.type = PrimitiveType::Lines;
        cached_geometry_.vertices.push_back({static_cast<float>(start_.x), static_cast<float>(start_.y), color_});
        cached_geometry_.vertices.push_back({static_cast<float>(end_.x), static_cast<float>(end_.y), color_});
        cached_geometry_.indices = {0, 1};
    } else {
        cached_geometry_.type = PrimitiveType::Triangles;
        float dx = static_cast<float>(end_.x - start_.x);
        float dy = static_cast<float>(end_.y - start_.y);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-5f) {
            return;
        }
        float nx = -dy / len;
        float ny = dx / len;
        float half_t = thickness_ * 0.5f;
        float ox = nx * half_t;
        float oy = ny * half_t;

        cached_geometry_.vertices.push_back({static_cast<float>(start_.x) + ox, static_cast<float>(start_.y) + oy, color_});
        cached_geometry_.vertices.push_back({static_cast<float>(start_.x) - ox, static_cast<float>(start_.y) - oy, color_});
        cached_geometry_.vertices.push_back({static_cast<float>(end_.x) - ox, static_cast<float>(end_.y) - oy, color_});
        cached_geometry_.vertices.push_back({static_cast<float>(end_.x) + ox, static_cast<float>(end_.y) + oy, color_});

        cached_geometry_.indices = {0, 1, 2, 0, 2, 3};
    }
}

void LinePrimitive::draw(IRenderTarget& target) const {
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_);
    }
}

} // namespace ooey
