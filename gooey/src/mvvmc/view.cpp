namespace ooey {}

#include "gooey/mvvmc/view.hpp"
#include <algorithm>

namespace gooey::mvvmc {
    using namespace ooey;

void View::add_child(std::shared_ptr<IDrawable>&& child) {
    children_.push_back(std::move(child));
}

const std::vector<std::shared_ptr<IDrawable>>& View::get_children() const {
    return children_;
}

void View::draw(ooey::IRenderTarget& target) const {
    for (const auto& child : children_) {
        child->draw(target);
    }
}

void View::clear_children() {
    children_.clear();
}

Size View::measure(Size constraints) {
    // 1. Calculate width
    int w = 0;
    if (width.policy == SizePolicy::Fixed) {
        w = static_cast<int>(width.value);
    } else if (width.policy == SizePolicy::MatchParent) {
        w = constraints.width;
    } else { // WrapContent
        int max_child_w = 0;
        Size child_constraints{std::max(0, constraints.width - padding_left - padding_right), 
                               std::max(0, constraints.height - padding_top - padding_bottom)};
        for (const auto& child : children_) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                Size child_size = child_view->measure(child_constraints);
                int child_w = child_view->is_absolute ? (child_view->absolute_bounds.x + child_view->absolute_bounds.width)
                                                      : (child_size.width + child_view->margin_left + child_view->margin_right);
                max_child_w = std::max(max_child_w, child_w);
            }
        }
        w = max_child_w + padding_left + padding_right;
    }

    // 2. Calculate height
    int h = 0;
    if (height.policy == SizePolicy::Fixed) {
        h = static_cast<int>(height.value);
    } else if (height.policy == SizePolicy::MatchParent) {
        h = constraints.height;
    } else { // WrapContent
        int max_child_h = 0;
        Size child_constraints{std::max(0, constraints.width - padding_left - padding_right), 
                               std::max(0, constraints.height - padding_top - padding_bottom)};
        for (const auto& child : children_) {
            auto* child_view = dynamic_cast<View*>(child.get());
            if (child_view) {
                Size child_size = child_view->measure(child_constraints);
                int child_h = child_view->is_absolute ? (child_view->absolute_bounds.y + child_view->absolute_bounds.height)
                                                      : (child_size.height + child_view->margin_top + child_view->margin_bottom);
                max_child_h = std::max(max_child_h, child_h);
            }
        }
        h = max_child_h + padding_top + padding_bottom;
    }

    // Clamp values to parent boundaries
    w = std::max(0, std::min(w, constraints.width));
    h = std::max(0, std::min(h, constraints.height));
    return Size{w, h};
}

void View::layout(Rect bounds) {
    layout_bounds = bounds;
    
    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);
    Size child_constraints{content_w, content_h};

    for (const auto& child : children_) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            if (child_view->is_absolute) {
                int cx = bounds.x + padding_left + child_view->absolute_bounds.x;
                int cy = bounds.y + padding_top + child_view->absolute_bounds.y;
                child_view->layout(Rect{cx, cy, child_view->absolute_bounds.width, child_view->absolute_bounds.height});
                continue;
            }

            Size child_size = child_view->measure(child_constraints);

            // Default (Overlay layout): align child in top-left with margin
            int cx = bounds.x + padding_left + child_view->margin_left;
            int cy = bounds.y + padding_top + child_view->margin_top;

            // Handle align_self stretch alignment
            int cw = child_size.width;
            int ch = child_size.height;
            if (child_view->align_self == Align::Stretch) {
                cw = content_w - (child_view->margin_left + child_view->margin_right);
            }

            child_view->layout(Rect{cx, cy, cw, ch});
        }
    }
}

} // namespace gooey::mvvmc