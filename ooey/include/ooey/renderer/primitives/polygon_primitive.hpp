#pragma once

#include "ooey/renderer/i_drawable.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"
#include <vector>

namespace ooey {

class PolygonPrimitive : public IDrawable {
public:
    PolygonPrimitive(std::vector<Point> points, Color fill_color, Color stroke_color = Color{0, 0, 0, 0}, float stroke_thickness = 0.0f);

    void set_points(std::vector<Point> points);
    const std::vector<Point>& get_points() const;

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

    std::vector<Point> points_;
    Color fill_color_;
    Color stroke_color_;
    float stroke_thickness_;

    mutable Geometry cached_geometry_;
    mutable bool is_dirty_{true};
};

} // namespace ooey
