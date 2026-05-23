#pragma once

#include "ooey/view.hpp"
#include "ooey/primitives/text_primitive.hpp"
#include <memory>
#include <string>

namespace ooey {

class Label : public View {
public:
    Label(std::string text, Font font, Point position, Color color);

    void set_text(const std::string& text);
    const std::string& get_text() const;

    void set_font(const Font& font);
    void set_color(Color color);
    void set_position(Point position);

private:
    std::shared_ptr<TextPrimitive> text_primitive_;
};

} // namespace ooey