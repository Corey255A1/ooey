namespace ooey {}

#include "gooey/controls/list_control.hpp"
#include "gooey/mvvmc/theme.hpp"
#include <algorithm>
#include <unordered_set>

namespace gooey::controls {
    using namespace ooey;

namespace {
    const char* keep_alive_family(const std::string& family) {
        static std::unordered_set<std::string> families;
        return families.insert(family).first->c_str();
    }
}

ListControl::ListControl() : ListControl(Rect{0, 0, 200, 200}, 40, Font{"sans-serif", 16}, Color{200, 200, 200}, Color{35, 35, 40}, Color{0, 120, 215}, Color{255, 255, 255}) {}

ListControl::ListControl(Rect bounds, int item_height, Font font, Color text_color, Color bg_color, Color highlight_bg_color, Color highlight_text_color)
    : bounds_(bounds), item_height_(item_height), font_(font), text_color_(text_color), bg_color_(bg_color),
      highlight_bg_color_(highlight_bg_color), highlight_text_color_(highlight_text_color) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;
    
    // Calculate how many items can be displayed based on list height and item height
    visible_count_ = item_height_ > 0 ? bounds_.height / item_height_ : 1;
    if (visible_count_ <= 0) {
        visible_count_ = 1;
    }

    // Create list box background primitive
    bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, 6, bg_color_, Color{100, 100, 110}, 1.5f);
}

Rect ListControl::bounds() const {
    return bounds_;
}

void ListControl::set_items(const std::vector<std::string>& items) {
    // Clear any previous view bindings
    for (const auto& old_view : item_views_) {
        if (old_view) {
            old_view->set_parent(nullptr);
            remove_child(old_view);
        }
    }
    item_views_.clear();

    items_ = items;
    // Reset selection if it goes out of bounds
    if (selected_index_ >= static_cast<int>(items_.size())) {
        selected_index_ = items_.empty() ? -1 : static_cast<int>(items_.size()) - 1;
    }
    
    // Reset scroll if out of bounds
    if (scroll_offset_ + visible_count_ > static_cast<int>(items_.size())) {
        scroll_offset_ = std::max(0, static_cast<int>(items_.size()) - visible_count_);
    }

    invalidate_layout();
}

const std::vector<std::string>& ListControl::get_items() const {
    return items_;
}

void ListControl::set_item_views(const std::vector<std::shared_ptr<GooeyElement>>& views) {
    // 1. Sever parent-child linkages for all previous views (preventing dangling parent pointers)
    for (const auto& old_view : item_views_) {
        if (old_view) {
            old_view->set_parent(nullptr);
            remove_child(old_view);
        }
    }
    
    // 2. Assign new views and establish linkage
    item_views_ = views;
    for (const auto& view : item_views_) {
        if (view) {
            view->set_parent(this);
            view->set_theme_manager(get_theme_manager());
            add_child(std::shared_ptr<IDrawable>(view));
        }
    }
    
    if (selected_index_ >= static_cast<int>(item_views_.size())) {
        selected_index_ = item_views_.empty() ? -1 : static_cast<int>(item_views_.size()) - 1;
    }
    if (scroll_offset_ + visible_count_ > static_cast<int>(item_views_.size())) {
        scroll_offset_ = std::max(0, static_cast<int>(item_views_.size()) - visible_count_);
    }
    
    invalidate_layout();
}

const std::vector<std::shared_ptr<GooeyElement>>& ListControl::get_item_views() const {
    return item_views_;
}

void ListControl::set_item_height(int height) {
    if (item_height_ != height) {
        item_height_ = height;
        visible_count_ = item_height_ > 0 ? bounds_.height / item_height_ : 1;
        if (visible_count_ <= 0) {
            visible_count_ = 1;
        }
        invalidate_layout();
    }
}

int ListControl::get_item_height() const {
    return item_height_;
}

void ListControl::set_selected_index(int index) {
    size_t total_items = item_views_.empty() ? items_.size() : item_views_.size();
    if (index < -1 || index >= static_cast<int>(total_items)) {
        return;
    }
    if (selected_index_ == index) {
        return;
    }
    selected_index_ = index;

    if (selected_index_ != -1) {
        // Keep selected element visible (adjust scroll_offset_)
        if (selected_index_ < scroll_offset_) {
            scroll_offset_ = selected_index_;
        } else if (selected_index_ >= scroll_offset_ + visible_count_) {
            scroll_offset_ = selected_index_ - (visible_count_ - 1);
        }
    }

    update_children();
    invalidate_layout();

    if (on_selected_changed) {
        on_selected_changed(selected_index_);
    }
}

