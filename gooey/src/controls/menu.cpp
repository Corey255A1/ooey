#include "gooey/controls/menu.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "gooey/application.hpp"
#include <algorithm>

namespace gooey::controls {

Menu::Menu(const std::vector<MenuItem>& items)
    : items_(items) {
    set_absolute(true);
}

Size Menu::do_measure(Size constraints) {
    // Determine total height and width
    width_ = 180;
    int h = 0;
    for (const auto& item : items_) {
        if (item.is_separator) {
            h += 10;
        } else {
            h += row_height_;
        }
    }
    return Size{width_, h};
}

void Menu::do_layout(Rect bounds) {
    bounds_ = bounds;
}

void Menu::draw(ooey::IRenderTarget& target) const {
    if (!is_open_) return;

    // Focus validation: close menu hierarchy if focus is completely lost
    auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(
        gooey::Application::get_instance()->get_controller());
    if (controller) {
        auto focused = controller->get_focused_element();
        bool has_focus = false;
        
        // Check down the submenu chain
        auto curr = const_cast<Menu*>(this)->shared_from_this();
        while (curr) {
            if (focused == curr) {
                has_focus = true;
                break;
            }
            curr = curr->active_submenu_;
        }
        
        // Check up the parent menu chain
        auto parent = parent_menu_.lock();
        while (parent) {
            if (focused == parent) {
                has_focus = true;
                break;
            }
            parent = parent->parent_menu_.lock();
        }
        
        // Also allow focus on the parent MenuBar itself
        if (!has_focus && focused) {
            auto* node = dynamic_cast<GooeyNode*>(focused.get());
            if (node && node->id == "propertiesGrid") { // skip properties grid interactions if needed, but standard menu bar id check:
                // ...
            }
        }

        if (!has_focus && controller->get_focused_element() != nullptr) {
            // Dismiss menu if focus shifted completely outside the menu hierarchy
            auto self = const_cast<Menu*>(this)->shared_from_this();
            gooey::Application::get_instance()->dispatch([self]() {
                self->close();
            });
            return;
        }
    }

    // 1. Draw Menu Panel Background and Border
    ooey::RectPrimitive panel_bg(bounds_, bg_color_, border_color_, 1.5f);
    panel_bg.draw(target);

    // 2. Draw Menu Items
    int curr_y = bounds_.y;
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        int item_h = item.is_separator ? 10 : row_height_;

        Rect item_rect{bounds_.x + 2, curr_y, bounds_.width - 4, item_h};

        if (item.is_separator) {
            ooey::LinePrimitive line(
                Point{bounds_.x + 8, curr_y + 5},
                Point{bounds_.x + bounds_.width - 8, curr_y + 5},
                border_color_,
                1.0f
            );
            line.draw(target);
        } else {
            // Hover background
            if (static_cast<int>(i) == hovered_idx_) {
                ooey::RectPrimitive hover_bg(item_rect, hover_bg_color_);
                hover_bg.draw(target);
            }

            Color text_col = (static_cast<int>(i) == hovered_idx_) ? hover_text_color_ : text_color_;

            // Checkbox indicator
            if (item.is_checkbox) {
                ooey::Rect check_box_rect{bounds_.x + 8, curr_y + (row_height_ - 14) / 2, 14, 14};
                ooey::RectPrimitive box(check_box_rect, bg_color_, text_col, 1.0f);
                box.draw(target);

                if (item.checked) {
                    // Draw a simple checkmark indicator
                    ooey::Rect check_rect{bounds_.x + 11, curr_y + (row_height_ - 8) / 2, 8, 8};
                    ooey::RectPrimitive check(check_rect, text_col);
                    check.draw(target);
                }
            }

            // Label text
            Point text_pos{bounds_.x + (item.is_checkbox ? 28 : 12), curr_y + (row_height_ - 14) / 2};
            ooey::TextPrimitive label(item.label, Font{"sans-serif", 13}, text_pos, text_col);
            label.draw(target);

            // Shortcut text
            if (!item.shortcut.empty()) {
                // Approximate shortcut text width (simple width estimate for positioning)
                int short_w = static_cast<int>(item.shortcut.length() * 7);
                Point short_pos{bounds_.x + bounds_.width - short_w - 12, curr_y + (row_height_ - 14) / 2};
                ooey::TextPrimitive shortcut(item.shortcut, Font{"sans-serif", 11}, short_pos, border_color_);
                shortcut.draw(target);
            }

            // Submenu indicator (right-pointing triangle arrow)
            if (!item.subitems.empty()) {
                int arrow_x = bounds_.x + bounds_.width - 15;
                int arrow_y = curr_y + (row_height_ - 10) / 2;
                // Simple representation of an arrow >
                ooey::TextPrimitive arrow(">", Font{"sans-serif", 12}, Point{arrow_x, arrow_y}, text_col);
                arrow.draw(target);
            }
        }

        curr_y += item_h;
    }

