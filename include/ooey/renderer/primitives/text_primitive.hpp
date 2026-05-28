#pragma once

#include "ooey/mvvmc/i_drawable.hpp"
#include "ooey/types.hpp"
#include <string>

namespace ooey::renderer {

class TextPrimitive : public IDrawable {
public:
    TextPrimitive(std::string text, Font font, Point position, Color color);

    void draw(IRenderTarget& target) const override;
    
    // Setters/Getters
    void set_text(const std::string& text);
    const std::string& get_text() const;

    void set_font(const Font& font);
    const Font& get_font() const;

    void set_position(Point position);
    Point get_position() const;

    void set_color(Color color);
    Color get_color() const;

private:
    std::string text_;
    Font font_;
    Point position_;
    Color color_;
};

} // namespace ooey::renderer
namespace ooey {
using renderer::TextPrimitive;
}
