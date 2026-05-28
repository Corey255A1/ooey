#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

namespace ooey::renderer {

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
    : rect_(rect), corner_radius_(corner_radius), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness) {}

void RoundedRectPrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    geo.type = PrimitiveType::Triangles;

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
        unsigned int center_index = static_cast<unsigned int>(geo.vertices.size());
        geo.vertices.push_back({x + w * 0.5f, y + h * 0.5f, fill_color_});

        unsigned int start_index = static_cast<unsigned int>(geo.vertices.size());
        for (const auto& v : perimeter) {
            geo.vertices.push_back(v);
        }

        size_t n = perimeter.size();
        for (size_t i = 0; i < n; ++i) {
            geo.indices.push_back(center_index);
            geo.indices.push_back(start_index + static_cast<unsigned int>(i));
            geo.indices.push_back(start_index + static_cast<unsigned int>((i + 1) % n));
        }
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        size_t n = perimeter.size();
        for (size_t i = 0; i < n; ++i) {
            const auto& p0 = perimeter[i];
            const auto& p1 = perimeter[(i + 1) % n];
            add_thick_line(geo, p0.x, p0.y, p1.x, p1.y, stroke_thickness_, stroke_color_);
        }
    }

    if (!geo.vertices.empty()) {
        target.draw_geometry(geo);
    }
}

} // namespace ooey::renderer
