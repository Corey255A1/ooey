#pragma once

#include "ooey/renderer/i_drawable.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"

namespace ooey {

class CirclePrimitive : public IDrawable {
public:
    CirclePrimitive(Point center, int radius, Color fill_color, Color stroke_color = Color{0, 0, 0, 0}, float stroke_thickness = 0.0f);

    void set_center(Point center);
    Point get_center() const;

    void set_radius(int radius);
    int get_radius() const;

    void set_fill_color(Color color);
    Color get_fill_color() const;

    void set_stroke_color(Color color);
    Color get_stroke_color() const;

    void set_stroke_thickness(float thickness);
    float get_stroke_thickness() const;

    void draw(IRenderTarget& target) const override;

    bool is_dirty() const;

private:
    void rebuild_geometry() const;

    Point center_;
    int radius_;
    Color fill_color_;
    Color stroke_color_;
    float stroke_thickness_;

    mutable Geometry cached_geometry_;
    mutable bool is_dirty_{true};
};

} // namespace ooey
