#include "gooey/controls/label.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/renderer/font_engine.hpp"

namespace gooey::controls {
    using namespace ooey;


Label::Label(std::string text, Font font, Point position, Color color) {
    text_primitive_ = std::make_shared<TextPrimitive>(std::move(text), font, position, color);
    add_child(text_primitive_);

    // Default to absolute positioning using constructor coordinates and text size
    is_absolute = true;
    Size text_size = FontEngine::measure_text(text_primitive_->get_text(), font);
    absolute_bounds = Rect{position.x, position.y, text_size.width, text_size.height};
    width = {SizePolicy::WrapContent};
    height = {SizePolicy::WrapContent};
}

void Label::set_text(const std::string& text) {
    text_primitive_->set_text(text);
    if (is_absolute) {
        Size text_size = FontEngine::measure_text(text, text_primitive_->get_font());
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
        Size text_size = FontEngine::measure_text(text_primitive_->get_text(), font);
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
        // Resolve fixed/match parent constraints if set, but default to text size
        Size text_size = FontEngine::measure_text(text_primitive_->get_text(), text_primitive_->get_font());
        int w = resolve_width(constraints.width, text_size.width + padding_left + padding_right);
        int h = resolve_height(constraints.height, text_size.height + padding_top + padding_bottom);
        return Size{w, h};
    }
    return Size{0, 0};
}

void Label::layout(Rect bounds) {
    View::layout(bounds);
    if (text_primitive_) {
        text_primitive_->set_position(Point{bounds.x + padding_left, bounds.y + padding_top});
    }
}


void Label::apply_style(const mvvmc::Style& style) {
    set_color(style.text_color);
    View::apply_style(style);
}

} // namespace gooey::controls