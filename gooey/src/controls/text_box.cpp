namespace ooey {}

#include "gooey/controls/text_box.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/renderer/font_engine.hpp"
#include <iostream>

namespace gooey::controls {
    using namespace ooey;

TextBox::TextBox(Rect bounds, Font font, Color text_color, Color bg_color)
    : bounds_(bounds) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;

    current_style_.fill_color = bg_color;
    current_style_.stroke_color = Color{200, 200, 200};
    current_style_.stroke_thickness = 1.5f;
    current_style_.corner_radius = 6;
    current_style_.text_color = text_color;

    // Modern inputs have rounded corners (6px), subtle gray border (1.5px)
    bg_ = std::make_shared<RoundedRectPrimitive>(bounds_, current_style_.corner_radius, current_style_.fill_color, current_style_.stroke_color, current_style_.stroke_thickness);
    text_primitive_ = std::make_shared<TextPrimitive>("", font, Point{bounds_.x + 5, bounds_.y + 5}, current_style_.text_color);
    
    add_child(bg_);
    add_child(text_primitive_);
}

Rect TextBox::bounds() const {
    return bounds_;
}

void TextBox::set_text(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        text_primitive_->set_text(text_);
    }
}

const std::string& TextBox::get_text() const {
    return text_;
}

bool TextBox::on_pointer_event(const Pointer& e) {
    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
    
    if (hit && e.state == PointerState::Pressed) {
        is_focused_ = true;
        // Accent focus outline (blue)
        bg_->set_stroke_color(Color{0, 120, 215});
        bg_->set_stroke_thickness(2.0f);
        return true;
    } else if (!hit && e.state == PointerState::Pressed) {
        is_focused_ = false;
        // Default subtle outline
        bg_->set_stroke_color(current_style_.stroke_color);
        bg_->set_stroke_thickness(current_style_.stroke_thickness);
    }
    
    return false;
}

bool TextBox::on_key_event(const KeyEvent& e) {
    if (!is_focused_) return false;

    // Platform independent key codes usually define backspace
    // For now, let's assume standard ASCII backspace (8) or Delete (127) mapped to key_code
    if (e.state == KeyState::Pressed) {
        if (e.key_code == 8 /* Backspace */ || e.key_code == 127 /* Delete */ || e.key_code == 0xFF08 /* XK_BackSpace */) {
            if (!text_.empty()) {
                text_.pop_back();
                text_primitive_->set_text(text_);
                if (on_text_changed) {
                    on_text_changed(text_);
                }
            }
            return true;
        }
    }
    return false; // let text events handle characters
}

bool TextBox::on_text_event(const TextEvent& e) {
    if (!is_focused_) return false;

    // Only process printable ASCII for now, or full UTF-8 encoding
    // A simplistic utf-8 encoder for char32_t:
    if (e.codepoint == 8 || e.codepoint == 127) {
        return true;
    }

    if (e.codepoint < 0x80) {
        if (e.codepoint >= 32 || e.codepoint == '\n' || e.codepoint == '\t') {
            text_ += static_cast<char>(e.codepoint);
        }
    } else if (e.codepoint < 0x800) {
        text_ += static_cast<char>(0xC0 | (e.codepoint >> 6));
        text_ += static_cast<char>(0x80 | (e.codepoint & 0x3F));
    } else if (e.codepoint < 0x10000) {
        text_ += static_cast<char>(0xE0 | (e.codepoint >> 12));
        text_ += static_cast<char>(0x80 | ((e.codepoint >> 6) & 0x3F));
        text_ += static_cast<char>(0x80 | (e.codepoint & 0x3F));
    } else {
        text_ += static_cast<char>(0xF0 | (e.codepoint >> 18));
        text_ += static_cast<char>(0x80 | ((e.codepoint >> 12) & 0x3F));
        text_ += static_cast<char>(0x80 | ((e.codepoint >> 6) & 0x3F));
        text_ += static_cast<char>(0x80 | (e.codepoint & 0x3F));
    }

    text_primitive_->set_text(text_);
    if (on_text_changed) {
        on_text_changed(text_);
    }
    return true;
}

Size TextBox::measure(Size constraints) {
    int w = resolve_width(constraints.width, absolute_bounds.width);
    int h = resolve_height(constraints.height, absolute_bounds.height);
    return Size{w, h};
}

void TextBox::layout(Rect bounds) {
    bounds_ = bounds;
    View::layout(bounds);

    if (bg_) {
        bg_->set_rect(bounds_);
    }

    if (text_primitive_) {
        Size font_size = FontEngine::measure_text("A", text_primitive_->get_font());
        int tx = bounds_.x + padding_left + 10;
        int ty = bounds_.y + padding_top + (bounds_.height - font_size.height) / 2;
        text_primitive_->set_position(Point{tx, ty});
    }
}

void TextBox::apply_style(const mvvmc::Style& style) {
    current_style_ = style;
    if (bg_) {
        bg_->set_fill_color(style.fill_color);
        if (!is_focused_) {
            bg_->set_stroke_color(style.stroke_color);
            bg_->set_stroke_thickness(style.stroke_thickness);
        }
        bg_->set_corner_radius(style.corner_radius);
    }
    if (text_primitive_) {
        text_primitive_->set_color(style.text_color);
    }
    View::apply_style(style);
}

void TextBox::set_font(const Font& font) {
    if (text_primitive_) {
        text_primitive_->set_font(font);
        layout(bounds_);
    }
}

const Font& TextBox::get_font() const {
    return text_primitive_->get_font();
}

} // namespace gooey::controls