int ListControl::get_selected_index() const {
    return selected_index_;
}

void ListControl::select_next() {
    size_t total_items = item_views_.empty() ? items_.size() : item_views_.size();
    if (selected_index_ + 1 < static_cast<int>(total_items)) {
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
                size_t total_items = item_views_.empty() ? items_.size() : item_views_.size();
                if (target_index < static_cast<int>(total_items)) {
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
    if (!item_views_.empty()) {
        for (int i = 0; i < visible_count_; ++i) {
            int item_idx = scroll_offset_ + i;
            if (item_idx < static_cast<int>(item_views_.size()) && i < static_cast<int>(item_bgs_.size()) && item_bgs_[i]) {
                if (item_idx == selected_index_) {
                    item_bgs_[i]->set_fill_color(highlight_bg_color_);
                } else {
                    item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
                }
            }
        }
        return;
    }

    for (int i = 0; i < visible_count_; ++i) {
        int item_idx = scroll_offset_ + i;
        if (item_idx < static_cast<int>(items_.size())) {
            const std::string& raw_item = items_[item_idx];
            bool has_checkbox = false;
            bool is_checked = false;
            std::string text_to_display = raw_item;

            // Check if string starts with checkbox patterns
            if (raw_item.starts_with("[✓]  ") || raw_item.starts_with("[✓] ")) {
                has_checkbox = true;
                is_checked = true;
                text_to_display = raw_item.substr(raw_item.starts_with("[✓]  ") ? 5 : 4);
            } else if (raw_item.starts_with("[ ]  ") || raw_item.starts_with("[ ] ")) {
                has_checkbox = true;
                is_checked = false;
                text_to_display = raw_item.substr(raw_item.starts_with("[ ]  ") ? 5 : 4);
            } else if (raw_item.starts_with("[X]  ") || raw_item.starts_with("[X] ") ||
                       raw_item.starts_with("[x]  ") || raw_item.starts_with("[x] ")) {
                has_checkbox = true;
                is_checked = true;
                text_to_display = raw_item.substr(raw_item.starts_with("[X]  ") || raw_item.starts_with("[x]  ") ? 5 : 4);
            }

            if (i < static_cast<int>(item_texts_.size()) && item_texts_[i]) {
                item_texts_[i]->set_text(text_to_display);

                if (stylize_items_) {
                    item_texts_[i]->set_font(Font{keep_alive_family(text_to_display), font_.size, font_.weight, font_.style});
                } else {
                    item_texts_[i]->set_font(font_);
                }

                if (item_idx == selected_index_) {
                    if (i < static_cast<int>(item_bgs_.size()) && item_bgs_[i]) {
                        item_bgs_[i]->set_fill_color(highlight_bg_color_);
                    }
                    item_texts_[i]->set_color(highlight_text_color_);
                } else {
                    if (i < static_cast<int>(item_bgs_.size()) && item_bgs_[i]) {
                        item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
                    }
                    item_texts_[i]->set_color(text_color_);
                }

                // Update checkbox positioning and visibility
                int item_y = bounds_.y + i * item_height_;
                if (has_checkbox && i < static_cast<int>(item_checkbox_bgs_.size()) && item_checkbox_bgs_[i]) {
                    item_texts_[i]->set_position(Point{bounds_.x + 35, item_y + (item_height_ - font_.size) / 2});

                    item_checkbox_bgs_[i]->set_fill_color(Color{30, 30, 35});
                    item_checkbox_bgs_[i]->set_stroke_color(Color{150, 150, 155});
                    item_checkbox_bgs_[i]->set_stroke_thickness(1.5f);

                    if (is_checked && i < static_cast<int>(item_checkbox_checks_.size()) && item_checkbox_checks_[i]) {
                        item_checkbox_checks_[i]->set_fill_color(Color{0, 120, 215}); // accent blue
                    } else if (i < static_cast<int>(item_checkbox_checks_.size()) && item_checkbox_checks_[i]) {
                        item_checkbox_checks_[i]->set_fill_color(Color{0, 0, 0, 0});
                    }
                } else {
                    item_texts_[i]->set_position(Point{bounds_.x + 10, item_y + (item_height_ - font_.size) / 2});

                    if (i < static_cast<int>(item_checkbox_bgs_.size()) && item_checkbox_bgs_[i]) {
                        item_checkbox_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
                        item_checkbox_bgs_[i]->set_stroke_color(Color{0, 0, 0, 0});
                        item_checkbox_bgs_[i]->set_stroke_thickness(0.0f);
                        if (i < static_cast<int>(item_checkbox_checks_.size()) && item_checkbox_checks_[i]) {
                            item_checkbox_checks_[i]->set_fill_color(Color{0, 0, 0, 0});
                        }
                    }
                }
            }
        } else {
            if (i < static_cast<int>(item_texts_.size()) && item_texts_[i]) {
                item_texts_[i]->set_text("");
            }
            if (i < static_cast<int>(item_bgs_.size()) && item_bgs_[i]) {
                item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
            }
            if (i < static_cast<int>(item_checkbox_bgs_.size()) && item_checkbox_bgs_[i]) {
                item_checkbox_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
                item_checkbox_bgs_[i]->set_stroke_color(Color{0, 0, 0, 0});
                item_checkbox_bgs_[i]->set_stroke_thickness(0.0f);
                if (i < static_cast<int>(item_checkbox_checks_.size()) && item_checkbox_checks_[i]) {
                    item_checkbox_checks_[i]->set_fill_color(Color{0, 0, 0, 0});
                }
            }
        }
    }
}

Size ListControl::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, absolute_bounds.width);
    int h = resolve_height(constraints.height, absolute_bounds.height);
    
    // Measure children so they have valid sizes during layout
    if (!item_views_.empty()) {
        for (auto& view : item_views_) {
            if (view) {
                view->measure(Size{w - 4, item_height_ - 4});
            }
        }
    }
    return Size{w, h};
}

