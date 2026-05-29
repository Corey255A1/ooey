#pragma once

#include "ooey/renderer/i_drawable.hpp"
#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"

namespace ooey {

class LinePrimitive : public IDrawable {
public:
    LinePrimitive(Point start, Point end, Color color, float thickness = 1.0f);

    void set_start(Point start);
    Point get_start() const;

    void set_end(Point end);
    Point get_end() const;

    void set_color(Color color);
    Color get_color() const;

    void set_thickness(float thickness);
    float get_thickness() const;

    void draw(IRenderTarget& target) const override;

    bool is_dirty() const;

private:
    void rebuild_geometry() const;

    Point start_;
    Point end_;
    Color color_;
    float thickness_{1.0f};

    mutable Geometry cached_geometry_;
    mutable bool is_dirty_{true};
};

} // namespace ooey
