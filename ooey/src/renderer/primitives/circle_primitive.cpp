#include "ooey/renderer/primitives/circle_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"
#include <cmath>
#include <vector>

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

CirclePrimitive::CirclePrimitive(Point center, int radius, Color fill_color, Color stroke_color, float stroke_thickness)
    : center_(center), radius_(radius), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness), is_dirty_(true) {}

void CirclePrimitive::set_center(Point center) {
    if (center_ != center) {
        center_ = center;
        is_dirty_ = true;
    }
}

Point CirclePrimitive::get_center() const {
    return center_;
}

void CirclePrimitive::set_radius(int radius) {
    if (radius_ != radius) {
        radius_ = radius;
        is_dirty_ = true;
    }
}

int CirclePrimitive::get_radius() const {
    return radius_;
}

void CirclePrimitive::set_fill_color(Color color) {
    if (fill_color_ != color) {
        fill_color_ = color;
        is_dirty_ = true;
    }
}

Color CirclePrimitive::get_fill_color() const {
    return fill_color_;
}

void CirclePrimitive::set_stroke_color(Color color) {
    if (stroke_color_ != color) {
        stroke_color_ = color;
        is_dirty_ = true;
    }
}

Color CirclePrimitive::get_stroke_color() const {
    return stroke_color_;
}

void CirclePrimitive::set_stroke_thickness(float thickness) {
    if (stroke_thickness_ != thickness) {
        stroke_thickness_ = thickness;
        is_dirty_ = true;
    }
}

float CirclePrimitive::get_stroke_thickness() const {
    return stroke_thickness_;
}

bool CirclePrimitive::is_dirty() const {
    return is_dirty_;
}

void CirclePrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();
    cached_geometry_.type = PrimitiveType::Triangles;

    constexpr int num_segments = 64;
    std::vector<Vertex> circle_points;
    circle_points.reserve(num_segments);

    float cx = static_cast<float>(center_.x);
    float cy = static_cast<float>(center_.y);
    float r = static_cast<float>(radius_);

    for (int i = 0; i < num_segments; ++i) {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(num_segments);
        circle_points.push_back({cx + r * std::cos(angle), cy + r * std::sin(angle), fill_color_});
    }

    if (fill_color_.a > 0) {
        unsigned int center_index = static_cast<unsigned int>(cached_geometry_.vertices.size());
        cached_geometry_.vertices.push_back({cx, cy, fill_color_});

        unsigned int start_index = static_cast<unsigned int>(cached_geometry_.vertices.size());
        for (const auto& v : circle_points) {
            cached_geometry_.vertices.push_back(v);
        }

        for (int i = 0; i < num_segments; ++i) {
            cached_geometry_.indices.push_back(center_index);
            cached_geometry_.indices.push_back(start_index + i);
            cached_geometry_.indices.push_back(start_index + ((i + 1) % num_segments));
        }
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        for (int i = 0; i < num_segments; ++i) {
            const auto& p0 = circle_points[i];
            const auto& p1 = circle_points[(i + 1) % num_segments];
            add_thick_line(cached_geometry_, p0.x, p0.y, p1.x, p1.y, stroke_thickness_, stroke_color_);
        }
    }
}

void CirclePrimitive::draw(IRenderTarget& target) const {
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_);
    }
}

} // namespace ooey
