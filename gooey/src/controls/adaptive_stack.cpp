#include "gooey/controls/adaptive_stack.hpp"
#include <algorithm>

namespace gooey::controls {

Size AdaptiveStack::do_measure(Size constraints) {
    if (constraints.width > 0 && constraints.width <= breakpoint_) {
        // COLUMN MEASUREMENT (Vertical)
        int content_max_w = 0;
        int total_h = 0;

        int avail_w = std::max(0, constraints.width - padding_left - padding_right);
        int avail_h = std::max(0, constraints.height - padding_top - padding_bottom);

        // First pass: measure non-MatchParent height children
        int non_flex_h = 0;
        int flex_count = 0;
        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                if (child_view->height.policy != SizePolicy::MatchParent) {
                    Size child_size = child_view->measure(Size{avail_w, avail_h});
                    non_flex_h += child_size.height + child_view->margin_top + child_view->margin_bottom;
                } else {
                    flex_count++;
                }
            }
        }

        int remaining_h = std::max(0, avail_h - non_flex_h);
        int per_flex_h = flex_count > 0 ? (remaining_h / flex_count) : 0;
        if (per_flex_h <= 0) {
            per_flex_h = 220; // Fallback default height for flex children in unconstrained space
        }

        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                Size child_constraints{avail_w, avail_h};
                if (child_view->height.policy == SizePolicy::MatchParent) {
                    child_constraints.height = std::max(0, per_flex_h - child_view->margin_top - child_view->margin_bottom);
                }
                if (stretch_when_vertical_) {
                    child_constraints.width = std::max(0, avail_w - child_view->margin_left - child_view->margin_right);
                }

                Size child_size = child_view->measure(child_constraints);
                int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
                int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

                content_max_w = std::max(content_max_w, child_total_w);
                total_h += child_total_h;

                if (child_view->height.policy != SizePolicy::MatchParent) {
                    avail_h = std::max(0, avail_h - child_total_h);
                }
            }
        }

        int final_w = resolve_width(constraints.width, content_max_w + padding_left + padding_right);
        int final_h = resolve_height(constraints.height, total_h + padding_top + padding_bottom);
        return Size{final_w, final_h};
    } else {
        // ROW MEASUREMENT (Horizontal)
        int content_max_h = 0;
        int total_w = 0;

        int avail_w = std::max(0, constraints.width - padding_left - padding_right);
        int avail_h = std::max(0, constraints.height - padding_top - padding_bottom);

        // First pass: measure non-MatchParent width children
        int non_flex_w = 0;
        int flex_count = 0;
        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                if (child_view->width.policy != SizePolicy::MatchParent) {
                    Size child_size = child_view->measure(Size{avail_w, avail_h});
                    non_flex_w += child_size.width + child_view->margin_left + child_view->margin_right;
                } else {
                    flex_count++;
                }
            }
        }

        int remaining_w = std::max(0, avail_w - non_flex_w);
        int per_flex_w = flex_count > 0 ? (remaining_w / flex_count) : 0;

        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                Size child_constraints{avail_w, avail_h};
                if (child_view->width.policy == SizePolicy::MatchParent) {
                    child_constraints.width = std::max(0, per_flex_w - child_view->margin_left - child_view->margin_right);
                }

                Size child_size = child_view->measure(child_constraints);
                int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
                int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

                content_max_h = std::max(content_max_h, child_total_h);
                total_w += child_total_w;

                if (child_view->width.policy != SizePolicy::MatchParent) {
                    avail_w = std::max(0, avail_w - child_total_w);
                }
            }
        }

        int final_w = resolve_width(constraints.width, total_w + padding_left + padding_right);
        int final_h = resolve_height(constraints.height, content_max_h + padding_top + padding_bottom);
        return Size{final_w, final_h};
    }
}

void AdaptiveStack::do_layout(Rect bounds) {
    layout_bounds = bounds;

    if (bounds.width > 0 && bounds.width <= breakpoint_) {
        // COLUMN LAYOUT (Vertical)
        int content_w = std::max(0, bounds.width - padding_left - padding_right);
        int avail_h = std::max(0, bounds.height - padding_top - padding_bottom);
        int current_y = bounds.y + padding_top;

        // First pass: measure non-MatchParent height children
        int non_flex_h = 0;
        int flex_count = 0;
        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                if (child_view->height.policy != SizePolicy::MatchParent) {
                    Size child_size = child_view->measure(Size{content_w, avail_h});
                    non_flex_h += child_size.height + child_view->margin_top + child_view->margin_bottom;
                } else {
                    flex_count++;
                }
            }
        }

        int remaining_h = std::max(0, avail_h - non_flex_h);
        int per_flex_h = flex_count > 0 ? (remaining_h / flex_count) : 0;
        if (per_flex_h <= 0) {
            per_flex_h = 220;
        }

        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                Size child_constraints{content_w, avail_h};
                if (child_view->height.policy == SizePolicy::MatchParent) {
                    child_constraints.height = std::max(0, per_flex_h - child_view->margin_top - child_view->margin_bottom);
                }
                if (stretch_when_vertical_) {
                    child_constraints.width = std::max(0, content_w - child_view->margin_left - child_view->margin_right);
                }

                Size child_size = child_view->measure(child_constraints);

                int child_w = child_size.width;
                int child_h = child_size.height;
                
                if (stretch_when_vertical_ || child_view->align_self == Align::Stretch) {
                    child_w = content_w - (child_view->margin_left + child_view->margin_right);
                }

                int cx = bounds.x + padding_left + child_view->margin_left;
                int cy = current_y + child_view->margin_top;

                child_view->layout(Rect{cx, cy, child_w, child_h});

                current_y += child_h + child_view->margin_top + child_view->margin_bottom;
            }
        }
    } else {
        // ROW LAYOUT (Horizontal)
        int content_h = std::max(0, bounds.height - padding_top - padding_bottom);
        int current_x = bounds.x + padding_left;
        int avail_w = std::max(0, bounds.width - padding_left - padding_right);

        // First, measure non-MatchParent width children
        int non_flex_w = 0;
        int flex_count = 0;
        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                if (child_view->width.policy != SizePolicy::MatchParent) {
                    Size child_size = child_view->measure(Size{avail_w, content_h});
                    non_flex_w += child_size.width + child_view->margin_left + child_view->margin_right;
                } else {
                    flex_count++;
                }
            }
        }

        int remaining_w = std::max(0, avail_w - non_flex_w);
        int per_flex_w = flex_count > 0 ? (remaining_w / flex_count) : 0;

        for (const auto& child : get_children()) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                Size child_constraints{avail_w, content_h};
                if (child_view->width.policy == SizePolicy::MatchParent) {
                    child_constraints.width = std::max(0, per_flex_w - child_view->margin_left - child_view->margin_right);
                }
                
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
}

} // namespace gooey::controls
