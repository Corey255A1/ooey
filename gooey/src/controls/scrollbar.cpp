#include "gooey/controls/scrollbar.hpp"
#include <algorithm>

namespace gooey::controls {

ScrollBar::ScrollBar(Rect bounds, ScrollBarOrientation orientation)
    : bounds_(bounds), orientation_(orientation) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;
    set_style_name("scrollbar");

    track_prim_ = std::make_shared<RectPrimitive>(bounds_, track_color_);
    add_child(track_prim_);

    thumb_prim_ = std::make_shared<RoundedRectPrimitive>(Rect{0, 0, 0, 0}, 4, thumb_color_);
    add_child(thumb_prim_);

    update_thumb_bounds();
}

Rect ScrollBar::bounds() const {
    return bounds_;
}

void ScrollBar::set_range(int min_val, int max_val, int page_size) {
    min_val_ = min_val;
    max_val_ = max_val;
    page_size_ = page_size;

    // Clamp value to new range boundaries
    int max_allowed = std::max(min_val_, max_val_ - page_size_);
    value_ = std::clamp(value_, min_val_, max_allowed);

    update_thumb_bounds();
    invalidate_layout();
}

void ScrollBar::set_value(int value) {
    int max_allowed = std::max(min_val_, max_val_ - page_size_);
    int clamped = std::clamp(value, min_val_, max_allowed);
    if (value_ != clamped) {
        value_ = clamped;
        update_thumb_bounds();
        invalidate_layout();
    }
}

void ScrollBar::update_thumb_bounds() {
    int max_scroll_range = max_val_ - min_val_;
    int scrollable_value = max_val_ - page_size_ - min_val_;

    if (orientation_ == ScrollBarOrientation::Vertical) {
        int thumb_h = bounds_.height;
        if (max_scroll_range > 0) {
            thumb_h = (bounds_.height * page_size_) / max_scroll_range;
        }
        thumb_h = std::max(12, std::min(thumb_h, bounds_.height));

        int thumb_y = bounds_.y;
        int scrollable_track = bounds_.height - thumb_h;
        if (scrollable_value > 0 && scrollable_track > 0) {
            float ratio = static_cast<float>(value_ - min_val_) / scrollable_value;
            thumb_y = bounds_.y + static_cast<int>(ratio * scrollable_track);
        }

        thumb_bounds_ = Rect{bounds_.x + 2, thumb_y, std::max(4, bounds_.width - 4), thumb_h};
    } else {
        int thumb_w = bounds_.width;
        if (max_scroll_range > 0) {
            thumb_w = (bounds_.width * page_size_) / max_scroll_range;
        }
        thumb_w = std::max(12, std::min(thumb_w, bounds_.width));

        int thumb_x = bounds_.x;
        int scrollable_track = bounds_.width - thumb_w;
        if (scrollable_value > 0 && scrollable_track > 0) {
            float ratio = static_cast<float>(value_ - min_val_) / scrollable_value;
            thumb_x = bounds_.x + static_cast<int>(ratio * scrollable_track);
        }

        thumb_bounds_ = Rect{thumb_x, bounds_.y + 2, thumb_w, std::max(4, bounds_.height - 4)};
    }

    if (thumb_prim_) {
        thumb_prim_->set_rect(thumb_bounds_);
    }
}

bool ScrollBar::on_pointer_event(const Pointer& e) {
    if (e.state == PointerState::Pressed) {
        bool hit_thumb = (e.x >= thumb_bounds_.x && e.x <= thumb_bounds_.x + thumb_bounds_.width &&
                          e.y >= thumb_bounds_.y && e.y <= thumb_bounds_.y + thumb_bounds_.height);
        if (hit_thumb) {
            dragging_thumb_ = true;
            drag_start_offset_ = (orientation_ == ScrollBarOrientation::Vertical) ? (e.y - thumb_bounds_.y) : (e.x - thumb_bounds_.x);
            return true;
        } else {
            bool hit_track = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                              e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
            if (hit_track) {
                int scrollable_value = max_val_ - page_size_ - min_val_;
                if (scrollable_value > 0) {
                    int new_val = value_;
                    if (orientation_ == ScrollBarOrientation::Vertical) {
                        int click_offset = e.y - bounds_.y;
                        int thumb_h = thumb_bounds_.height;
                        int scrollable_track = bounds_.height - thumb_h;
                        if (scrollable_track > 0) {
                            int target_y = click_offset - thumb_h / 2;
                            float ratio = std::clamp(static_cast<float>(target_y) / scrollable_track, 0.0f, 1.0f);
                            new_val = min_val_ + static_cast<int>(ratio * scrollable_value);
                        }
                    } else {
                        int click_offset = e.x - bounds_.x;
                        int thumb_w = thumb_bounds_.width;
                        int scrollable_track = bounds_.width - thumb_w;
                        if (scrollable_track > 0) {
                            int target_x = click_offset - thumb_w / 2;
                            float ratio = std::clamp(static_cast<float>(target_x) / scrollable_track, 0.0f, 1.0f);
                            new_val = min_val_ + static_cast<int>(ratio * scrollable_value);
                        }
                    }
                    if (new_val != value_) {
                        set_value(new_val);
                        if (on_value_changed) {
                            on_value_changed(value_);
                        }
                    }
                }
                return true;
            }
        }
    } else if (e.state == PointerState::Released) {
        if (dragging_thumb_) {
            dragging_thumb_ = false;
            return true;
        }
    } else if (e.state == PointerState::Moved) {
        if (dragging_thumb_) {
            int scrollable_value = max_val_ - page_size_ - min_val_;
            if (scrollable_value > 0) {
                int new_val = value_;
                if (orientation_ == ScrollBarOrientation::Vertical) {
                    int target_y = e.y - bounds_.y - drag_start_offset_;
                    int scrollable_track = bounds_.height - thumb_bounds_.height;
                    if (scrollable_track > 0) {
                        float ratio = std::clamp(static_cast<float>(target_y) / scrollable_track, 0.0f, 1.0f);
                        new_val = min_val_ + static_cast<int>(ratio * scrollable_value);
                    }
                } else {
                    int target_x = e.x - bounds_.x - drag_start_offset_;
                    int scrollable_track = bounds_.width - thumb_bounds_.width;
                    if (scrollable_track > 0) {
                        float ratio = std::clamp(static_cast<float>(target_x) / scrollable_track, 0.0f, 1.0f);
                        new_val = min_val_ + static_cast<int>(ratio * scrollable_value);
                    }
                }
                if (new_val != value_) {
                    set_value(new_val);
                    if (on_value_changed) {
                        on_value_changed(value_);
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool ScrollBar::on_key_event(const ooey::KeyEvent& /*e*/) {
    return false;
}

Size ScrollBar::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, bounds_.width);
    int h = resolve_height(constraints.height, bounds_.height);
    return Size{w, h};
}

void ScrollBar::do_layout(Rect bounds) {
    bounds_ = bounds;
    if (track_prim_) {
        track_prim_->set_rect(bounds_);
    }
    update_thumb_bounds();
}

void ScrollBar::apply_style(const mvvmc::Style& style) {
    track_color_ = style.fill_color;
    if (style.stroke_color.a > 0) {
        thumb_color_ = style.stroke_color;
    }
    if (track_prim_) {
        track_prim_->set_fill_color(track_color_);
    }
    if (thumb_prim_) {
        thumb_prim_->set_fill_color(thumb_color_);
    }
}

} // namespace gooey::controls
