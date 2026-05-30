#include "gooey/controls/row.hpp"
#include <algorithm>

namespace gooey::controls {

Size Row::measure(Size constraints) {
    int content_max_h = 0;
    int total_w = 0;

    int avail_w = std::max(0, constraints.width - padding_left - padding_right);
    int avail_h = std::max(0, constraints.height - padding_top - padding_bottom);
    Size child_constraints{avail_w, avail_h};

    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            Size child_size = child_view->measure(child_constraints);
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

            content_max_h = std::max(content_max_h, child_total_h);
            total_w += child_total_w;

            avail_w = std::max(0, avail_w - child_total_w);
            child_constraints.width = avail_w;
        }
    }

    int final_w = resolve_width(constraints.width, total_w + padding_left + padding_right);
    int final_h = resolve_height(constraints.height, content_max_h + padding_top + padding_bottom);
    return Size{final_w, final_h};
}

void Row::layout(Rect bounds) {
    layout_bounds = bounds;

    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);
    int current_x = bounds.x + padding_left;
    Size child_constraints{std::max(0, bounds.width - padding_left - padding_right), content_h};

    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            child_constraints.width = std::max(0, bounds.x + bounds.width - padding_right - current_x);
            Size child_size = child_view->measure(child_constraints);

            int child_w = child_size.width;
            int child_h = child_size.height;
            if (child_view->align_self == Align::Stretch) {
                child_h = content_h - (child_view->margin_top + child_view->margin_bottom);
            }

            int cx = current_x + child_view->margin_left;
            int cy = bounds.y + padding_top + child_view->margin_top;

            child_view->layout(Rect{cx, cy, child_w, child_h});

            current_x += child_w + child_view->margin_left + child_view->margin_right;
        }
    }
}

} // namespace gooey::controls
