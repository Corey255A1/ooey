#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <cmath>

namespace ooey {

static void add_thick_line(Geometry& geo, float sx, float sy, float ex, float ey, float thickness, Color color) {
    if (thickness <= 0.0f) {
        return;
    }
    float dx = ex - sx;
    float dy = ey - sy;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-5f) {
        return;
    }
    float nx = -dy / len;
    float ny = dx / len;
    float half_t = thickness * 0.5f;
    float ox = nx * half_t;
    float oy = ny * half_t;

    unsigned int base = static_cast<unsigned int>(geo.vertices.size());
    geo.vertices.push_back({sx + ox, sy + oy, color});
    geo.vertices.push_back({sx - ox, sy - oy, color});
    geo.vertices.push_back({ex - ox, ey - oy, color});
    geo.vertices.push_back({ex + ox, ey + oy, color});

    geo.indices.push_back(base + 0);
    geo.indices.push_back(base + 1);
    geo.indices.push_back(base + 2);
    geo.indices.push_back(base + 0);
    geo.indices.push_back(base + 2);
    geo.indices.push_back(base + 3);
}

RectPrimitive::RectPrimitive(Rect rect, Color fill_color, Color stroke_color, float stroke_thickness)
    : rect_(rect), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness), is_dirty_(true) {}

void RectPrimitive::set_rect(Rect rect) {
    if (rect_ != rect) {
        rect_ = rect;
        is_dirty_ = true;
    }
}

Rect RectPrimitive::get_rect() const {
    return rect_;
}

void RectPrimitive::set_color(Color color) {
    if (fill_color_ != color) {
        fill_color_ = color;
        is_dirty_ = true;
    }
}

void RectPrimitive::set_fill_color(Color color) {
    if (fill_color_ != color) {
        fill_color_ = color;
        is_dirty_ = true;
    }
}

Color RectPrimitive::get_fill_color() const {
    return fill_color_;
}

void RectPrimitive::set_stroke_color(Color color) {
    if (stroke_color_ != color) {
        stroke_color_ = color;
        is_dirty_ = true;
    }
}

Color RectPrimitive::get_stroke_color() const {
    return stroke_color_;
}

void RectPrimitive::set_stroke_thickness(float thickness) {
    if (stroke_thickness_ != thickness) {
        stroke_thickness_ = thickness;
        is_dirty_ = true;
    }
}

float RectPrimitive::get_stroke_thickness() const {
    return stroke_thickness_;
}

bool RectPrimitive::is_dirty() const {
    return is_dirty_;
}

void RectPrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();
    cached_geometry_.type = PrimitiveType::Triangles;

    if (fill_color_.a > 0) {
        cached_geometry_.vertices.push_back({static_cast<float>(rect_.x), static_cast<float>(rect_.y), fill_color_});
        cached_geometry_.vertices.push_back({static_cast<float>(rect_.x + rect_.width), static_cast<float>(rect_.y), fill_color_});
        cached_geometry_.vertices.push_back({static_cast<float>(rect_.x + rect_.width), static_cast<float>(rect_.y + rect_.height), fill_color_});
        cached_geometry_.vertices.push_back({static_cast<float>(rect_.x), static_cast<float>(rect_.y + rect_.height), fill_color_});

        cached_geometry_.indices = {0, 1, 2, 0, 2, 3};
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        float x = static_cast<float>(rect_.x);
        float y = static_cast<float>(rect_.y);
        float w = static_cast<float>(rect_.width);
        float h = static_cast<float>(rect_.height);
        float t = stroke_thickness_;

        add_thick_line(cached_geometry_, x, y, x + w, y, t, stroke_color_);
        add_thick_line(cached_geometry_, x + w, y, x + w, y + h, t, stroke_color_);
        add_thick_line(cached_geometry_, x + w, y + h, x, y + h, t, stroke_color_);
        add_thick_line(cached_geometry_, x, y + h, x, y, t, stroke_color_);
    }
}

void RectPrimitive::draw(IRenderTarget& target) const {
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_);
    }
}

} // namespace ooey
