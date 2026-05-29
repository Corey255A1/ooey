#include "ooey/renderer/primitives/curve_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"
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
    : p0_(p0), control1_(control), control2_(control), p1_(p1), color_(color), thickness_(thickness), is_cubic_(false), is_dirty_(true) {}

CurvePrimitive::CurvePrimitive(Point p0, Point control1, Point control2, Point p1, Color color, float thickness)
    : p0_(p0), control1_(control1), control2_(control2), p1_(p1), color_(color), thickness_(thickness), is_cubic_(true), is_dirty_(true) {}

void CurvePrimitive::set_p0(Point p0) {
    if (p0_ != p0) {
        p0_ = p0;
        is_dirty_ = true;
    }
}

Point CurvePrimitive::get_p0() const {
    return p0_;
}

void CurvePrimitive::set_control1(Point control) {
    if (control1_ != control) {
        control1_ = control;
        is_dirty_ = true;
    }
}

Point CurvePrimitive::get_control1() const {
    return control1_;
}

void CurvePrimitive::set_control2(Point control) {
    if (control2_ != control) {
        control2_ = control;
        is_dirty_ = true;
    }
}

Point CurvePrimitive::get_control2() const {
    return control2_;
}

void CurvePrimitive::set_p1(Point p1) {
    if (p1_ != p1) {
        p1_ = p1;
        is_dirty_ = true;
    }
}

Point CurvePrimitive::get_p1() const {
    return p1_;
}

void CurvePrimitive::set_color(Color color) {
    if (color_ != color) {
        color_ = color;
        is_dirty_ = true;
    }
}

Color CurvePrimitive::get_color() const {
    return color_;
}

void CurvePrimitive::set_thickness(float thickness) {
    if (thickness_ != thickness) {
        thickness_ = thickness;
        is_dirty_ = true;
    }
}

float CurvePrimitive::get_thickness() const {
    return thickness_;
}

bool CurvePrimitive::is_dirty() const {
    return is_dirty_;
}

void CurvePrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();

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
        cached_geometry_.type = PrimitiveType::Lines;
        for (int i = 0; i < num_segments; ++i) {
            unsigned int base = static_cast<unsigned int>(cached_geometry_.vertices.size());
            cached_geometry_.vertices.push_back({static_cast<float>(curve_points[i].x), static_cast<float>(curve_points[i].y), color_});
            cached_geometry_.vertices.push_back({static_cast<float>(curve_points[i + 1].x), static_cast<float>(curve_points[i + 1].y), color_});
            cached_geometry_.indices.push_back(base);
            cached_geometry_.indices.push_back(base + 1);
        }
    } else {
        cached_geometry_.type = PrimitiveType::Triangles;
        for (int i = 0; i < num_segments; ++i) {
            add_thick_line(cached_geometry_,
                           static_cast<float>(curve_points[i].x), static_cast<float>(curve_points[i].y),
                           static_cast<float>(curve_points[i + 1].x), static_cast<float>(curve_points[i + 1].y),
                           thickness_, color_);
        }
    }
}

void CurvePrimitive::draw(IRenderTarget& target) const {
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_);
    }
}

} // namespace ooey
