namespace ooey {}

#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/theme.hpp"
#include <algorithm>

namespace gooey::mvvmc {
    using namespace ooey;

View::View() = default;

void View::add_child(std::shared_ptr<IDrawable>&& child) {
    auto* child_view = dynamic_cast<View*>(child.get());
    if (child_view) {
        child_view->set_theme_manager(get_theme_manager());
    }
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

int View::calculate_content_width(Size child_constraints) {
    int max_child_w = 0;
    for (const auto& child : children_) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            Size child_size = child_view->measure(child_constraints);
            int child_w = child_view->is_absolute ? (child_view->absolute_bounds.x + child_view->absolute_bounds.width)
                                                  : (child_size.width + child_view->margin_left + child_view->margin_right);
            max_child_w = std::max(max_child_w, child_w);
        }
    }
    return max_child_w;
}

int View::calculate_content_height(Size child_constraints) {
    int max_child_h = 0;
    for (const auto& child : children_) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            Size child_size = child_view->measure(child_constraints);
            int child_h = child_view->is_absolute ? (child_view->absolute_bounds.y + child_view->absolute_bounds.height)
                                                  : (child_size.height + child_view->margin_top + child_view->margin_bottom);
            max_child_h = std::max(max_child_h, child_h);
        }
    }
    return max_child_h;
}

Size View::measure(Size constraints) {
    Size child_constraints{std::max(0, constraints.width - padding_left - padding_right), 
                           std::max(0, constraints.height - padding_top - padding_bottom)};
    
    int max_child_w = calculate_content_width(child_constraints);
    int w = resolve_width(constraints.width, max_child_w + padding_left + padding_right);

    int max_child_h = calculate_content_height(child_constraints);
    int h = resolve_height(constraints.height, max_child_h + padding_top + padding_bottom);

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

int View::resolve_width(int constraint_w, int content_w) const {
    int w = 0;
    if (width.policy == SizePolicy::Fixed) {
        w = static_cast<int>(width.value);
    } else if (width.policy == SizePolicy::MatchParent) {
        w = constraint_w;
    } else {
        w = content_w;
    }
    return std::max(0, std::min(w, constraint_w));
}

int View::resolve_height(int constraint_h, int content_h) const {
    int h = 0;
    if (height.policy == SizePolicy::Fixed) {
        h = static_cast<int>(height.value);
    } else if (height.policy == SizePolicy::MatchParent) {
        h = constraint_h;
    } else {
        h = content_h;
    }
    return std::max(0, std::min(h, constraint_h));
}

void View::set_style_name(const std::string& name) {
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

void View::set_theme_manager(std::shared_ptr<ThemeManager> manager) {
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

    for (const auto& child : children_) {
        auto* child_view = dynamic_cast<View*>(child.get());
        if (child_view) {
            child_view->set_theme_manager(manager);
        }
    }
}

void View::apply_style(const Style& /*style*/) {
}

} // namespace gooey::mvvmc