#include "ooey/renderer/primitives/sinusoid_primitive.hpp"
#include <cmath>
#include <vector>

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

SinusoidPrimitive::SinusoidPrimitive(Point start, Point end, float amplitude, float frequency, float phase, Color color, float thickness)
    : start_(start), end_(end), amplitude_(amplitude), frequency_(frequency), phase_(phase), color_(color), thickness_(thickness) {}

void SinusoidPrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    geo.type = PrimitiveType::Triangles;

    int x_start = start_.x;
    int x_end = end_.x;
    int width = x_end - x_start;
    if (width <= 0) {
        return;
    }

    constexpr int num_segments = 100;
    std::vector<Point> points;
    points.reserve(num_segments + 1);

    float cy = static_cast<float>(start_.y);

    for (int i = 0; i <= num_segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(num_segments);
        float x = static_cast<float>(x_start) + t * static_cast<float>(width);
        float angle = 2.0f * PI * (frequency_ * t) + phase_;
        float y = cy + amplitude_ * std::sin(angle);
        points.push_back({static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
    }

    if (thickness_ <= 1.0f) {
        geo.type = PrimitiveType::Lines;
        for (int i = 0; i < num_segments; ++i) {
            unsigned int base = static_cast<unsigned int>(geo.vertices.size());
            geo.vertices.push_back({static_cast<float>(points[i].x), static_cast<float>(points[i].y), color_});
            geo.vertices.push_back({static_cast<float>(points[i + 1].x), static_cast<float>(points[i + 1].y), color_});
            geo.indices.push_back(base);
            geo.indices.push_back(base + 1);
        }
    } else {
        geo.type = PrimitiveType::Triangles;
        for (int i = 0; i < num_segments; ++i) {
            add_thick_line(geo,
                           static_cast<float>(points[i].x), static_cast<float>(points[i].y),
                           static_cast<float>(points[i + 1].x), static_cast<float>(points[i + 1].y),
                           thickness_, color_);
        }
    }

    if (!geo.vertices.empty()) {
        target.draw_geometry(geo);
    }
}

} // namespace ooey::renderer
