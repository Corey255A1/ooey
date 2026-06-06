#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include <algorithm>

namespace gooey::mvvmc {

GooeyElement::GooeyElement() = default;

Size GooeyElement::measure(Size constraints) {
    if (is_measure_clean_ && 
        constraints.width == last_measure_constraints_.width && 
        constraints.height == last_measure_constraints_.height) {
        return measured_size_;
    }
    measured_size_ = do_measure(constraints);
    last_measure_constraints_ = constraints;
    is_measure_clean_ = true;
    return measured_size_;
}

Size GooeyElement::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, padding_left + padding_right);
    int h = resolve_height(constraints.height, padding_top + padding_bottom);
    return Size{w, h};
}

void GooeyElement::layout(Rect bounds) {
    if (is_layout_clean_ && 
        bounds.x == layout_bounds.x && 
        bounds.y == layout_bounds.y && 
        bounds.width == layout_bounds.width && 
        bounds.height == layout_bounds.height) {
        return;
    }
    do_layout(bounds);
    layout_bounds = bounds;
    is_layout_clean_ = true;
}

void GooeyElement::do_layout(Rect bounds) {
    layout_bounds = bounds;
}

void GooeyElement::invalidate_layout() {
    is_measure_clean_ = false;
    is_layout_clean_ = false;
    if (parent_) {
        parent_->invalidate_layout();
    }
}

int GooeyElement::resolve_width(int constraint_w, int content_w) const {
    int w = 0;
    if (width.policy == SizePolicy::Fixed) {
        w = static_cast<int>(width.value);
    } else if (width.policy == SizePolicy::Percentage) {
        w = static_cast<int>(constraint_w * (width.value / 100.0f));
    } else if (width.policy == SizePolicy::Flex) {
        w = constraint_w;
    } else if (width.policy == SizePolicy::MatchParent) {
        if (constraint_w >= 50000) {
            w = content_w;
        } else {
            w = constraint_w;
        }
    } else {
        w = content_w;
    }
    w = std::clamp(static_cast<float>(w), min_width, max_width);
    return std::max(0, std::min(w, constraint_w));
}

int GooeyElement::resolve_height(int constraint_h, int content_h) const {
    int h = 0;
    if (height.policy == SizePolicy::Fixed) {
        h = static_cast<int>(height.value);
    } else if (height.policy == SizePolicy::Percentage) {
        h = static_cast<int>(constraint_h * (height.value / 100.0f));
    } else if (height.policy == SizePolicy::Flex) {
        h = constraint_h;
    } else if (height.policy == SizePolicy::MatchParent) {
        if (constraint_h >= 50000) {
            h = content_h;
        } else {
            h = constraint_h;
        }
    } else {
        h = content_h;
    }
    h = std::clamp(static_cast<float>(h), min_height, max_height);
    return std::max(0, std::min(h, constraint_h));
}

void GooeyElement::set_style_name(const std::string& name) {
    style_name_ = name;
    auto manager = get_theme_manager();
    if (manager) {
        auto theme = manager->active_theme.get();
        if (theme) {
            Style style;
            if (theme->get_style(style_name_, style)) {
                apply_style(style);
            }
        }
    }
}

void GooeyElement::set_theme_manager(std::shared_ptr<ThemeManager> manager) {
    theme_manager_ = manager;
    if (manager) {
        theme_subscription_ = manager->active_theme.subscribe([this](const std::shared_ptr<Theme>& theme) {
            if (theme && !style_name_.empty()) {
                Style style;
                if (theme->get_style(style_name_, style)) {
                    apply_style(style);
                }
            }
        });
    } else {
        theme_subscription_ = {};
    }
}

void GooeyElement::apply_style(const Style& /*style*/) {
}

} // namespace gooey::mvvmc
