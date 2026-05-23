#pragma once

#include "ooey/i_drawable.hpp"
#include "ooey/types.hpp"

namespace ooey {

class LinePrimitive : public IDrawable {
public:
    LinePrimitive(Point start, Point end, Color color);

    void draw(IRenderTarget& target) const override;

private:
    Point start_;
    Point end_;
    Color color_;
};

} // namespace ooey