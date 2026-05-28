#pragma once

#include "ooey/mvvmc/i_drawable.hpp"
#include "ooey/types.hpp"

namespace ooey::renderer {

class CirclePrimitive : public IDrawable {
public:
    CirclePrimitive(Point center, int radius, Color fill_color, Color stroke_color = Color{0, 0, 0, 0}, float stroke_thickness = 0.0f);

    void set_center(Point center) { center_ = center; }
    Point get_center() const { return center_; }

    void set_radius(int radius) { radius_ = radius; }
    int get_radius() const { return radius_; }

    void set_fill_color(Color color) { fill_color_ = color; }
    Color get_fill_color() const { return fill_color_; }

    void set_stroke_color(Color color) { stroke_color_ = color; }
    Color get_stroke_color() const { return stroke_color_; }

    void set_stroke_thickness(float thickness) { stroke_thickness_ = thickness; }
    float get_stroke_thickness() const { return stroke_thickness_; }

    void draw(IRenderTarget& target) const override;

private:
    Point center_;
    int radius_;
    Color fill_color_;
    Color stroke_color_;
    float stroke_thickness_;
};

} // namespace ooey::renderer

namespace ooey {
using renderer::CirclePrimitive;
}