    // 3. Draw Active Submenu
    if (active_submenu_) {
        active_submenu_->draw(target);
    }
}

bool Menu::on_pointer_event(const Pointer& e) {
    if (!is_open_) return false;

    // Check if pointer falls inside our submenu first
    if (active_submenu_ && active_submenu_->on_pointer_event(e)) {
        return true;
    }

    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);

    if (e.state == PointerState::Moved) {
        if (hit) {
            // Find which item is hovered
            int curr_y = bounds_.y;
            int new_hover = -1;
            for (size_t i = 0; i < items_.size(); ++i) {
                int item_h = items_[i].is_separator ? 10 : row_height_;
                if (e.y >= curr_y && e.y < curr_y + item_h) {
                    if (!items_[i].is_separator) {
                        new_hover = static_cast<int>(i);
                    }
                    break;
                }
                curr_y += item_h;
            }

            if (new_hover != hovered_idx_) {
                hovered_idx_ = new_hover;
                
                // Focus ourselves when hovered so keys/events are routed to us
                auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(
                    gooey::Application::get_instance()->get_controller());
                if (controller) {
                    controller->set_focused_element(shared_from_this());
                }

                // If hovered item has a submenu, open it
                if (hovered_idx_ >= 0 && !items_[hovered_idx_].subitems.empty()) {
                    close_submenus();
                    
                    int sub_y = bounds_.y;
                    for (int k = 0; k < hovered_idx_; ++k) {
                        sub_y += items_[k].is_separator ? 10 : row_height_;
                    }

                    auto submenu = std::make_shared<Menu>(items_[hovered_idx_].subitems);
                    submenu->parent_menu_ = shared_from_this();
                    
                    // Position to the right of this item
                    Rect sub_rect{bounds_.x + bounds_.width - 2, sub_y, 180, 0}; // height resolved by measure
                    Size sub_size = submenu->measure(Size{180, 400});
                    sub_rect.height = sub_size.height;
                    submenu->layout(sub_rect);

                    active_submenu_ = submenu;
                } else {
                    close_submenus();
                }
                
                invalidate_layout();
            }
            return true;
        } else {
            if (hovered_idx_ != -1) {
                hovered_idx_ = -1;
                invalidate_layout();
            }
        }
    }

    if (hit && e.state == PointerState::Pressed) {
        auto self = shared_from_this();
        if (hovered_idx_ >= 0 && hovered_idx_ < static_cast<int>(items_.size())) {
            auto action = items_[hovered_idx_].action;
            bool is_checkbox = items_[hovered_idx_].is_checkbox;
            if (items_[hovered_idx_].subitems.empty()) {
                if (is_checkbox) {
                    items_[hovered_idx_].checked = !items_[hovered_idx_].checked;
                }
                close();
                if (action) {
                    action();
                }
            }
        }
        return true;
    }

    return hit;
}