void ListControl::do_layout(Rect bounds) {
    bounds_ = bounds;
    GooeyNode::do_layout(bounds);

    if (bg_) {
        bg_->set_rect(bounds_);
    } else {
        bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, 6, bg_color_, Color{100, 100, 110}, 1.5f);
    }

    visible_count_ = item_height_ > 0 ? bounds_.height / item_height_ : 1;
    if (visible_count_ <= 0) {
        visible_count_ = 1;
    }

    if (item_views_.empty()) {
        if (static_cast<int>(item_bgs_.size()) != visible_count_) {
            item_bgs_.resize(visible_count_);
            item_checkbox_bgs_.resize(visible_count_);
            item_checkbox_checks_.resize(visible_count_);
            item_texts_.resize(visible_count_);
        }

        for (int i = 0; i < visible_count_; ++i) {
            int item_y = bounds_.y + i * item_height_;
            Rect item_rect{bounds_.x + 2, item_y + 2, bounds_.width - 4, item_height_ - 4};
            
            if (!item_bgs_[i]) {
                item_bgs_[i] = std::make_shared<RectPrimitive>(item_rect, Color{0, 0, 0, 0});
            } else {
                item_bgs_[i]->set_rect(item_rect);
            }

            int box_size = 18;
            int by = item_y + (item_height_ - box_size) / 2;
            Rect box_rect{bounds_.x + 10, by, box_size, box_size};
            if (!item_checkbox_bgs_[i]) {
                item_checkbox_bgs_[i] = std::make_shared<RoundedRectPrimitive>(box_rect, 4, Color{30, 30, 35}, Color{150, 150, 155}, 1.5f);
            } else {
                item_checkbox_bgs_[i]->set_rect(box_rect);
            }

            Rect check_rect{bounds_.x + 14, by + 4, box_size - 8, box_size - 8};
            if (!item_checkbox_checks_[i]) {
                item_checkbox_checks_[i] = std::make_shared<RectPrimitive>(check_rect, Color{0, 120, 215});
            } else {
                item_checkbox_checks_[i]->set_rect(check_rect);
            }

            Point text_pos{bounds_.x + 35, item_y + (item_height_ - font_.size) / 2};
            if (!item_texts_[i]) {
                item_texts_[i] = std::make_shared<TextPrimitive>("", font_, text_pos, text_color_);
            } else {
                item_texts_[i]->set_position(text_pos);
            }
        }
        update_children();
    } else {
        if (static_cast<int>(item_bgs_.size()) != visible_count_) {
            item_bgs_.resize(visible_count_);
        }
        for (int i = 0; i < visible_count_; ++i) {
            int item_y = bounds_.y + i * item_height_;
            Rect item_rect{bounds_.x + 2, item_y + 2, bounds_.width - 4, item_height_ - 4};
            if (!item_bgs_[i]) {
                item_bgs_[i] = std::make_shared<RectPrimitive>(item_rect, Color{0, 0, 0, 0});
            } else {
                item_bgs_[i]->set_rect(item_rect);
            }
            
            int item_idx = scroll_offset_ + i;
            if (item_idx < static_cast<int>(item_views_.size())) {
                if (item_idx == selected_index_) {
                    item_bgs_[i]->set_fill_color(highlight_bg_color_);
                } else {
                    item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
                }
            } else {
                item_bgs_[i]->set_fill_color(Color{0, 0, 0, 0});
            }
        }

        // Layout the child views
        for (size_t idx = 0; idx < item_views_.size(); ++idx) {
            auto& view = item_views_[idx];
            if (view) {
                int visible_pos = static_cast<int>(idx) - scroll_offset_;
                if (visible_pos >= 0 && visible_pos < visible_count_) {
                    int item_y = bounds_.y + visible_pos * item_height_;
                    Rect item_rect{bounds_.x + 2, item_y + 2, bounds_.width - 4, item_height_ - 4};
                    view->layout(item_rect);
                } else {
                    view->layout(Rect{0, 0, 0, 0}); // off screen
                }
            }
        }
    }
}

