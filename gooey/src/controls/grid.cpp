#include "gooey/controls/grid.hpp"
#include <algorithm>

namespace gooey::controls {

Grid::Grid(int rows, int columns) : rows_(std::max(1, rows)), columns_(std::max(1, columns)) {}

Size Grid::measure(Size constraints) {
    int final_w = 0;
    if (width.policy == SizePolicy::Fixed) {
        final_w = static_cast<int>(width.value);
    } else {
        final_w = constraints.width;
    }

    int final_h = 0;
    if (height.policy == SizePolicy::Fixed) {
        final_h = static_cast<int>(height.value);
    } else {
        final_h = constraints.height;
    }

    final_w = std::max(0, std::min(final_w, constraints.width));
    final_h = std::max(0, std::min(final_h, constraints.height));
    return Size{final_w, final_h};
}

void Grid::layout(Rect bounds) {
    layout_bounds = bounds;

    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);

    int cell_w = content_w / columns_;
    int cell_h = content_h / rows_;

    int index = 0;
    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            int row = index / columns_;
            int col = index % columns_;

            if (row >= rows_) break; // Grid overflow protection

            int cx = bounds.x + padding_left + col * cell_w + child_view->margin_left;
            int cy = bounds.y + padding_top + row * cell_h + child_view->margin_top;

            int cw = cell_w - (child_view->margin_left + child_view->margin_right);
            int ch = cell_h - (child_view->margin_top + child_view->margin_bottom);

            // Respect child measured sizes if they don't stretch
            Size child_constraints{std::max(0, cw), std::max(0, ch)};
            Size child_size = child_view->measure(child_constraints);

            if (child_view->align_self != Align::Stretch) {
                cw = child_size.width;
                ch = child_size.height;
            }

            child_view->layout(Rect{cx, cy, std::max(0, cw), std::max(0, ch)});
            index++;
        }
    }
}

} // namespace gooey::controls
