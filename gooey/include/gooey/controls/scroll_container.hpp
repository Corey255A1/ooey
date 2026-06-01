#pragma once

#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "gooey/controls/scrollbar.hpp"
#include <memory>

namespace gooey::controls {

class ScrollContainer : public View, public IInteractive {
public:
    ScrollContainer();

    Rect bounds() const override { return layout_bounds; }

    void set_child(std::shared_ptr<View> child);
    std::shared_ptr<View> get_child() const { return child_; }

    int get_scroll_offset_y() const { return scroll_offset_y_; }
    void set_scroll_offset_y(int offset);
    bool needs_scroll() const { return needs_scroll_; }

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override { return false; }

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    std::shared_ptr<View> child_;
    std::shared_ptr<ScrollBar> v_scroll_;
    int scroll_offset_y_{0};
    int max_scroll_y_{0};
    Size child_measured_size_{0, 0};
    bool needs_scroll_{false};

    // Drag-scroll tracking
    bool dragging_content_{false};
    int drag_start_y_{0};
    int drag_start_offset_{0};
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::ScrollContainer;
}