void ListControl::draw(ooey::IRenderTarget& target) const {
    if (bg_) {
        bg_->draw(target);
    }
    
    if (item_views_.empty()) {
        for (int i = 0; i < visible_count_; ++i) {
            int item_idx = scroll_offset_ + i;
            if (item_idx < static_cast<int>(items_.size())) {
                if (i < static_cast<int>(item_bgs_.size()) && item_bgs_[i]) {
                    item_bgs_[i]->draw(target);
                }
                if (i < static_cast<int>(item_checkbox_bgs_.size()) && item_checkbox_bgs_[i]) {
                    item_checkbox_bgs_[i]->draw(target);
                }
                if (i < static_cast<int>(item_checkbox_checks_.size()) && item_checkbox_checks_[i]) {
                    item_checkbox_checks_[i]->draw(target);
                }
                if (i < static_cast<int>(item_texts_.size()) && item_texts_[i]) {
                    item_texts_[i]->draw(target);
                }
            }
        }
    } else {
        // Draw the background highlights for selection
        for (int i = 0; i < visible_count_; ++i) {
            int item_idx = scroll_offset_ + i;
            if (item_idx < static_cast<int>(item_views_.size())) {
                if (i < static_cast<int>(item_bgs_.size()) && item_bgs_[i]) {
                    item_bgs_[i]->draw(target);
                }
            }
        }
        
        // Draw only the visible child views
        if (clip_children) {
            target.push_clip(layout_bounds);
        }
        for (size_t idx = 0; idx < item_views_.size(); ++idx) {
            int visible_pos = static_cast<int>(idx) - scroll_offset_;
            if (visible_pos >= 0 && visible_pos < visible_count_) {
                if (item_views_[idx]) {
                    item_views_[idx]->draw(target);
                }
            }
        }
        if (clip_children) {
            target.pop_clip();
        }
    }
}

void ListControl::set_stylize_items(bool stylize) {
    if (stylize_items_ != stylize) {
        stylize_items_ = stylize;
        update_children();
    }
}

bool ListControl::get_stylize_items() const {
    return stylize_items_;
}

void ListControl::apply_style(const mvvmc::Style& style) {
    bg_color_ = style.fill_color;
    text_color_ = style.text_color;
    if (bg_) {
        bg_->set_fill_color(style.fill_color);
        bg_->set_stroke_color(style.stroke_color);
        bg_->set_stroke_thickness(style.stroke_thickness);
        bg_->set_corner_radius(style.corner_radius);
    }
    highlight_bg_color_ = style.stroke_color;
    highlight_text_color_ = style.fill_color;
    
    update_children();
    GooeyNode::apply_style(style);
}

} // namespace gooey::controls
