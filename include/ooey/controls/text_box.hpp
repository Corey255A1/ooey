#pragma once

#include "ooey/view.hpp"
#include "ooey/i_interactive.hpp"
#include "ooey/primitives/rect_primitive.hpp"
#include "ooey/primitives/text_primitive.hpp"
#include <functional>
#include <memory>
#include <string>

namespace ooey {

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

private:
    Rect bounds_;
    std::shared_ptr<RectPrimitive> bg_;
    std::shared_ptr<TextPrimitive> text_primitive_;
    std::string text_;
    bool is_focused_{false};
};

} // namespace ooey