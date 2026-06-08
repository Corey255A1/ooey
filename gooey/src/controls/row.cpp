#include "gooey/controls/row.hpp"
#include <algorithm>
#include <vector>

namespace gooey::controls {

Size Row::do_measure(Size constraints) {
    int content_max_h = 0;

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
    int avail_w_for_sizes = std::max(0, avail_w - total_spacing);

    int total_w_sizes = 0;
    float total_weight = 0.0f;
    std::vector<GooeyElement*> flex_children;

    // Pass 1: Measure non-flex children (fixed, wrap-content, match-parent, percentage)
    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<GooeyElement*>(child.get());
        if (child_view) {
            if (child_view->is_absolute) {
                child_view->measure(Size{avail_w, avail_h});
                int child_total_h = child_view->absolute_bounds.y + child_view->absolute_bounds.height;
                content_max_h = std::max(content_max_h, child_total_h);
                continue;
            }

            if (child_view->width.policy == SizePolicy::Flex) {
                total_weight += child_view->width.value;
                flex_children.push_back(child_view);
            } else {
                int child_constraint_w = std::max(0, avail_w_for_sizes - total_w_sizes);
                if (child_view->width.policy == SizePolicy::Percentage) {
                    child_constraint_w = static_cast<int>(avail_w * (child_view->width.value / 100.0f));
                } else if (child_view->width.policy == SizePolicy::Fixed) {
                    child_constraint_w = static_cast<int>(child_view->width.value);
                }
                
                Size child_size = child_view->measure(Size{child_constraint_w, avail_h});

                int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
                int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

                content_max_h = std::max(content_max_h, child_total_h);
                total_w_sizes += child_total_w;
            }
        }
    }

    // Pass 2: Distribute remaining space to flex children
    int remaining_w = std::max(0, avail_w_for_sizes - total_w_sizes);
    if (total_weight > 0.0f && remaining_w > 0) {
        for (auto* child_view : flex_children) {
            float weight = child_view->width.value;
            int child_share = static_cast<int>((weight / total_weight) * remaining_w);
            
            child_share = std::clamp(static_cast<float>(child_share), child_view->min_width, child_view->max_width);

            Size child_size = child_view->measure(Size{child_share, avail_h});
            int child_total_w = child_size.width + child_view->margin_left + child_view->margin_right;
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;

            content_max_h = std::max(content_max_h, child_total_h);
            total_w_sizes += child_total_w;
        }
    } else if (total_weight > 0.0f) {
        for (auto* child_view : flex_children) {
            Size child_size = child_view->measure(Size{0, avail_h});
            int child_total_h = child_size.height + child_view->margin_top + child_view->margin_bottom;
            content_max_h = std::max(content_max_h, child_total_h);
            total_w_sizes += child_view->margin_left + child_view->margin_right;
        }
    }

    int final_w = resolve_width(constraints.width, total_w_sizes + total_spacing + padding_left + padding_right);
    int final_h = resolve_height(constraints.height, content_max_h + padding_top + padding_bottom);
    return Size{final_w, final_h};
}

void Row::do_layout(Rect bounds) {
    layout_bounds = bounds;

    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);

    int total_children_w = 0;
    int visible_child_count = 0;
    for (const auto& child : get_children()) {
        auto* child_view = dynamic_cast<GooeyElement*>(child.get());
        if (child_view && !child_view->is_absolute) {
            total_children_w += child_view->get_measured_size().width + child_view->margin_left + child_view->margin_right;
            visible_child_count++;
        }
    }

    int total_spacing = std::max(0, visible_child_count - 1) * spacing_;
    int total_allocated_w = total_children_w + total_spacing;

    int start_x = bounds.x + padding_left;
    int spacing_override = spacing_;

    if (justify_content == Justify::Center) {
        start_x += (content_w - total_allocated_w) / 2;
    } else if (justify_content == Justify::End) {
        start_x += (content_w - total_allocated_w);
    } else if (justify_content == Justify::SpaceBetween) {
        if (visible_child_count > 1) {
            spacing_override = (content_w - total_children_w) / (visible_child_count - 1);
        }
    } else if (justify_content == Justify::SpaceAround) {
        if (visible_child_count > 0) {
            float gap_size = static_cast<float>(content_w - total_children_w) / static_cast<float>(visible_child_count);
            start_x += static_cast<int>(gap_size / 2.0f);
            spacing_override = static_cast<int>(gap_size);
        }
    } else if (justify_content == Justify::SpaceEvenly) {
        if (visible_child_count > 0) {
            int gap = (content_w - total_children_w) / (visible_child_count + 1);
            start_x += gap;
            spacing_override = gap;
        }
    }

    int current_x = start_x;
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
                current_x += spacing_override;
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

            int cx = current_x + child_view->margin_left;
            int cy = bounds.y + padding_top + child_view->margin_top;

            if (align == Align::Stretch) {
                child_h = content_h - (child_view->margin_top + child_view->margin_bottom);
            } else if (align == Align::Center) {
                cy += (content_h - child_h - child_view->margin_top - child_view->margin_bottom) / 2;
            } else if (align == Align::End) {
                cy += (content_h - child_h - child_view->margin_bottom);
            }

            child_view->layout(Rect{cx, cy, child_w, child_h});

            current_x += child_w + child_view->margin_left + child_view->margin_right;
            first = false;
        }
    }
}

} // namespace gooey::controls
