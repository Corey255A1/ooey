#pragma once

#include "ooey/view.hpp"
#include "ooey/i_interactive.hpp"
#include "ooey/primitives/rounded_rect_primitive.hpp"
#include "ooey/controls/label.hpp"
#include <functional>
#include <memory>
#include <string>

namespace ooey {

class Button : public View, public IInteractive {
public:
    Button(Rect bounds, Color color);
    Button(Rect bounds, Color fill_color, Color stroke_color, float stroke_thickness, int corner_radius, const std::string& label_text = "", Color label_color = Color{255, 255, 255});

    Rect bounds() const override;

    void set_color(Color color);
    void set_fill_color(Color color);
    void set_stroke_color(Color color);
    void set_stroke_thickness(float thickness);
    void set_corner_radius(int radius);
    void set_label_text(const std::string& text);

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    std::function<void()> on_click;

private:
    Rect bounds_;
    Color color_;
    std::shared_ptr<RoundedRectPrimitive> bg_;
    std::shared_ptr<Label> label_;
};

} // namespace ooey