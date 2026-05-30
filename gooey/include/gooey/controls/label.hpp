#pragma once

namespace ooey {}


#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class Label : public View {
public:
    Label(std::string text, Font font, Point position, Color color);

    void set_text(const std::string& text);
    const std::string& get_text() const;

    void set_font(const Font& font);
    const Font& get_font() const;

    void set_color(Color color);
    Color get_color() const;

    void set_position(Point position);
    Point get_position() const;

    // Layout support
    Size measure(Size constraints) override;
    void layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    std::shared_ptr<TextPrimitive> text_primitive_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::Label;
}
