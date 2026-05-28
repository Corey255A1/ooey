#include "ooey/controls/label.hpp"

namespace ooey::controls {

Label::Label(std::string text, Font font, Point position, Color color) {
    text_primitive_ = std::make_shared<TextPrimitive>(std::move(text), font, position, color);
    add_child(text_primitive_);
}

void Label::set_text(const std::string& text) {
    text_primitive_->set_text(text);
}

const std::string& Label::get_text() const {
    return text_primitive_->get_text();
}

void Label::set_font(const Font& font) {
    text_primitive_->set_font(font);
}

void Label::set_color(Color color) {
    text_primitive_->set_color(color);
}

void Label::set_position(Point position) {
    text_primitive_->set_position(position);
}

} // namespace ooey::controls