#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

namespace ooey {

constexpr float PI = 3.14159265f;

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

static void add_corner_arc(std::vector<Vertex>& points, float cx, float cy, float r, float start_angle, float end_angle, Color color) {
    constexpr int num_arc_segments = 8;
    for (int i = 0; i <= num_arc_segments; ++i) {
        float angle = start_angle + (end_angle - start_angle) * static_cast<float>(i) / static_cast<float>(num_arc_segments);
        points.push_back({cx + r * std::cos(angle), cy + r * std::sin(angle), color});
    }
}

RoundedRectPrimitive::RoundedRectPrimitive(Rect rect, int corner_radius, Color fill_color, Color stroke_color, float stroke_thickness)
    : rect_(rect), corner_radius_(corner_radius), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness), is_dirty_(true) {}

void RoundedRectPrimitive::set_rect(Rect rect) {
    if (rect_ != rect) {
        rect_ = rect;
        is_dirty_ = true;
    }
}

Rect RoundedRectPrimitive::get_rect() const {
    return rect_;
}

void RoundedRectPrimitive::set_corner_radius(int radius) {
    if (corner_radius_ != radius) {
        corner_radius_ = radius;
        is_dirty_ = true;
    }
}

int RoundedRectPrimitive::get_corner_radius() const {
    return corner_radius_;
}

void RoundedRectPrimitive::set_fill_color(Color color) {
    if (fill_color_ != color) {
        fill_color_ = color;
        is_dirty_ = true;
    }
}

Color RoundedRectPrimitive::get_fill_color() const {
    return fill_color_;
}

void RoundedRectPrimitive::set_stroke_color(Color color) {
    if (stroke_color_ != color) {
        stroke_color_ = color;
        is_dirty_ = true;
    }
}

Color RoundedRectPrimitive::get_stroke_color() const {
    return stroke_color_;
}

void RoundedRectPrimitive::set_stroke_thickness(float thickness) {
    if (stroke_thickness_ != thickness) {
        stroke_thickness_ = thickness;
        is_dirty_ = true;
    }
}

float RoundedRectPrimitive::get_stroke_thickness() const {
    return stroke_thickness_;
}

bool RoundedRectPrimitive::is_dirty() const {
    return is_dirty_;
}

void RoundedRectPrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();
    cached_geometry_.type = PrimitiveType::Triangles;

    float x = static_cast<float>(rect_.x);
    float y = static_cast<float>(rect_.y);
    float w = static_cast<float>(rect_.width);
    float h = static_cast<float>(rect_.height);
    float r = static_cast<float>(corner_radius_);

    if (2.0f * r > w) {
        r = w * 0.5f;
    }
    if (2.0f * r > h) {
        r = h * 0.5f;
    }

    if (r < 0.0f) {
        r = 0.0f;
    }

    std::vector<Vertex> perimeter;
    add_corner_arc(perimeter, x + r, y + r, r, PI, 1.5f * PI, fill_color_);
    add_corner_arc(perimeter, x + w - r, y + r, r, 1.5f * PI, 2.0f * PI, fill_color_);
    add_corner_arc(perimeter, x + w - r, y + h - r, r, 0.0f, 0.5f * PI, fill_color_);
    add_corner_arc(perimeter, x + r, y + h - r, r, 0.5f * PI, PI, fill_color_);

    if (fill_color_.a > 0) {
        unsigned int center_index = static_cast<unsigned int>(cached_geometry_.vertices.size());
        cached_geometry_.vertices.push_back({x + w * 0.5f, y + h * 0.5f, fill_color_});

        unsigned int start_index = static_cast<unsigned int>(cached_geometry_.vertices.size());
        for (const auto& v : perimeter) {
            cached_geometry_.vertices.push_back(v);
        }

        size_t n = perimeter.size();
        for (size_t i = 0; i < n; ++i) {
            cached_geometry_.indices.push_back(center_index);
            cached_geometry_.indices.push_back(start_index + static_cast<unsigned int>(i));
            cached_geometry_.indices.push_back(start_index + static_cast<unsigned int>((i + 1) % n));
        }
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        size_t n = perimeter.size();
        for (size_t i = 0; i < n; ++i) {
            const auto& p0 = perimeter[i];
            const auto& p1 = perimeter[(i + 1) % n];
            add_thick_line(cached_geometry_, p0.x, p0.y, p1.x, p1.y, stroke_thickness_, stroke_color_);
        }
    }
}

void RoundedRectPrimitive::draw(IRenderTarget& target) const {
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_);
    }
}

} // namespace ooey
