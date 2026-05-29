#pragma once

namespace ooey {}


#include "gooey/mvvmc/i_drawable.hpp"
#include "ooey/types.hpp"

namespace gooey::renderer {
    using namespace ooey;

class RectPrimitive : public IDrawable {
public:
    RectPrimitive(Rect rect, Color fill_color, Color stroke_color = Color{0, 0, 0, 0}, float stroke_thickness = 0.0f);

    void set_rect(Rect rect) { rect_ = rect; }
    Rect get_rect() const { return rect_; }

    void set_color(Color color) { fill_color_ = color; } // Compatibility
    void set_fill_color(Color color) { fill_color_ = color; }
    Color get_fill_color() const { return fill_color_; }

    void set_stroke_color(Color color) { stroke_color_ = color; }
    Color get_stroke_color() const { return stroke_color_; }

    void set_stroke_thickness(float thickness) { stroke_thickness_ = thickness; }
    float get_stroke_thickness() const { return stroke_thickness_; }

    void draw(IRenderTarget& target) const override;

private:
    Rect rect_;
    Color fill_color_;
    Color stroke_color_{0, 0, 0, 0};
    float stroke_thickness_{0.0f};
};

} // namespace gooey::renderer
namespace gooey {
    using namespace ooey;
using gooey::renderer::RectPrimitive;
}
