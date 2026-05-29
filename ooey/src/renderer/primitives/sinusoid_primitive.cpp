#include "ooey/renderer/primitives/sinusoid_primitive.hpp"
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

SinusoidPrimitive::SinusoidPrimitive(Point start, Point end, float amplitude, float frequency, float phase, Color color, float thickness)
    : start_(start), end_(end), amplitude_(amplitude), frequency_(frequency), phase_(phase), color_(color), thickness_(thickness), is_dirty_(true) {}

void SinusoidPrimitive::set_start(Point start) {
    if (start_ != start) {
        start_ = start;
        is_dirty_ = true;
    }
}

Point SinusoidPrimitive::get_start() const {
    return start_;
}

void SinusoidPrimitive::set_end(Point end) {
    if (end_ != end) {
        end_ = end;
        is_dirty_ = true;
    }
}

Point SinusoidPrimitive::get_end() const {
    return end_;
}

void SinusoidPrimitive::set_amplitude(float amplitude) {
    if (amplitude_ != amplitude) {
        amplitude_ = amplitude;
        is_dirty_ = true;
    }
}

float SinusoidPrimitive::get_amplitude() const {
    return amplitude_;
}

void SinusoidPrimitive::set_frequency(float frequency) {
    if (frequency_ != frequency) {
        frequency_ = frequency;
        is_dirty_ = true;
    }
}

float SinusoidPrimitive::get_frequency() const {
    return frequency_;
}

void SinusoidPrimitive::set_phase(float phase) {
    if (phase_ != phase) {
        phase_ = phase;
        is_dirty_ = true;
    }
}

float SinusoidPrimitive::get_phase() const {
    return phase_;
}

void SinusoidPrimitive::set_color(Color color) {
    if (color_ != color) {
        color_ = color;
        is_dirty_ = true;
    }
}

Color SinusoidPrimitive::get_color() const {
    return color_;
}

void SinusoidPrimitive::set_thickness(float thickness) {
    if (thickness_ != thickness) {
        thickness_ = thickness;
        is_dirty_ = true;
    }
}

float SinusoidPrimitive::get_thickness() const {
    return thickness_;
}

bool SinusoidPrimitive::is_dirty() const {
    return is_dirty_;
}

void SinusoidPrimitive::rebuild_geometry() const {
    cached_geometry_.vertices.clear();
    cached_geometry_.indices.clear();

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
        cached_geometry_.type = PrimitiveType::Lines;
        for (int i = 0; i < num_segments; ++i) {
            unsigned int base = static_cast<unsigned int>(cached_geometry_.vertices.size());
            cached_geometry_.vertices.push_back({static_cast<float>(points[i].x), static_cast<float>(points[i].y), color_});
            cached_geometry_.vertices.push_back({static_cast<float>(points[i + 1].x), static_cast<float>(points[i + 1].y), color_});
            cached_geometry_.indices.push_back(base);
            cached_geometry_.indices.push_back(base + 1);
        }
    } else {
        cached_geometry_.type = PrimitiveType::Triangles;
        for (int i = 0; i < num_segments; ++i) {
            add_thick_line(cached_geometry_,
                           static_cast<float>(points[i].x), static_cast<float>(points[i].y),
                           static_cast<float>(points[i + 1].x), static_cast<float>(points[i + 1].y),
                           thickness_, color_);
        }
    }
}

void SinusoidPrimitive::draw(IRenderTarget& target) const {
    if (is_dirty_) {
        rebuild_geometry();
        is_dirty_ = false;
    }
    if (!cached_geometry_.vertices.empty()) {
        target.draw_geometry(cached_geometry_);
    }
}

} // namespace ooey
