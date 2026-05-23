#include "ooey/primitives/line_primitive.hpp"

namespace ooey {

LinePrimitive::LinePrimitive(Point start, Point end, Color color) : start_(start), end_(end), color_(color) {}

void LinePrimitive::draw(IRenderTarget& target) const {
    Geometry geo;
    geo.type = PrimitiveType::Lines;
    
    geo.vertices.push_back({static_cast<float>(start_.x), static_cast<float>(start_.y), color_});
    geo.vertices.push_back({static_cast<float>(end_.x), static_cast<float>(end_.y), color_});

    geo.indices = {0, 1};

    target.draw_geometry(geo);
}

} // namespace ooey