#include "gooey/controls/scroll_container.hpp"
#include <algorithm>
#include <utility>

namespace gooey::controls {

ScrollContainer::ScrollContainer() {
    clip_children = true;

    v_scroll_ = std::make_shared<ScrollBar>(Rect{0, 0, 12, 100}, ScrollBarOrientation::Vertical);
    v_scroll_->on_value_changed = [this](int value) {
        set_scroll_offset_y(value);
    };
    add_child(v_scroll_);

    h_scroll_ = std::make_shared<ScrollBar>(Rect{0, 0, 100, 12}, ScrollBarOrientation::Horizontal);
    h_scroll_->on_value_changed = [this](int value) {
        set_scroll_offset_x(value);
    };
    add_child(h_scroll_);
}

void ScrollContainer::set_child(std::shared_ptr<GooeyNode> child) {
    child_ = std::move(child);
    clear_children();
    if (child_) {
        add_child(child_);
    }
    add_child(v_scroll_);
    add_child(h_scroll_);

    scroll_offset_x_ = 0;
    scroll_offset_y_ = 0;
    max_scroll_x_ = 0;
    max_scroll_y_ = 0;
    needs_scroll_x_ = false;
    needs_scroll_y_ = false;
    invalidate_layout();
}

void ScrollContainer::set_scroll_offset_x(int offset) {
    int clamped = std::clamp(offset, 0, max_scroll_x_);
    if (scroll_offset_x_ != clamped) {
        scroll_offset_x_ = clamped;
        invalidate_layout();
    }
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
        needs_scroll_x_ = false;
        needs_scroll_y_ = false;

        // Measure child with unconstrained size initially
        Size child_size = child_->measure(Size{100000, 100000});

        bool changed = true;
        int iterations = 0;
        while (changed && iterations < 3) {
            changed = false;
            bool new_needs_y = (child_size.height > avail_h - (needs_scroll_x_ ? 12 : 0));
            bool new_needs_x = (child_size.width > avail_w - (needs_scroll_y_ ? 12 : 0));

            if (new_needs_y != needs_scroll_y_) {
                needs_scroll_y_ = new_needs_y;
                changed = true;
            }
            if (new_needs_x != needs_scroll_x_) {
                needs_scroll_x_ = new_needs_x;
                changed = true;
            }

            if (changed) {
                int cw = 100000;
                int ch = 100000;
                
                if (child_->width.policy == SizePolicy::MatchParent) {
                    cw = std::max(0, avail_w - (needs_scroll_y_ ? 12 : 0));
                }
                if (child_->height.policy == SizePolicy::MatchParent) {
                    ch = std::max(0, avail_h - (needs_scroll_x_ ? 12 : 0));
                }
                child_size = child_->measure(Size{cw, ch});
            }
            iterations++;
        }

        int viewport_w = avail_w - (needs_scroll_y_ ? 12 : 0);
        int viewport_h = avail_h - (needs_scroll_x_ ? 12 : 0);

        max_scroll_x_ = needs_scroll_x_ ? std::max(0, child_size.width - viewport_w) : 0;
        max_scroll_y_ = needs_scroll_y_ ? std::max(0, child_size.height - viewport_h) : 0;

        scroll_offset_x_ = needs_scroll_x_ ? std::clamp(scroll_offset_x_, 0, max_scroll_x_) : 0;
        scroll_offset_y_ = needs_scroll_y_ ? std::clamp(scroll_offset_y_, 0, max_scroll_y_) : 0;

        child_measured_size_ = child_size;
    } else {
        needs_scroll_x_ = false;
        needs_scroll_y_ = false;
        max_scroll_x_ = 0;
        max_scroll_y_ = 0;
        scroll_offset_x_ = 0;
        scroll_offset_y_ = 0;
        child_measured_size_ = Size{0, 0};
    }

    if (needs_scroll_y_) {
        int v_h = avail_h - (needs_scroll_x_ ? 12 : 0);
        v_scroll_->measure(Size{12, v_h});
    }
    if (needs_scroll_x_) {
        int h_w = avail_w - (needs_scroll_y_ ? 12 : 0);
        h_scroll_->measure(Size{h_w, 12});
    }

    int w = resolve_width(constraints.width, child_measured_size_.width + padding_left + padding_right + (needs_scroll_y_ ? 12 : 0));
    int h = resolve_height(constraints.height, child_measured_size_.height + padding_top + padding_bottom + (needs_scroll_x_ ? 12 : 0));
    return Size{w, h};
}

