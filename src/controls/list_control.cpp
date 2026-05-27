#include "ooey/controls/list_control.hpp"
#include <algorithm>

namespace ooey {

ListControl::ListControl(Rect bounds, int item_height, Font font, Color text_color, Color bg_color, Color highlight_bg_color, Color highlight_text_color)
    : bounds_(bounds), item_height_(item_height), font_(font), text_color_(text_color), bg_color_(bg_color),
      highlight_bg_color_(highlight_bg_color), highlight_text_color_(highlight_text_color) {
    
    // Calculate how many items can be displayed based on list height and item height
    visible_count_ = item_height_ > 0 ? bounds_.height / item_height_ : 1;
    if (visible_count_ <= 0) {
        visible_count_ = 1;
    }

    // Create list box background primitive
    bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, 6, bg_color_, Color{100, 100, 110}, 1.5f);
    add_child(bg_);

    for (int i = 0; i < visible_count_; ++i) {
        int item_y = bounds_.y + i * item_height_;
        
        // Item slot background primitive (slightly inset)
        Rect item_rect{bounds_.x + 2, item_y + 2, bounds_.width - 4, item_height_ - 4};
        auto item_bg = std::make_shared<RectPrimitive>(item_rect, Color{0, 0, 0, 0});
        item_bgs_.push_back(item_bg);
        add_child(item_bg);

        // Item text primitive (centered vertically)
        Point text_pos{bounds_.x + 10, item_y + (item_height_ - font_.size) / 2};
        auto item_text = std::make_shared<TextPrimitive>("", font_, text_pos, text_color_);
        item_texts_.push_back(item_text);
        add_child(item_text);
    }
}

Rect ListControl::bounds() const {
    return bounds_;
}

void ListControl::set_items(const std::vector<std::string>& items) {
    items_ = items;
    // Reset selection if it goes out of bounds
    if (selected_index_ >= static_cast<int>(items_.size())) {
        selected_index_ = items_.empty() ? 0 : static_cast<int>(items_.size()) - 1;
    }
    
    // Reset scroll if out of bounds
    if (scroll_offset_ + visible_count_ > static_cast<int>(items_.size())) {
        scroll_offset_ = std::max(0, static_cast<int>(items_.size()) - visible_count_);
    }

    update_children();
}

const std::vector<std::string>& ListControl::get_items() const {
    return items_;
}

void ListControl::set_selected_index(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    selected_index_ = index;

    // Keep selected element visible (adjust scroll_offset_)
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + visible_count_) {
        scroll_offset_ = selected_index_ - (visible_count_ - 1);
    }

    update_children();

    if (on_selected_changed) {
        on_selected_changed(selected_index_);
    }
}

int ListControl::get_selected_index() const {
    return selected_index_;
}

void ListControl::select_next() {
    if (selected_index_ + 1 < static_cast<int>(items_.size())) {
        set_selected_index(selected_index_ + 1);
    }
}

void ListControl::select_previous() {
    if (selected_index_ - 1 >= 0) {
        set_selected_index(selected_index_ - 1);
    }
}

bool ListControl::on_pointer_event(const Pointer& e) {
    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
    
    if (hit && e.state == PointerState::Pressed) {
        if (item_height_ > 0) {
            int clicked_visible_index = (e.y - bounds_.y) / item_height_;
            if (clicked_visible_index >= 0 && clicked_visible_index < visible_count_) {
                int target_index = scroll_offset_ + clicked_visible_index;
                if (target_index < static_cast<int>(items_.size())) {
                    set_selected_index(target_index);
                }
            }
        }
        return true;
    }
    return false;
}

bool ListControl::on_key_event(const KeyEvent& e) {
    if (e.state == KeyState::Pressed) {
        if (e.key_code == 0xFF52 /* XK_Up */) {
            select_previous();
            return true;
        } else if (e.key_code == 0xFF54 /* XK_Down */) {
            select_next();
            return true;
        }
    }
    return false;
}

void ListControl::update_children() {
    for (int i = 0; i < visible_count_; ++i) {
        int item_idx = scroll_offset_ + i;
        if (item_idx < static_cast<int>(items_.size())) {
            item_texts_[i]->set_text(items_[item_idx]);
            if (item_idx == selected_index_) {
                item_bgs_[i]->set_fill_color(highlight_bg_color_);
                item_texts_[i]->set_color(highlight_text_color_);
            } else {
                item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
                item_texts_[i]->set_color(text_color_);
            }
        } else {
            item_texts_[i]->set_text("");
            item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
        }
    }
}

} // namespace ooey
