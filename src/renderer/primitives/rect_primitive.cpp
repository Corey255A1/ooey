#include "ooey/renderer/primitives/rect_primitive.hpp"
#include <cmath>

namespace ooey::renderer {

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

RectPrimitive::RectPrimitive(Rect rect, Color fill_color, Color stroke_color, float stroke_thickness)
    : rect_(rect), fill_color_(fill_color), stroke_color_(stroke_color), stroke_thickness_(stroke_thickness) {}

void RectPrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    geo.type = PrimitiveType::Triangles;

    if (fill_color_.a > 0) {
        geo.vertices.push_back({static_cast<float>(rect_.x), static_cast<float>(rect_.y), fill_color_});
        geo.vertices.push_back({static_cast<float>(rect_.x + rect_.width), static_cast<float>(rect_.y), fill_color_});
        geo.vertices.push_back({static_cast<float>(rect_.x + rect_.width), static_cast<float>(rect_.y + rect_.height), fill_color_});
        geo.vertices.push_back({static_cast<float>(rect_.x), static_cast<float>(rect_.y + rect_.height), fill_color_});

        geo.indices = {0, 1, 2, 0, 2, 3};
    }

    if (stroke_thickness_ > 0.0f && stroke_color_.a > 0) {
        float x = static_cast<float>(rect_.x);
        float y = static_cast<float>(rect_.y);
        float w = static_cast<float>(rect_.width);
        float h = static_cast<float>(rect_.height);
        float t = stroke_thickness_;

        add_thick_line(geo, x, y, x + w, y, t, stroke_color_);
        add_thick_line(geo, x + w, y, x + w, y + h, t, stroke_color_);
        add_thick_line(geo, x + w, y + h, x, y + h, t, stroke_color_);
        add_thick_line(geo, x, y + h, x, y, t, stroke_color_);
    }

    if (!geo.vertices.empty()) {
        target.draw_geometry(geo);
    }
}

} // namespace ooey::renderer