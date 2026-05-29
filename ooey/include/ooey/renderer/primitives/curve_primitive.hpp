#pragma once

#include "ooey/renderer/i_drawable.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"

namespace ooey {

class CurvePrimitive : public IDrawable {
public:
    // Quadratic Bezier constructor
    CurvePrimitive(Point p0, Point control, Point p1, Color color, float thickness = 1.0f);

    // Cubic Bezier constructor
    CurvePrimitive(Point p0, Point control1, Point control2, Point p1, Color color, float thickness = 1.0f);

    void set_p0(Point p0);
    Point get_p0() const;

    void set_control1(Point control);
    Point get_control1() const;

    void set_control2(Point control);
    Point get_control2() const;

    void set_p1(Point p1);
    Point get_p1() const;

    void set_color(Color color);
    Color get_color() const;

    void set_thickness(float thickness);
    float get_thickness() const;

    void draw(IRenderTarget& target) const override;

    bool is_dirty() const;

private:
    void rebuild_geometry() const;

    Point p0_;
    Point control1_;
    Point control2_;
    Point p1_;
    Color color_;
    float thickness_;
    bool is_cubic_;

    mutable Geometry cached_geometry_;
    mutable bool is_dirty_{true};
};

} // namespace ooey
