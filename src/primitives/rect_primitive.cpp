#include "ooey/primitives/rect_primitive.hpp"

namespace ooey {

RectPrimitive::RectPrimitive(Rect rect, Color color) : rect_(rect), color_(color) {}

void RectPrimitive::set_color(Color color) {
    color_ = color;
}

std::vector<Geometry> RectPrimitive::generate_geometry() const {
    Geometry geo;
    geo.type = PrimitiveType::Triangles;
    
    // Top-left
    geo.vertices.push_back({static_cast<float>(rect_.x), static_cast<float>(rect_.y), color_});
    // Top-right
    geo.vertices.push_back({static_cast<float>(rect_.x + rect_.width), static_cast<float>(rect_.y), color_});
    // Bottom-right
    geo.vertices.push_back({static_cast<float>(rect_.x + rect_.width), static_cast<float>(rect_.y + rect_.height), color_});
    // Bottom-left
    geo.vertices.push_back({static_cast<float>(rect_.x), static_cast<float>(rect_.y + rect_.height), color_});

    geo.indices = {0, 1, 2, 0, 2, 3};

    return {geo};
}

} // namespace ooey