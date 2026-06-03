#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/theme.hpp"
#include <algorithm>

namespace gooey::mvvmc {
    using namespace ooey;

GooeyNode::GooeyNode() = default;

void GooeyNode::add_child(std::shared_ptr<IDrawable>&& child) {
    auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
    if (child_elem) {
        child_elem->set_theme_manager(get_theme_manager());
        child_elem->parent_ = this;
    }
    children_.push_back(std::move(child));
    invalidate_layout();
}

const std::vector<std::shared_ptr<IDrawable>>& GooeyNode::get_children() const {
    return children_;
}

void GooeyNode::remove_child(const std::shared_ptr<IDrawable>& child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        auto* child_elem = dynamic_cast<GooeyElement*>(it->get());
        if (child_elem) {
            child_elem->parent_ = nullptr;
        }
        children_.erase(it);
        invalidate_layout();
    }
}

void GooeyNode::draw(ooey::IRenderTarget& target) const {
    if (clip_children) {
        target.push_clip(layout_bounds);
    }
    for (const auto& child : children_) {
        child->draw(target);
    }
    if (clip_children) {
        target.pop_clip();
    }
}

void GooeyNode::clear_children() {
    for (const auto& child : children_) {
        auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
        if (child_elem) {
            child_elem->parent_ = nullptr;
        }
    }
    children_.clear();
    invalidate_layout();
}

int GooeyNode::calculate_content_width(Size child_constraints) {
    int max_child_w = 0;
    for (const auto& child : children_) {
        auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
        if (child_elem) {
            Size child_size = child_elem->measure(child_constraints);
            int child_w = child_elem->is_absolute ? (child_elem->absolute_bounds.x + child_elem->absolute_bounds.width)
                                                  : (child_size.width + child_elem->margin_left + child_elem->margin_right);
            max_child_w = std::max(max_child_w, child_w);
        }
    }
    return max_child_w;
}

int GooeyNode::calculate_content_height(Size child_constraints) {
    int max_child_h = 0;
    for (const auto& child : children_) {
        auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
        if (child_elem) {
            Size child_size = child_elem->measure(child_constraints);
            int child_h = child_elem->is_absolute ? (child_elem->absolute_bounds.y + child_elem->absolute_bounds.height)
                                                  : (child_size.height + child_elem->margin_top + child_elem->margin_bottom);
            max_child_h = std::max(max_child_h, child_h);
        }
    }
    return max_child_h;
}

Size GooeyNode::do_measure(Size constraints) {
    Size child_constraints{std::max(0, constraints.width - padding_left - padding_right), 
                           std::max(0, constraints.height - padding_top - padding_bottom)};
    
    int max_child_w = calculate_content_width(child_constraints);
    int w = resolve_width(constraints.width, max_child_w + padding_left + padding_right);

    int max_child_h = calculate_content_height(child_constraints);
    int h = resolve_height(constraints.height, max_child_h + padding_top + padding_bottom);

    return Size{w, h};
}

void GooeyNode::do_layout(Rect bounds) {
    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);
    Size child_constraints{content_w, content_h};

    for (const auto& child : children_) {
        auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
        if (child_elem) {
            if (child_elem->is_absolute) {
                int cx = bounds.x + padding_left + child_elem->absolute_bounds.x;
                int cy = bounds.y + padding_top + child_elem->absolute_bounds.y;
                child_elem->layout(Rect{cx, cy, child_elem->absolute_bounds.width, child_elem->absolute_bounds.height});
                continue;
            }

            Size child_size = child_elem->measure(child_constraints);

            // Default (Overlay layout): align child in top-left with margin
            int cx = bounds.x + padding_left + child_elem->margin_left;
            int cy = bounds.y + padding_top + child_elem->margin_top;

            // Handle align_self stretch alignment
            int cw = child_size.width;
            int ch = child_size.height;
            if (child_elem->align_self == Align::Stretch) {
                cw = content_w - (child_elem->margin_left + child_elem->margin_right);
            }

            child_elem->layout(Rect{cx, cy, cw, ch});
        }
    }
}

void GooeyNode::set_theme_manager(std::shared_ptr<ThemeManager> manager) {
    GooeyElement::set_theme_manager(manager);
    for (const auto& child : children_) {
        auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
        if (child_elem) {
            child_elem->set_theme_manager(manager);
        }
    }
}

} // namespace gooey::mvvmc