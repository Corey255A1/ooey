#include "gooey/controls/column.hpp"
#include <algorithm>

namespace gooey::controls {

Size Column::measure(Size constraints) {
    int content_max_w = 0;
    int total_h = 0;

    int avail_w = std::max(0, constraints.width - padding_left - padding_right);
    int avail_h = std::max(0, constraints.height - padding_top - padding_bottom);
    Size child_constraints{avail_w, avail_h};

    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            Size child_size = child_view->measure(child_constraints);
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

            content_max_w = std::max(content_max_w, child_total_w);
            total_h += child_total_h;

            avail_h = std::max(0, avail_h - child_total_h);
            child_constraints.height = avail_h;
        }
    }

    int final_w = 0;
    if (width.policy == SizePolicy::Fixed) {
        final_w = static_cast<int>(width.value);
    } else if (width.policy == SizePolicy::MatchParent) {
        final_w = constraints.width;
    } else {
        final_w = content_max_w + padding_left + padding_right;
    }

    int final_h = 0;
    if (height.policy == SizePolicy::Fixed) {
        final_h = static_cast<int>(height.value);
    } else if (height.policy == SizePolicy::MatchParent) {
        final_h = constraints.height;
    } else {
        final_h = total_h + padding_top + padding_bottom;
    }

    final_w = std::max(0, std::min(final_w, constraints.width));
    final_h = std::max(0, std::min(final_h, constraints.height));
    return Size{final_w, final_h};
}

void Column::layout(Rect bounds) {
    layout_bounds = bounds;

    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int current_y = bounds.y + padding_top;
    Size child_constraints{content_w, std::max(0, bounds.height - padding_top - padding_bottom)};

    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            child_constraints.height = std::max(0, bounds.y + bounds.height - padding_bottom - current_y);
            Size child_size = child_view->measure(child_constraints);

            int child_w = child_size.width;
            int child_h = child_size.height;
            if (child_view->align_self == Align::Stretch) {
                child_w = content_w - (child_view->margin_left + child_view->margin_right);
            }

            int cx = bounds.x + padding_left + child_view->margin_left;
            int cy = current_y + child_view->margin_top;

            child_view->layout(Rect{cx, cy, child_w, child_h});

            current_y += child_h + child_view->margin_top + child_view->margin_bottom;
        }
    }
}

} // namespace gooey::controls
