#pragma once

#include "gooey/mvvmc/gooey_element.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include <functional>
#include <memory>

namespace gooey::controls {
    using namespace ooey;

enum class ScrollBarOrientation {
    Vertical,
    Horizontal
};

class ScrollBar : public mvvmc::GooeyElement, public IInteractive {
public:
    ScrollBar(Rect bounds, ScrollBarOrientation orientation);

    Rect bounds() const override;

    void set_range(int min_val, int max_val, int page_size);
    void set_value(int value);
    int get_value() const { return value_; }

    ScrollBarOrientation get_orientation() const { return orientation_; }

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    void draw(ooey::IRenderTarget& target) const override;

    std::function<void(int)> on_value_changed;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
    void apply_style(const mvvmc::Style& style) override;

private:
    void update_thumb_bounds();

    Rect bounds_;
    ScrollBarOrientation orientation_;
    int min_val_{0};
    int max_val_{100};
    int page_size_{10};
    int value_{0};

    Rect thumb_bounds_{0, 0, 0, 0};
    bool dragging_thumb_{false};
    int drag_start_offset_{0};

    Color track_color_{40, 40, 45};
    Color thumb_color_{100, 100, 110};

    std::shared_ptr<RectPrimitive> track_prim_;
    std::shared_ptr<RoundedRectPrimitive> thumb_prim_;
};

} // namespace gooey::controls
namespace gooey {
    using namespace ooey;
using gooey::controls::ScrollBar;
using gooey::controls::ScrollBarOrientation;
}
