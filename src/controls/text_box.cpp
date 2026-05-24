#include "ooey/controls/text_box.hpp"
#include <iostream>

namespace ooey {

TextBox::TextBox(Rect bounds, Font font, Color text_color, Color bg_color)
    : bounds_(bounds) {
    
    bg_ = std::make_shared<RectPrimitive>(bounds_, bg_color);
    text_primitive_ = std::make_shared<TextPrimitive>("", font, Point{bounds_.x + 5, bounds_.y + 5}, text_color);
    
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
    // Basic hit test
    bool hit = (e.x >= bounds_.x && e.x <= bounds_.x + bounds_.width &&
                e.y >= bounds_.y && e.y <= bounds_.y + bounds_.height);
    
    if (hit && e.state == PointerState::Pressed) {
        is_focused_ = true;
        bg_->set_color(Color{200, 200, 200}); // Highlight border/bg to show focus
        return true;
    } else if (!hit && e.state == PointerState::Pressed) {
        is_focused_ = false;
        bg_->set_color(Color{255, 255, 255}); // Unfocus
    }
    
    return false; // don't swallow if we didn't hit or just releasing outside
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

} // namespace ooey