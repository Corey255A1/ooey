#include "gooey/controls/column.hpp"
#include <algorithm>
#include <vector>

namespace gooey::controls {

Size Column::do_measure(Size constraints) {
    int content_max_w = 0;

    int avail_w = std::max(0, constraints.width - padding_left - padding_right);
    int avail_h = std::max(0, constraints.height - padding_top - padding_bottom);

    int visible_child_count = 0;
    for (const auto& child : get_children()) {
        auto* child_elem = dynamic_cast<GooeyElement*>(child.get());
        if (child_elem && !child_elem->is_absolute) {
            visible_child_count++;
        }
    }

    int total_spacing = std::max(0, visible_child_count - 1) * spacing_;
    int avail_h_for_sizes = std::max(0, avail_h - total_spacing);

    int total_h_sizes = 0;
    float total_weight = 0.0f;
    std::vector<GooeyElement*> flex_children;

    // Pass 1: Measure non-flex children (fixed, wrap-content, match-parent, percentage)
    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<GooeyElement*>(child.get());
        if (child_view) {
            if (child_view->is_absolute) {
                child_view->measure(Size{avail_w, avail_h});
                int child_total_w = child_view->absolute_bounds.x + child_view->absolute_bounds.width;
                content_max_w = std::max(content_max_w, child_total_w);
                continue;
            }

            if (child_view->height.policy == SizePolicy::Flex) {
                total_weight += child_view->height.value;
                flex_children.push_back(child_view);
            } else {
                int child_constraint_h = std::max(0, avail_h_for_sizes - total_h_sizes);
                if (child_view->height.policy == SizePolicy::Percentage) {
                    child_constraint_h = static_cast<int>(avail_h * (child_view->height.value / 100.0f));
                } else if (child_view->height.policy == SizePolicy::Fixed) {
                    child_constraint_h = static_cast<int>(child_view->height.value);
                }
                
                Size child_size = child_view->measure(Size{avail_w, child_constraint_h});

                int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
                int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

                content_max_w = std::max(content_max_w, child_total_w);
                total_h_sizes += child_total_h;
            }
        }
    }

    // Pass 2: Distribute remaining space to flex children
    int remaining_h = std::max(0, avail_h_for_sizes - total_h_sizes);
    if (total_weight > 0.0f && remaining_h > 0) {
        for (auto* child_view : flex_children) {
            float weight = child_view->height.value;
            int child_share = static_cast<int>((weight / total_weight) * remaining_h);
            
            child_share = std::clamp(static_cast<float>(child_share), child_view->min_height, child_view->max_height);

            Size child_size = child_view->measure(Size{avail_w, child_share});
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

            content_max_w = std::max(content_max_w, child_total_w);
            total_h_sizes += child_total_h;
        }
    } else if (total_weight > 0.0f) {
        for (auto* child_view : flex_children) {
            Size child_size = child_view->measure(Size{avail_w, 0});
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            content_max_w = std::max(content_max_w, child_total_w);
            total_h_sizes += child_view->margin_top + child_view->margin_bottom;
        }
    }

    int final_w = resolve_width(constraints.width, content_max_w + padding_left + padding_right);
    int final_h = resolve_height(constraints.height, total_h_sizes + total_spacing + padding_top + padding_bottom);
    return Size{final_w, final_h};
}

void Column::do_layout(Rect bounds) {
    layout_bounds = bounds;

    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);

    int total_children_h = 0;
    int visible_child_count = 0;
    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<GooeyElement*>(child.get());
        if (child_view && !child_view->is_absolute) {
            total_children_h += child_view->get_measured_size().height + child_view->margin_top + child_view->margin_bottom;
            visible_child_count++;
        }
    }

    int total_spacing = std::max(0, visible_child_count - 1) * spacing_;
    int total_allocated_h = total_children_h + total_spacing;

    int start_y = bounds.y + padding_top;
    int spacing_override = spacing_;

    if (justify_content == Justify::Center) {
        start_y += (content_h - total_allocated_h) / 2;
    } else if (justify_content == Justify::End) {
        start_y += (content_h - total_allocated_h);
    } else if (justify_content == Justify::SpaceBetween) {
        if (visible_child_count > 1) {
            spacing_override = (content_h - total_children_h) / (visible_child_count - 1);
        }
    } else if (justify_content == Justify::SpaceAround) {
        if (visible_child_count > 0) {
            float gap_size = static_cast<float>(content_h - total_children_h) / static_cast<float>(visible_child_count);
            start_y += static_cast<int>(gap_size / 2.0f);
            spacing_override = static_cast<int>(gap_size);
        }
    } else if (justify_content == Justify::SpaceEvenly) {
        if (visible_child_count > 0) {
            int gap = (content_h - total_children_h) / (visible_child_count + 1);
            start_y += gap;
            spacing_override = gap;
        }
    }

    int current_y = start_y;
    bool first = true;
    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<GooeyElement*>(child.get());
        if (child_view) {
            if (child_view->is_absolute) {
                int cx = bounds.x + padding_left + child_view->absolute_bounds.x;
                int cy = bounds.y + padding_top + child_view->absolute_bounds.y;
                child_view->layout(Rect{cx, cy, child_view->absolute_bounds.width, child_view->absolute_bounds.height});
                continue;
            }

            if (!first) {
                current_y += spacing_override;
            }

            int child_w = child_view->get_measured_size().width;
            int child_h = child_view->get_measured_size().height;

            Align align = child_view->align_self;
            if (align == Align::Inherit) {
                align = align_items;
                if (align == Align::Inherit) {
                    align = Align::Start;
                }
            }

            int cx = bounds.x + padding_left + child_view->margin_left;
            int cy = current_y + child_view->margin_top;

            if (align == Align::Stretch) {
                child_w = content_w - (child_view->margin_left + child_view->margin_right);
            } else if (align == Align::Center) {
                cx += (content_w - child_w - child_view->margin_left - child_view->margin_right) / 2;
            } else if (align == Align::End) {
                cx += (content_w - child_w - child_view->margin_right);
            }

            child_view->layout(Rect{cx, cy, child_w, child_h});

            current_y += child_h + child_view->margin_top + child_view->margin_bottom;
            first = false;
        }
    }
}

} // namespace gooey::controls
