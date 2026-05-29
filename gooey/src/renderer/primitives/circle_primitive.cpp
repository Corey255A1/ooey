namespace ooey {}

#include "gooey/renderer/primitives/circle_primitive.hpp"
#include <cmath>
#include <vector>

namespace gooey::renderer {
    using namespace ooey;

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
    : center_(center), radius_(radius), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness) {}

void CirclePrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    geo.type = PrimitiveType::Triangles;

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
        unsigned int center_index = static_cast<unsigned int>(geo.vertices.size());
        geo.vertices.push_back({cx, cy, fill_color_});

        unsigned int start_index = static_cast<unsigned int>(geo.vertices.size());
        for (const auto& v : circle_points) {
            geo.vertices.push_back(v);
        }

        for (int i = 0; i < num_segments; ++i) {
            geo.indices.push_back(center_index);
            geo.indices.push_back(start_index + i);
            geo.indices.push_back(start_index + ((i + 1) % num_segments));
        }
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        for (int i = 0; i < num_segments; ++i) {
            const auto& p0 = circle_points[i];
            const auto& p1 = circle_points[(i + 1) % num_segments];
            add_thick_line(geo, p0.x, p0.y, p1.x, p1.y, stroke_thickness_, stroke_color_);
        }
    }

    if (!geo.vertices.empty()) {
        target.draw_geometry(geo);
    }
}

} // namespace gooey::renderer
