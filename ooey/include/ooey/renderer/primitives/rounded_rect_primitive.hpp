#pragma once

#include "ooey/renderer/i_drawable.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"

namespace ooey {

class RoundedRectPrimitive : public IDrawable {
public:
    RoundedRectPrimitive(Rect rect, int corner_radius, Color fill_color, Color stroke_color = Color{0, 0, 0, 0}, float stroke_thickness = 0.0f);

    void set_rect(Rect rect);
    Rect get_rect() const;

    void set_corner_radius(int radius);
    int get_corner_radius() const;

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

    Rect rect_;
    int corner_radius_;
    Color fill_color_;
    Color stroke_color_{0, 0, 0, 0};
    float stroke_thickness_{0.0f};

    mutable Geometry cached_geometry_;
    mutable bool is_dirty_{true};
};

} // namespace ooey
