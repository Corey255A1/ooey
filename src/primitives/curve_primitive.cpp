#include "ooey/primitives/curve_primitive.hpp"
#include <cmath>
#include <vector>

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

CurvePrimitive::CurvePrimitive(Point p0, Point control, Point p1, Color color, float thickness)
    : p0_(p0), control1_(control), control2_(control), p1_(p1), color_(color), thickness_(thickness), is_cubic_(false) {}

CurvePrimitive::CurvePrimitive(Point p0, Point control1, Point control2, Point p1, Color color, float thickness)
    : p0_(p0), control1_(control1), control2_(control2), p1_(p1), color_(color), thickness_(thickness), is_cubic_(true) {}

void CurvePrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    geo.type = PrimitiveType::Triangles;

    constexpr int num_segments = 30;
    std::vector<Point> curve_points;
    curve_points.reserve(num_segments + 1);

    float x0 = static_cast<float>(p0_.x);
    float y0 = static_cast<float>(p0_.y);
    float x1 = static_cast<float>(p1_.x);
    float y1 = static_cast<float>(p1_.y);

    float cx1 = static_cast<float>(control1_.x);
    float cy1 = static_cast<float>(control1_.y);
    float cx2 = static_cast<float>(control2_.x);
    float cy2 = static_cast<float>(control2_.y);

    for (int i = 0; i <= num_segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(num_segments);
        float one_minus_t = 1.0f - t;

        float x = 0.0f;
        float y = 0.0f;

        if (is_cubic_) {
            float w0 = one_minus_t * one_minus_t * one_minus_t;
            float w1 = 3.0f * one_minus_t * one_minus_t * t;
            float w2 = 3.0f * one_minus_t * t * t;
            float w3 = t * t * t;

            x = w0 * x0 + w1 * cx1 + w2 * cx2 + w3 * x1;
            y = w0 * y0 + w1 * cy1 + w2 * cy2 + w3 * y1;
        } else {
            float w0 = one_minus_t * one_minus_t;
            float w1 = 2.0f * one_minus_t * t;
            float w2 = t * t;

            x = w0 * x0 + w1 * cx1 + w2 * x1;
            y = w0 * y0 + w1 * cy1 + w2 * y1;
        }

        curve_points.push_back({static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
    }

    if (thickness_ <= 1.0f) {
        geo.type = PrimitiveType::Lines;
        for (int i = 0; i < num_segments; ++i) {
            unsigned int base = static_cast<unsigned int>(geo.vertices.size());
            geo.vertices.push_back({static_cast<float>(curve_points[i].x), static_cast<float>(curve_points[i].y), color_});
            geo.vertices.push_back({static_cast<float>(curve_points[i + 1].x), static_cast<float>(curve_points[i + 1].y), color_});
            geo.indices.push_back(base);
            geo.indices.push_back(base + 1);
        }
    } else {
        geo.type = PrimitiveType::Triangles;
        for (int i = 0; i < num_segments; ++i) {
            add_thick_line(geo,
                           static_cast<float>(curve_points[i].x), static_cast<float>(curve_points[i].y),
                           static_cast<float>(curve_points[i + 1].x), static_cast<float>(curve_points[i + 1].y),
                           thickness_, color_);
        }
    }

    if (!geo.vertices.empty()) {
        target.draw_geometry(geo);
    }
}

} // namespace ooey
