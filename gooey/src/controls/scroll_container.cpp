#include "gooey/controls/scroll_container.hpp"
#include <algorithm>

namespace gooey::controls {

ScrollContainer::ScrollContainer() {
    clip_children = true;
    v_scroll_ = std::make_shared<ScrollBar>(Rect{0, 0, 12, 100}, ScrollBarOrientation::Vertical);
    v_scroll_->on_value_changed = [this](int value) {
        set_scroll_offset_y(value);
    };
    add_child(v_scroll_);
}

void ScrollContainer::set_child(std::shared_ptr<View> child) {
    child_ = child;
    clear_children();
    add_child(v_scroll_);
    if (child_) {
        add_child(child_);
    }
    scroll_offset_y_ = 0;
    max_scroll_y_ = 0;
    needs_scroll_ = false;
    invalidate_layout();
}

void ScrollContainer::set_scroll_offset_y(int offset) {
    int clamped = std::clamp(offset, 0, max_scroll_y_);
    if (scroll_offset_y_ != clamped) {
        scroll_offset_y_ = clamped;
        invalidate_layout();
    }
}

Size ScrollContainer::do_measure(Size constraints) {
    int avail_w = std::max(0, constraints.width - padding_left - padding_right);
    int avail_h = std::max(0, constraints.height - padding_top - padding_bottom);

    if (child_) {
        // Measure child with unconstrained height
        Size child_constraints{avail_w, 100000};
        Size child_size = child_->measure(child_constraints);

        if (child_size.height > avail_h) {
            needs_scroll_ = true;
            // Subtract scrollbar width (12px) and re-measure
            child_constraints.width = std::max(0, avail_w - 12);
            child_size = child_->measure(child_constraints);
            max_scroll_y_ = child_size.height - avail_h;
        } else {
            needs_scroll_ = false;
            max_scroll_y_ = 0;
            scroll_offset_y_ = 0;
        }
        child_measured_size_ = child_size;
    } else {
        needs_scroll_ = false;
        max_scroll_y_ = 0;
        scroll_offset_y_ = 0;
        child_measured_size_ = Size{0, 0};
    }

    if (needs_scroll_) {
        v_scroll_->measure(Size{12, avail_h});
    }

    return constraints;
}

void ScrollContainer::do_layout(Rect bounds) {
    layout_bounds = bounds;

    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);

    if (needs_scroll_) {
        scroll_offset_y_ = std::clamp(scroll_offset_y_, 0, max_scroll_y_);

        v_scroll_->set_range(0, max_scroll_y_ + content_h, content_h);
        v_scroll_->set_value(scroll_offset_y_);
        v_scroll_->layout(Rect{bounds.x + bounds.width - padding_right - 12, bounds.y + padding_top, 12, content_h});

        if (child_) {
            int child_h = (child_->height.policy == SizePolicy::MatchParent) ? (max_scroll_y_ + content_h) : child_measured_size_.height;
            child_->layout(Rect{bounds.x + padding_left, bounds.y + padding_top - scroll_offset_y_, content_w - 12, child_h});
        }
    } else {
        if (child_) {
            int child_h = (child_->height.policy == SizePolicy::MatchParent) ? content_h : child_measured_size_.height;
            child_->layout(Rect{bounds.x + padding_left, bounds.y + padding_top, content_w, child_h});
        }
        v_scroll_->layout(Rect{0, 0, 0, 0});
    }
}

bool ScrollContainer::on_pointer_event(const Pointer& e) {
    // Intercept clicks/touches for drag-to-scroll
    if (e.state == PointerState::Pressed) {
        if (e.x >= layout_bounds.x && e.x <= layout_bounds.x + layout_bounds.width &&
            e.y >= layout_bounds.y && e.y <= layout_bounds.y + layout_bounds.height) {
            
            // Check if user clicked on scrollbar
            bool hit_scrollbar = needs_scroll_ && (e.x >= layout_bounds.x + layout_bounds.width - padding_right - 12);
            if (!hit_scrollbar) {
                dragging_content_ = true;
                drag_start_y_ = e.y;
                drag_start_offset_ = scroll_offset_y_;
                return true;
            }
        }
    } else if (e.state == PointerState::Released) {
        if (dragging_content_) {
            dragging_content_ = false;
            return true;
        }
    } else if (e.state == PointerState::Moved) {
        if (dragging_content_) {
            int dy = e.y - drag_start_y_;
            int new_offset = drag_start_offset_ - dy;
            set_scroll_offset_y(new_offset);
            v_scroll_->set_value(scroll_offset_y_);
            return true;
        }
    }
    return false;
}

} // namespace gooey::controls
