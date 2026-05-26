#pragma once

#include "ooey/i_drawable.hpp"
#include "ooey/types.hpp"
#include <vector>

namespace ooey {

class PolygonPrimitive : public IDrawable {
public:
    PolygonPrimitive(std::vector<Point> points, Color fill_color, Color stroke_color = Color{0, 0, 0, 0}, float stroke_thickness = 0.0f);

    void set_points(std::vector<Point> points) { points_ = std::move(points); }
    const std::vector<Point>& get_points() const { return points_; }

    void set_fill_color(Color color) { fill_color_ = color; }
    Color get_fill_color() const { return fill_color_; }

    void set_stroke_color(Color color) { stroke_color_ = color; }
    Color get_stroke_color() const { return stroke_color_; }

    void set_stroke_thickness(float thickness) { stroke_thickness_ = thickness; }
    float get_stroke_thickness() const { return stroke_thickness_; }

    void draw(IRenderTarget& target) const override;

private:
    std::vector<Point> points_;
    Color fill_color_;
    Color stroke_color_;
    float stroke_thickness_;
};

} // namespace ooey
