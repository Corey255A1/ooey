#include "styled_panel.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "gooey/mvvmc/theme.hpp"

using namespace ooey;
using namespace gooey;

StyledPanel::StyledPanel() {
    set_padding(15);
    set_margin(5);
    set_align_self(Align::Stretch);
}

void StyledPanel::draw(ooey::IRenderTarget& target) const {
    if (corner_radius_ > 0 || stroke_thickness_ > 0.0f) {
        RoundedRectPrimitive(layout_bounds, corner_radius_, fill_color_, stroke_color_, stroke_thickness_).draw(target);
    } else {
        RectPrimitive(layout_bounds, fill_color_).draw(target);
    }
    Column::draw(target);
}

void StyledPanel::apply_style(const gooey::mvvmc::Style& style) {
    fill_color_ = style.fill_color;
    stroke_color_ = style.stroke_color;
    stroke_thickness_ = style.stroke_thickness;
    corner_radius_ = style.corner_radius;
    Column::apply_style(style);
}
