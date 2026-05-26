#pragma once

#include "ooey/i_drawable.hpp"
#include "ooey/types.hpp"

namespace ooey {

class LinePrimitive : public IDrawable {
public:
    LinePrimitive(Point start, Point end, Color color, float thickness = 1.0f);

    void set_start(Point start) { start_ = start; }
    Point get_start() const { return start_; }

    void set_end(Point end) { end_ = end; }
    Point get_end() const { return end_; }

    void set_color(Color color) { color_ = color; }
    Color get_color() const { return color_; }

    void set_thickness(float thickness) { thickness_ = thickness; }
    float get_thickness() const { return thickness_; }

    void draw(IRenderTarget& target) const override;

private:
    Point start_;
    Point end_;
    Color color_;
    float thickness_{1.0f};
};

} // namespace ooey