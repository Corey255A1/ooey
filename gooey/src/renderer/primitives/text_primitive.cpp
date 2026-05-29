namespace ooey {}

#include "gooey/renderer/primitives/text_primitive.hpp"
#include "ooey/renderer/i_render_target.hpp"

namespace gooey::renderer {
    using namespace ooey;

TextPrimitive::TextPrimitive(std::string text, Font font, Point position, Color color)
    : text_(std::move(text)), font_(font), position_(position), color_(color) {}

void TextPrimitive::draw(IRenderTarget& target) const {
    target.draw_text(text_, font_, position_, color_);
}

void TextPrimitive::set_text(const std::string& text) {
    text_ = text;
}

const std::string& TextPrimitive::get_text() const {
    return text_;
}

void TextPrimitive::set_font(const Font& font) {
    font_ = font;
}

const Font& TextPrimitive::get_font() const {
    return font_;
}

void TextPrimitive::set_position(Point position) {
    position_ = position;
}

Point TextPrimitive::get_position() const {
    return position_;
}

void TextPrimitive::set_color(Color color) {
    color_ = color;
}

Color TextPrimitive::get_color() const {
    return color_;
}

} // namespace gooey::renderer