#pragma once

namespace ooey {}


#include "gooey/mvvmc/view.hpp"
#include "gooey/renderer/primitives/text_primitive.hpp"
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
    void set_color(Color color);
    void set_position(Point position);

private:
    std::shared_ptr<TextPrimitive> text_primitive_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::Label;
}
