namespace ooey {}

#include "gooey/controls/label.hpp"
#include "ooey/renderer/bitmap_font.hpp"

namespace gooey::controls {
    using namespace ooey;

Label::Label(std::string text, Font font, Point position, Color color) {
    text_primitive_ = std::make_shared<TextPrimitive>(std::move(text), font, position, color);
    add_child(text_primitive_);

    // Default to absolute positioning using constructor coordinates and text size
    is_absolute = true;
    Size text_size = BitmapFont::measure_text(text_primitive_->get_text(), font.size);
    absolute_bounds = Rect{position.x, position.y, text_size.width, text_size.height};
    width = {SizePolicy::WrapContent};
    height = {SizePolicy::WrapContent};
}

void Label::set_text(const std::string& text) {
    text_primitive_->set_text(text);
    if (is_absolute) {
        Size text_size = BitmapFont::measure_text(text, text_primitive_->get_font().size);
        absolute_bounds.width = text_size.width;
        absolute_bounds.height = text_size.height;
    }
}

const std::string& Label::get_text() const {
    return text_primitive_->get_text();
}

void Label::set_font(const Font& font) {
    text_primitive_->set_font(font);
    if (is_absolute) {
        Size text_size = BitmapFont::measure_text(text_primitive_->get_text(), font.size);
        absolute_bounds.width = text_size.width;
        absolute_bounds.height = text_size.height;
    }
}

const Font& Label::get_font() const {
    return text_primitive_->get_font();
}

void Label::set_color(Color color) {
    text_primitive_->set_color(color);
}

Color Label::get_color() const {
    return text_primitive_->get_color();
}

void Label::set_position(Point position) {
    text_primitive_->set_position(position);
    if (is_absolute) {
        absolute_bounds.x = position.x;
        absolute_bounds.y = position.y;
    }
}

Point Label::get_position() const {
    return text_primitive_->get_position();
}

Size Label::measure(Size constraints) {
    if (text_primitive_) {
        return BitmapFont::measure_text(text_primitive_->get_text(), text_primitive_->get_font().size);
    }
    return Size{0, 0};
}

void Label::layout(Rect bounds) {
    View::layout(bounds);
    if (text_primitive_) {
        text_primitive_->set_position(Point{bounds.x + padding_left, bounds.y + padding_top});
    }
}

} // namespace gooey::controls