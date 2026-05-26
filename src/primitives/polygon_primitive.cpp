#include "ooey/primitives/polygon_primitive.hpp"
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

PolygonPrimitive::PolygonPrimitive(std::vector<Point> points, Color fill_color, Color stroke_color, float stroke_thickness)
    : points_(std::move(points)), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness) {}

void PolygonPrimitive::draw(IRenderTarget& target) const {
    if (points_.size() < 3) {
        return;
    }

    Geometry geo;
    geo.type = PrimitiveType::Triangles;

    if (fill_color_.a > 0) {
        unsigned int start_index = static_cast<unsigned int>(geo.vertices.size());
        for (const auto& pt : points_) {
            geo.vertices.push_back({static_cast<float>(pt.x), static_cast<float>(pt.y), fill_color_});
        }

        size_t k = points_.size();
        for (size_t i = 1; i + 1 < k; ++i) {
            geo.indices.push_back(start_index);
            geo.indices.push_back(start_index + static_cast<unsigned int>(i));
            geo.indices.push_back(start_index + static_cast<unsigned int>(i + 1));
        }
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        size_t k = points_.size();
        for (size_t i = 0; i < k; ++i) {
            const auto& p0 = points_[i];
            const auto& p1 = points_[(i + 1) % k];
            add_thick_line(geo, static_cast<float>(p0.x), static_cast<float>(p0.y), static_cast<float>(p1.x), static_cast<float>(p1.y), stroke_thickness_, stroke_color_);
        }
    }

    if (!geo.vertices.empty()) {
        target.draw_geometry(geo);
    }
}

} // namespace ooey
