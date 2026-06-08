#include "gooey/controls/menubar.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/application.hpp"
#include "ooey/renderer/font_engine.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/line_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <algorithm>

namespace gooey::controls {
    using namespace ooey;

namespace {
    GooeyNode* find_root_node(GooeyNode* node) {
        GooeyNode* curr = node;
        while (curr->get_parent()) {
            auto* next = dynamic_cast<GooeyNode*>(curr->get_parent());
            if (!next) break;
            curr = next;
        }
        return curr;
    }
}

MenuBar::MenuBar() : MenuBar(std::vector<MenuCategory>{}) {}

MenuBar::MenuBar(const std::vector<MenuCategory>& categories)
    : categories_(categories) {
    width = {SizePolicy::MatchParent};
    height = {SizePolicy::WrapContent};
}

void MenuBar::set_categories(const std::vector<MenuCategory>& categories) {
    categories_ = categories;
    close_menu();
    invalidate_layout();
}

Size MenuBar::do_measure(Size constraints) {
    int w = resolve_width(constraints.width, 100);
    int h = 40;
    if (w <= breakpoint_) {
        if (hamburger_open_) {
            h = 40 + static_cast<int>(categories_.size()) * 40;
        }
    }
    h = resolve_height(constraints.height, h);
    return Size{w, h};
}

void MenuBar::do_layout(Rect bounds) {
    bounds_ = bounds;
    GooeyElement::do_layout(bounds);

    category_rects_.clear();
    collapsed_category_rects_.clear();
    hamburger_rect_ = Rect{0, 0, 0, 0};

    if (bounds_.width > breakpoint_) {
        hamburger_open_ = false;
        int curr_x = bounds_.x + 10;
        Font font{"sans-serif", 14};
        for (const auto& cat : categories_) {
            Size text_size = FontEngine::measure_text(cat.name, font);
            int cat_w = text_size.width + 30;
            category_rects_.push_back(Rect{curr_x, bounds_.y, cat_w, 40});
            curr_x += cat_w;
        }
    } else {
        hamburger_rect_ = Rect{bounds_.x + bounds_.width - 40, bounds_.y, 40, 40};
        if (hamburger_open_) {
            for (size_t i = 0; i < categories_.size(); ++i) {
                collapsed_category_rects_.push_back(Rect{
                    bounds_.x,
                    bounds_.y + 40 + static_cast<int>(i) * 40,
                    bounds_.width,
                    40
                });
            }
        }
    }

    if (active_menu_ && active_category_idx_ >= 0 && active_category_idx_ < static_cast<int>(categories_.size())) {
        Point menu_pos;
        if (bounds_.width > breakpoint_) {
            Rect cat_rect = category_rects_[active_category_idx_];
            menu_pos = Point{cat_rect.x, cat_rect.y + cat_rect.height};
        } else {
            Rect cat_rect = collapsed_category_rects_[active_category_idx_];
            menu_pos = Point{bounds_.x + 40, cat_rect.y + cat_rect.height};
        }

        Size menu_size = active_menu_->measure(Size{180, 400});
        auto* root_node = find_root_node(this);
        int rel_x = menu_pos.x - root_node->layout_bounds.x - root_node->padding_left;
        int rel_y = menu_pos.y - root_node->layout_bounds.y - root_node->padding_top;
        active_menu_->set_absolute_bounds(Rect{rel_x, rel_y, menu_size.width, menu_size.height});
        active_menu_->layout(Rect{menu_pos.x, menu_pos.y, menu_size.width, menu_size.height});
    }
}

void MenuBar::draw(ooey::IRenderTarget& target) const {
    ooey::RectPrimitive bar_bg(Rect{bounds_.x, bounds_.y, bounds_.width, 40}, bar_bg_color_);
    bar_bg.draw(target);

    ooey::LinePrimitive bottom_border(
        Point{bounds_.x, bounds_.y + 39},
        Point{bounds_.x + bounds_.width, bounds_.y + 39},
        border_color_,
        1.0f
    );
    bottom_border.draw(target);

    if (bounds_.width <= breakpoint_ && hamburger_open_) {
        int expanded_h = static_cast<int>(categories_.size()) * 40;
        ooey::RectPrimitive expanded_bg(
            Rect{bounds_.x, bounds_.y + 40, bounds_.width, expanded_h},
            bar_bg_color_,
            border_color_,
            1.0f
        );
        expanded_bg.draw(target);
    }

    Font font{"sans-serif", 14};

    if (bounds_.width > breakpoint_) {
        for (size_t i = 0; i < categories_.size(); ++i) {
            Rect cat_rect = category_rects_[i];
            bool is_active = (static_cast<int>(i) == active_category_idx_);
            bool is_hovered = (static_cast<int>(i) == hovered_category_idx_);

            if (is_active) {
                ooey::RectPrimitive active_bg(cat_rect, active_bg_color_);
                active_bg.draw(target);
            } else if (is_hovered) {
                ooey::RectPrimitive hover_bg(cat_rect, hover_bg_color_);
                hover_bg.draw(target);
            }

            Color text_col = (is_active) ? active_text_color_ : text_color_;
            Size text_size = FontEngine::measure_text(categories_[i].name, font);
            Point text_pos{
                cat_rect.x + (cat_rect.width - text_size.width) / 2,
                cat_rect.y + (cat_rect.height - text_size.height) / 2
            };

            ooey::TextPrimitive label(categories_[i].name, font, text_pos, text_col);
            label.draw(target);
        }
    } else {
        bool draw_hover = hamburger_hovered_ || hamburger_open_;
        if (draw_hover) {
            ooey::RectPrimitive ham_bg(hamburger_rect_, hover_bg_color_);
            ham_bg.draw(target);
        }

        int cx = hamburger_rect_.x + 20;
        int cy = hamburger_rect_.y + 20;
        for (int offset : {-6, 0, 6}) {
            ooey::LinePrimitive line(
                Point{cx - 9, cy + offset},
                Point{cx + 9, cy + offset},
                text_color_,
                2.0f
            );
            line.draw(target);
        }

        if (hamburger_open_) {
            for (size_t i = 0; i < categories_.size(); ++i) {
                Rect cat_rect = collapsed_category_rects_[i];
                bool is_active = (static_cast<int>(i) == active_category_idx_);
                bool is_hovered = (static_cast<int>(i) == hovered_category_idx_);

                if (is_active) {
                    ooey::RectPrimitive active_bg(cat_rect, active_bg_color_);
                    active_bg.draw(target);
                } else if (is_hovered) {
                    ooey::RectPrimitive hover_bg(cat_rect, hover_bg_color_);
                    hover_bg.draw(target);
                }

                Color text_col = (is_active) ? active_text_color_ : text_color_;
                Size text_size = FontEngine::measure_text(categories_[i].name, font);
                Point text_pos{
                    cat_rect.x + 20,
                    cat_rect.y + (cat_rect.height - text_size.height) / 2
                };

                ooey::TextPrimitive label(categories_[i].name, font, text_pos, text_col);
                label.draw(target);
            }
        }
    }

    GooeyNode::draw(target);
}

bool MenuBar::on_pointer_event(const Pointer& e) {
    bool hit_overall = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                        e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);

