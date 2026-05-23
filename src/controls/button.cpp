#include "ooey/controls/button.hpp"

namespace ooey {

Button::Button(Rect bounds, Color color) : bounds_(bounds), color_(color) {
    bg_ = std::make_shared<RectPrimitive>(bounds_, color_);
    // To construct the view hierarchy for the button
    add_child(bg_);
}

Rect Button::bounds() const {
    return bounds_;
}

void Button::set_color(Color color) {
    color_ = color;
    if (bg_) {
        bg_->set_color(color_);
    }
}

bool Button::on_pointer_event(const Pointer& e) {
    if (e.state == PointerState::Pressed) {
        if (on_click) on_click();
        return true;
    }
    return false;
}

bool Button::on_key_event(const KeyEvent& e) {
    return false;
}

} // namespace ooey