bool Menu::on_key_event(const KeyEvent& e) {
    if (!is_open_) return false;

    // Route keyboard events to active submenu first
    if (active_submenu_ && active_submenu_->on_key_event(e)) {
        return true;
    }

    if (e.state == KeyState::Pressed) {
        if (e.key_code == 0xFF52 /* Up */) {
            // Find previous selectable item
            int idx = hovered_idx_ - 1;
            while (idx >= 0 && items_[idx].is_separator) {
                idx--;
            }
            if (idx >= 0) {
                hovered_idx_ = idx;
                invalidate_layout();
            }
            return true;
        } else if (e.key_code == 0xFF54 /* Down */) {
            // Find next selectable item
            int idx = hovered_idx_ + 1;
            while (idx < static_cast<int>(items_.size()) && items_[idx].is_separator) {
                idx++;
            }
            if (idx < static_cast<int>(items_.size())) {
                hovered_idx_ = idx;
                invalidate_layout();
            }
            return true;
        } else if (e.key_code == 27 /* Escape */) {
            close();
            return true;
        } else if (e.key_code == 0xFF0D || e.key_code == 13 /* Enter */) {
            auto self = shared_from_this();
            if (hovered_idx_ >= 0 && hovered_idx_ < static_cast<int>(items_.size())) {
                auto action = items_[hovered_idx_].action;
                bool is_checkbox = items_[hovered_idx_].is_checkbox;
                if (items_[hovered_idx_].subitems.empty()) {
                    if (is_checkbox) {
                        items_[hovered_idx_].checked = !items_[hovered_idx_].checked;
                    }
                    close();
                    if (action) {
                        action();
                    }
                }
            }
            return true;
        } else if (e.key_code == 0xFF51 /* Left */) {
            if (auto parent = parent_menu_.lock()) {
                parent->close_submenus();
                auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(
                    gooey::Application::get_instance()->get_controller());
                if (controller) {
                    controller->set_focused_element(parent);
                }
                return true;
            } else if (on_navigate_sibling) {
                on_navigate_sibling(-1);
                return true;
            }
        } else if (e.key_code == 0xFF53 /* Right */) {
            if (hovered_idx_ >= 0 && hovered_idx_ < static_cast<int>(items_.size()) && !items_[hovered_idx_].subitems.empty()) {
                close_submenus();
                int sub_y = bounds_.y;
                for (int k = 0; k < hovered_idx_; ++k) {
                    sub_y += items_[k].is_separator ? 10 : row_height_;
                }
                auto submenu = std::make_shared<Menu>(items_[hovered_idx_].subitems);
                submenu->parent_menu_ = shared_from_this();

                Rect sub_rect{bounds_.x + bounds_.width - 2, sub_y, 180, 0};
                Size sub_size = submenu->measure(Size{180, 400});
                sub_rect.height = sub_size.height;
                submenu->layout(sub_rect);

                active_submenu_ = submenu;

                auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(
                    gooey::Application::get_instance()->get_controller());
                if (controller) {
                    controller->set_focused_element(active_submenu_);
                }
                invalidate_layout();
                return true;
            } else if (on_navigate_sibling) {
                on_navigate_sibling(1);
                return true;
            }
        }
    }
    return false;
}

void Menu::close() {
    if (!is_open_) return;
    auto self = shared_from_this();
    is_open_ = false;
    
    close_submenus();

    // Remove ourselves from parent nodes
    auto* parent_el = get_parent();
    if (parent_el) {
        auto* parent_node = dynamic_cast<GooeyNode*>(parent_el);
        if (parent_node) {
            parent_node->remove_child(shared_from_this());
        }
    }

    if (on_close) {
        on_close();
    }

    // Clean up focus in controller
    auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(
        gooey::Application::get_instance()->get_controller());
    if (controller && controller->get_focused_element() == shared_from_this()) {
        controller->set_focused_element(nullptr);
    }
}

void Menu::close_submenus() {
    if (active_submenu_) {
        active_submenu_->close();
        active_submenu_ = nullptr;
    }
}

} // namespace gooey::controls
