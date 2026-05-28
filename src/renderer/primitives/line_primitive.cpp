#include "ooey/renderer/primitives/line_primitive.hpp"
#include <cmath>

namespace ooey::renderer {

LinePrimitive::LinePrimitive(Point start, Point end, Color color, float thickness)
    : start_(start), end_(end), color_(color), thickness_(thickness) {}

void LinePrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    if (thickness_ <= 1.0f) {
        geo.type = PrimitiveType::Lines;
        geo.vertices.push_back({static_cast<float>(start_.x), static_cast<float>(start_.y), color_});
        geo.vertices.push_back({static_cast<float>(end_.x), static_cast<float>(end_.y), color_});
        geo.indices = {0, 1};
    } else {
        geo.type = PrimitiveType::Triangles;
        float dx = static_cast<float>(end_.x - start_.x);
        float dy = static_cast<float>(end_.y - start_.y);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-5f) {
            return;
        }
        float nx = -dy / len;
        float ny = dx / len;
        float half_t = thickness_ * 0.5f;
        float ox = nx * half_t;
        float oy = ny * half_t;

        geo.vertices.push_back({static_cast<float>(start_.x) + ox, static_cast<float>(start_.y) + oy, color_});
        geo.vertices.push_back({static_cast<float>(start_.x) - ox, static_cast<float>(start_.y) - oy, color_});
        geo.vertices.push_back({static_cast<float>(end_.x) - ox, static_cast<float>(end_.y) - oy, color_});
        geo.vertices.push_back({static_cast<float>(end_.x) + ox, static_cast<float>(end_.y) + oy, color_});

        geo.indices = {0, 1, 2, 0, 2, 3};
    }
    target.draw_geometry(geo);
}

} // namespace ooey::renderer