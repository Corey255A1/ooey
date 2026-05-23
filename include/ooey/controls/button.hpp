#pragma once

#include "ooey/view.hpp"
#include "ooey/i_interactive.hpp"
#include "ooey/primitives/rect_primitive.hpp"
#include <functional>
#include <memory>

namespace ooey {

class Button : public View, public IInteractive {
public:
    Button(Rect bounds, Color color);

    Rect bounds() const override;

    void set_color(Color color);

    bool on_pointer_event(const Pointer& e) override;

    bool on_key_event(const KeyEvent& e) override;

    std::function<void()> on_click;

private:
    Rect bounds_;
    Color color_;
    std::shared_ptr<RectPrimitive> bg_;
};

} // namespace ooey