    if (e.state == PointerState::Moved) {
        if (!hit_overall) {
            if (hovered_category_idx_ != -1 || hamburger_hovered_) {
                hovered_category_idx_ = -1;
                hamburger_hovered_ = false;
                invalidate_layout();
            }
            return false;
        }

        if (bounds_.width > breakpoint_) {
            int new_hover = -1;
            for (size_t i = 0; i < categories_.size(); ++i) {
                if (e.x >= category_rects_[i].x && e.x <= category_rects_[i].x + category_rects_[i].width &&
                    e.y >= category_rects_[i].y && e.y <= category_rects_[i].y + category_rects_[i].height) {
                    new_hover = static_cast<int>(i);
                    break;
                }
            }

            if (new_hover != hovered_category_idx_) {
                hovered_category_idx_ = new_hover;
                if (active_category_idx_ != -1 && new_hover != -1 && new_hover != active_category_idx_) {
                    open_menu(new_hover);
                }
                invalidate_layout();
            }
            hamburger_hovered_ = false;
        } else {
            bool ham_hover = (e.x >= hamburger_rect_.x && e.x <= hamburger_rect_.x + hamburger_rect_.width &&
                              e.y >= hamburger_rect_.y && e.y <= hamburger_rect_.y + hamburger_rect_.height);
            if (ham_hover != hamburger_hovered_) {
                hamburger_hovered_ = ham_hover;
                invalidate_layout();
            }

            if (hamburger_open_) {
                int new_hover = -1;
                for (size_t i = 0; i < categories_.size(); ++i) {
                    if (e.x >= collapsed_category_rects_[i].x && e.x <= collapsed_category_rects_[i].x + collapsed_category_rects_[i].width &&
                        e.y >= collapsed_category_rects_[i].y && e.y <= collapsed_category_rects_[i].y + collapsed_category_rects_[i].height) {
                        new_hover = static_cast<int>(i);
                        break;
                    }
                }
                if (new_hover != hovered_category_idx_) {
                    hovered_category_idx_ = new_hover;
                    invalidate_layout();
                }
            } else {
                hovered_category_idx_ = -1;
            }
        }
        return true;
    }