void ScrollContainer::do_layout(Rect bounds) {
    layout_bounds = bounds;

    int content_w = std::max(0, bounds.width - padding_left - padding_right);
    int content_h = std::max(0, bounds.height - padding_top - padding_bottom);

    int viewport_w = content_w - (needs_scroll_y_ ? 12 : 0);
    int viewport_h = content_h - (needs_scroll_x_ ? 12 : 0);

    if (needs_scroll_y_) {
        scroll_offset_y_ = std::clamp(scroll_offset_y_, 0, max_scroll_y_);
        v_scroll_->set_range(0, max_scroll_y_ + viewport_h, viewport_h);
        v_scroll_->set_value(scroll_offset_y_);
        v_scroll_->layout(Rect{bounds.x + bounds.width - padding_right - 12, bounds.y + padding_top, 12, viewport_h});
    } else {
        v_scroll_->layout(Rect{0, 0, 0, 0});
    }

    if (needs_scroll_x_) {
        scroll_offset_x_ = std::clamp(scroll_offset_x_, 0, max_scroll_x_);
        h_scroll_->set_range(0, max_scroll_x_ + viewport_w, viewport_w);
        h_scroll_->set_value(scroll_offset_x_);
        h_scroll_->layout(Rect{bounds.x + padding_left, bounds.y + bounds.height - padding_bottom - 12, viewport_w, 12});
    } else {
        h_scroll_->layout(Rect{0, 0, 0, 0});
    }

    if (child_) {
        int cx = bounds.x + padding_left - scroll_offset_x_;
        int cy = bounds.y + padding_top - scroll_offset_y_;

        int cw = (child_->width.policy == SizePolicy::MatchParent && !needs_scroll_x_) ? viewport_w : child_measured_size_.width;
        int ch = (child_->height.policy == SizePolicy::MatchParent && !needs_scroll_y_) ? viewport_h : child_measured_size_.height;

        child_->layout(Rect{cx, cy, cw, ch});
    }
}

void ScrollContainer::scroll_to_visible(Rect rect) {
    int content_w = std::max(0, layout_bounds.width - padding_left - padding_right);
    int content_h = std::max(0, layout_bounds.height - padding_top - padding_bottom);

    int viewport_w = content_w - (needs_scroll_y_ ? 12 : 0);
    int viewport_h = content_h - (needs_scroll_x_ ? 12 : 0);

    int left_margin = 20;
    int right_margin = 20;
    int top_margin = 20;
    int bottom_margin = 20;

    if (needs_scroll_y_) {
        if (rect.y < scroll_offset_y_ + top_margin) {
            set_scroll_offset_y(std::max(0, rect.y - top_margin));
        } else if (rect.y + rect.height > scroll_offset_y_ + viewport_h - bottom_margin) {
            set_scroll_offset_y(std::max(0, rect.y + rect.height - viewport_h + bottom_margin));
        }
        v_scroll_->set_value(scroll_offset_y_);
    }

    if (needs_scroll_x_) {
        if (rect.x < scroll_offset_x_ + left_margin) {
            set_scroll_offset_x(std::max(0, rect.x - left_margin));
        } else if (rect.x + rect.width > scroll_offset_x_ + viewport_w - right_margin) {
            set_scroll_offset_x(std::max(0, rect.x + rect.width - viewport_w + right_margin));
        }
        h_scroll_->set_value(scroll_offset_x_);
    }
}

bool ScrollContainer::on_pointer_event(const Pointer& e) {
    if (e.state == PointerState::Pressed) {
        if (e.x >= layout_bounds.x && e.x <= layout_bounds.x + layout_bounds.width &&
            e.y >= layout_bounds.y && e.y <= layout_bounds.y + layout_bounds.height) {

            // Check if user clicked on scrollbars
            bool hit_v_scrollbar = needs_scroll_y_ && (e.x >= layout_bounds.x + layout_bounds.width - padding_right - 12);
            bool hit_h_scrollbar = needs_scroll_x_ && (e.y >= layout_bounds.y + layout_bounds.height - padding_bottom - 12);

            if (!hit_v_scrollbar && !hit_h_scrollbar) {
                dragging_content_ = true;
                drag_start_x_ = e.x;
                drag_start_y_ = e.y;
                drag_start_offset_x_ = scroll_offset_x_;
                drag_start_offset_y_ = scroll_offset_y_;
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
            int dx = e.x - drag_start_x_;
            int dy = e.y - drag_start_y_;
            if (needs_scroll_x_) {
                set_scroll_offset_x(drag_start_offset_x_ - dx);
                h_scroll_->set_value(scroll_offset_x_);
            }
            if (needs_scroll_y_) {
                set_scroll_offset_y(drag_start_offset_y_ - dy);
                v_scroll_->set_value(scroll_offset_y_);
            }
            return true;
        }
    }
    return false;
}

} // namespace gooey::controls
