#include "gooey/controls/flow_layout.hpp"
#include <algorithm>
#include <vector>

namespace gooey::controls {

Size FlowLayout::measure(Size constraints) {
    int current_row_w = 0;
    int current_row_h = 0;
    int max_row_width = 0;
    int total_height = 0;
    int avail_w = std::max(0, constraints.width - padding_left - padding_right);

    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            Size child_size = child_view->measure(Size{avail_w, std::max(0, constraints.height - padding_top - padding_bottom)});
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

            if (current_row_w + child_total_w > avail_w && current_row_w > 0) {
                // Wrap to next row
                max_row_width = std::max(max_row_width, current_row_w);
                total_height += current_row_h;
                current_row_w = child_total_w;
                current_row_h = child_total_h;
            } else {
                current_row_w += child_total_w;
                current_row_h = std::max(current_row_h, child_total_h);
            }
        }
    }
    max_row_width = std::max(max_row_width, current_row_w);
    total_height += current_row_h;

    int final_w = 0;
    if (width.policy == SizePolicy::Fixed) {
        final_w = static_cast<int>(width.value);
    } else if (width.policy == SizePolicy::MatchParent) {
        final_w = constraints.width;
    } else {
        final_w = max_row_width + padding_left + padding_right;
    }

    int final_h = 0;
    if (height.policy == SizePolicy::Fixed) {
        final_h = static_cast<int>(height.value);
    } else if (height.policy == SizePolicy::MatchParent) {
        final_h = constraints.height;
    } else {
        final_h = total_height + padding_top + padding_bottom;
    }

    final_w = std::max(0, std::min(final_w, constraints.width));
    final_h = std::max(0, std::min(final_h, constraints.height));
    return Size{final_w, final_h};
}

void FlowLayout::layout(Rect bounds) {
    layout_bounds = bounds;

    int avail_w = std::max(0, bounds.width - padding_left - padding_right);
    int avail_h = std::max(0, bounds.height - padding_top - padding_bottom);

    struct RowInfo {
        std::vector<View*> views;
        int height{0};
        int width{0};
    };

    std::vector<RowInfo> rows;
    RowInfo current_row;
    int current_row_w = 0;

    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            Size child_size = child_view->measure(Size{avail_w, avail_h});
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

            if (current_row_w + child_total_w > avail_w && !current_row.views.empty()) {
                rows.push_back(current_row);
                current_row = RowInfo{};
                current_row_w = 0;
            }

            current_row.views.push_back(child_view);
            current_row.width += child_total_w;
            current_row.height = std::max(current_row.height, child_total_h);
            current_row_w += child_total_w;
        }
    }
    if (!current_row.views.empty()) {
        rows.push_back(current_row);
    }

    int current_y = bounds.y + padding_top;
    for (const auto& r : rows) {
        int current_x = bounds.x + padding_left;
        for (auto* child_view : r.views) {
            Size child_size = child_view->measure(Size{avail_w, r.height});
            int child_w = child_size.width;
            int child_h = child_size.height;

            if (child_view->align_self == Align::Stretch) {
                child_h = r.height - (child_view->margin_top + child_view->margin_bottom);
            }

            int cx = current_x + child_view->margin_left;
            int cy = current_y + child_view->margin_top;

            child_view->layout(Rect{cx, cy, child_w, child_h});

            current_x += child_w + child_view->margin_left + child_view->margin_right;
        }
        current_y += r.height;
    }
}

} // namespace gooey::controls
