#pragma once

#include "ooey/mvvmc/i_drawable.hpp"
#include "ooey/types.hpp"

namespace ooey::renderer {

class CurvePrimitive : public IDrawable {
public:
    // Quadratic Bezier constructor
    CurvePrimitive(Point p0, Point control, Point p1, Color color, float thickness = 1.0f);

    // Cubic Bezier constructor
    CurvePrimitive(Point p0, Point control1, Point control2, Point p1, Color color, float thickness = 1.0f);

    void set_p0(Point p0) { p0_ = p0; }
    Point get_p0() const { return p0_; }

    void set_control1(Point control) { control1_ = control; }
    Point get_control1() const { return control1_; }

    void set_control2(Point control) { control2_ = control; }
    Point get_control2() const { return control2_; }

    void set_p1(Point p1) { p1_ = p1; }
    Point get_p1() const { return p1_; }

    void set_color(Color color) { color_ = color; }
    Color get_color() const { return color_; }

    void set_thickness(float thickness) { thickness_ = thickness; }
    float get_thickness() const { return thickness_; }

    void draw(IRenderTarget& target) const override;

private:
    Point p0_;
    Point control1_;
    Point control2_;
    Point p1_;
    Color color_;
    float thickness_;
    bool is_cubic_;
};

} // namespace ooey::renderer

namespace ooey {
using renderer::CurvePrimitive;
}
