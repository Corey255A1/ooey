#pragma once

namespace ooey {}


#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/text_primitive.hpp"
#include <functional>
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class TextBox : public View, public IInteractive {
public:
    TextBox(Rect bounds, Font font, Color text_color, Color bg_color);

    Rect bounds() const override;

    void set_text(const std::string& text);
    const std::string& get_text() const;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;
    bool on_text_event(const TextEvent& e) override;

    std::function<void(const std::string&)> on_text_changed;

    // Layout support
    Size measure(Size constraints) override;
    void layout(Rect bounds) override;

private:
    Rect bounds_;
    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<TextPrimitive> text_primitive_;
    std::string text_;
    bool is_focused_{false};
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::TextBox;
}
