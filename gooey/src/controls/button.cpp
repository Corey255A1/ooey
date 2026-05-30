namespace ooey {}

#include "gooey/controls/button.hpp"
#include "ooey/renderer/bitmap_font.hpp"

namespace gooey::controls {
    using namespace ooey;

Button::Button(Rect bounds, Color color) : bounds_(bounds), color_(color) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;

    // Default modern button has 8px corner radius
    bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, 8, color_);
    add_child(bg_);
}

Button::Button(Rect bounds, Color fill_color, Color stroke_color, float stroke_thickness, int corner_radius, const std::string& label_text, Color label_color)
    : bounds_(bounds), color_(fill_color) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;

    bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, corner_radius, fill_color, stroke_color, stroke_thickness);
    add_child(bg_);

    if (!label_text.empty()) {
        Font font{"sans-serif", 14};
        Size text_size = BitmapFont::measure_text(label_text, font.size);
        int lx = bounds_.x + (bounds_.width - text_size.width) / 2;
        int ly = bounds_.y + (bounds_.height - text_size.height) / 2;
        label_ = std::make_shared<Label>(label_text, font, Point{lx, ly}, label_color);
        add_child(label_);
    }
}

Rect Button::bounds() const {
    return bounds_;
}

void Button::set_color(Color color) {
    color_ = color;
    if (bg_) {
        bg_->set_fill_color(color_);
    }
}

void Button::set_fill_color(Color color) {
    color_ = color;
    if (bg_) {
        bg_->set_fill_color(color);
    }
}

void Button::set_stroke_color(Color color) {
    if (bg_) {
        bg_->set_stroke_color(color);
    }
}

void Button::set_stroke_thickness(float thickness) {
    if (bg_) {
        bg_->set_stroke_thickness(thickness);
    }
}

void Button::set_corner_radius(int radius) {
    if (bg_) {
        bg_->set_corner_radius(radius);
    }
}

void Button::set_label_text(const std::string& text) {
    if (label_) {
        label_->set_text(text);
        Size text_size = BitmapFont::measure_text(text, 14);
        int lx = bounds_.x + (bounds_.width - text_size.width) / 2;
        int ly = bounds_.y + (bounds_.height - text_size.height) / 2;
        label_->set_position(Point{lx, ly});
    } else {
        Font font{"sans-serif", 14};
        Size text_size = BitmapFont::measure_text(text, font.size);
        int lx = bounds_.x + (bounds_.width - text_size.width) / 2;
        int ly = bounds_.y + (bounds_.height - text_size.height) / 2;
        label_ = std::make_shared<Label>(text, font, Point{lx, ly}, Color{255, 255, 255});
        add_child(label_);
    }
}

bool Button::on_pointer_event(const Pointer& e) {
    // Basic hit test
    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
    if (hit && e.state == PointerState::Pressed) {
        if (on_click) {
            on_click();
        }
        return true;
    }
    return false;
}

bool Button::on_key_event(const KeyEvent& /*e*/) {
    return false;
}

Size Button::measure(Size constraints) {
    int w = 0;
    if (width.policy == SizePolicy::Fixed) {
        w = static_cast<int>(width.value);
    } else if (width.policy == SizePolicy::MatchParent) {
        w = constraints.width;
    } else {
        if (label_) {
            Size text_size = BitmapFont::measure_text(label_->get_text(), label_->get_font().size);
            w = text_size.width + padding_left + padding_right + 30;
        } else {
            w = absolute_bounds.width;
        }
    }

    int h = 0;
    if (height.policy == SizePolicy::Fixed) {
        h = static_cast<int>(height.value);
    } else if (height.policy == SizePolicy::MatchParent) {
        h = constraints.height;
    } else {
        if (label_) {
            Size text_size = BitmapFont::measure_text(label_->get_text(), label_->get_font().size);
            h = text_size.height + padding_top + padding_bottom + 20;
        } else {
            h = absolute_bounds.height;
        }
    }

    w = std::max(0, std::min(w, constraints.width));
    h = std::max(0, std::min(h, constraints.height));
    return Size{w, h};
}

void Button::layout(Rect bounds) {
    bounds_ = bounds;
    View::layout(bounds);
    
    if (bg_) {
        bg_->set_rect(bounds_);
    }
    
    if (label_) {
        Size text_size = BitmapFont::measure_text(label_->get_text(), label_->get_font().size);
        int lx = bounds_.x + (bounds_.width - text_size.width) / 2;
        int ly = bounds_.y + (bounds_.height - text_size.height) / 2;
        label_->set_position(Point{lx, ly});
        label_->layout(Rect{lx, ly, text_size.width, text_size.height});
    }
}

} // namespace gooey::controls