    if (e.state == PointerState::Pressed) {
        if (bounds_.width > breakpoint_) {
            for (size_t i = 0; i < categories_.size(); ++i) {
                if (e.x >= category_rects_[i].x && e.x <= category_rects_[i].x + category_rects_[i].width &&
                    e.y >= category_rects_[i].y && e.y <= category_rects_[i].y + category_rects_[i].height) {
                    if (active_category_idx_ == static_cast<int>(i)) {
                        close_menu();
                    } else {
                        open_menu(static_cast<int>(i));
                    }
                    return true;
                }
            }
            close_menu();
        } else {
            if (e.x >= hamburger_rect_.x && e.x <= hamburger_rect_.x + hamburger_rect_.width &&
                e.y >= hamburger_rect_.y && e.y <= hamburger_rect_.y + hamburger_rect_.height) {
                hamburger_open_ = !hamburger_open_;
                close_menu();
                invalidate_layout();
                return true;
            }

            if (hamburger_open_) {
                for (size_t i = 0; i < categories_.size(); ++i) {
                    if (e.x >= collapsed_category_rects_[i].x && e.x <= collapsed_category_rects_[i].x + collapsed_category_rects_[i].width &&
                        e.y >= collapsed_category_rects_[i].y && e.y <= collapsed_category_rects_[i].y + collapsed_category_rects_[i].height) {
                        if (active_category_idx_ == static_cast<int>(i)) {
                            close_menu();
                        } else {
                            open_menu(static_cast<int>(i));
                        }
                        return true;
                    }
                }
            }
        }
    }

    return hit_overall;
}

void MenuBar::open_menu(int idx) {
    close_menu();

    active_category_idx_ = idx;
    if (idx < 0 || idx >= static_cast<int>(categories_.size())) return;

    active_menu_ = std::make_shared<Menu>(categories_[idx].items);

    Point menu_pos;
    if (bounds_.width > breakpoint_) {
        Rect cat_rect = category_rects_[idx];
        menu_pos = Point{cat_rect.x, cat_rect.y + cat_rect.height};
    } else {
        Rect cat_rect = collapsed_category_rects_[idx];
        menu_pos = Point{bounds_.x + 40, cat_rect.y + cat_rect.height};
    }

    Size menu_size = active_menu_->measure(Size{180, 400});
    auto* root_node = find_root_node(this);
    int rel_x = menu_pos.x - root_node->layout_bounds.x - root_node->padding_left;
    int rel_y = menu_pos.y - root_node->layout_bounds.y - root_node->padding_top;
    
    active_menu_->set_absolute_bounds(Rect{rel_x, rel_y, menu_size.width, menu_size.height});
    active_menu_->layout(Rect{menu_pos.x, menu_pos.y, menu_size.width, menu_size.height});

    active_menu_->on_close = [this]() {
        active_category_idx_ = -1;
        active_menu_ = nullptr;
        invalidate_layout();
    };

    active_menu_->on_navigate_sibling = [this](int direction) {
        if (categories_.empty()) return;
        int next_idx = (active_category_idx_ + direction + static_cast<int>(categories_.size())) % static_cast<int>(categories_.size());
        open_menu(next_idx);
    };

    auto* controller = dynamic_cast<gooey::mvvmc::Controller*>(
        gooey::Application::get_instance()->get_controller());
    if (controller) {
        controller->set_focused_element(active_menu_);
    }

    root_node->add_child(std::shared_ptr<IDrawable>(active_menu_));
    invalidate_layout();
}

void MenuBar::close_menu() {
    if (active_menu_) {
        active_menu_->on_close = nullptr;
        active_menu_->close();
        active_menu_ = nullptr;
    }
    active_category_idx_ = -1;
    invalidate_layout();
}

bool MenuBar::on_key_event(const KeyEvent& e) {
    if (e.state == KeyState::Pressed) {
        if (e.key_code == 27 /* Escape */) {
            close_menu();
            return true;
        }
        if (e.key_code == 0xFF51 /* Left */) {
            if (active_category_idx_ != -1) {
                int next_idx = (active_category_idx_ - 1 + static_cast<int>(categories_.size())) % static_cast<int>(categories_.size());
                open_menu(next_idx);
                return true;
            }
        } else if (e.key_code == 0xFF53 /* Right */) {
            if (active_category_idx_ != -1) {
                int next_idx = (active_category_idx_ + 1) % static_cast<int>(categories_.size());
                open_menu(next_idx);
                return true;
            }
        }
    }
    return false;
}

void MenuBar::apply_style(const mvvmc::Style& style) {
    bar_bg_color_ = style.fill_color;
    border_color_ = style.stroke_color;
    text_color_ = style.text_color;

    if (bar_bg_color_.r != 0 || bar_bg_color_.g != 0 || bar_bg_color_.b != 0 || bar_bg_color_.a != 0) {
        hover_bg_color_ = Color{
            static_cast<uint8_t>(std::clamp(bar_bg_color_.r + 20, 0, 255)),
            static_cast<uint8_t>(std::clamp(bar_bg_color_.g + 20, 0, 255)),
            static_cast<uint8_t>(std::clamp(bar_bg_color_.b + 20, 0, 255)),
            bar_bg_color_.a
        };
    }
    GooeyElement::apply_style(style);
    invalidate_layout();
}

} // namespace gooey::controls
