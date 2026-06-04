#pragma once

#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "gooey/controls/label.hpp"
#include <functional>
#include <memory>
#include <string>

namespace gooey::controls {
    using namespace ooey;

class CheckBox : public mvvmc::GooeyElement, public IInteractive {
public:
    CheckBox();
    CheckBox(Rect bounds, std::string text, bool initial_checked = false);

    void draw(ooey::IRenderTarget& target) const override;
    Rect bounds() const override;

    // Value bindings compatibility
    void set_value(bool checked);
    bool get_value() const;

    void set_checked(bool checked);
    bool is_checked() const;

    void set_text(const std::string& text);
    void set_label_text(const std::string& text);
    const std::string& get_text() const;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    std::function<void(bool)> on_checked_changed;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    Rect bounds_;
    std::string text_;
    bool checked_{false};
    std::shared_ptr<RoundedRectPrimitive> box_bg_;
    std::shared_ptr<RectPrimitive> checked_indicator_;
    std::shared_ptr<Label> label_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::CheckBox;
}
