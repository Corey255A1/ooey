#pragma once

#include "ooey/i_drawable.hpp"
#include "ooey/types.hpp"

namespace ooey {

class RectPrimitive : public IDrawable {
public:
    RectPrimitive(Rect rect, Color color);

    void set_color(Color color);

    std::vector<Geometry> generate_geometry() const override;

private:
    Rect rect_;
    Color color_;
};

} // namespace ooey