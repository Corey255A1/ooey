#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include "gooey/controls/scrollbar.hpp"
#include <memory>

namespace gooey::controls {

class ScrollContainer : public GooeyNode, public IInteractive {
public:
    ScrollContainer();

    Rect bounds() const override { return layout_bounds; }

    void set_child(std::shared_ptr<GooeyNode> child);
    std::shared_ptr<GooeyNode> get_child() const { return child_; }
    void add_child(std::shared_ptr<IDrawable>&& child) override;

    int get_scroll_offset_x() const { return scroll_offset_x_; }
    void set_scroll_offset_x(int offset);

    int get_scroll_offset_y() const { return scroll_offset_y_; }
    void set_scroll_offset_y(int offset);

    bool needs_scroll() const { return needs_scroll_y_; }
    bool needs_scroll_x() const { return needs_scroll_x_; }
    bool needs_scroll_y() const { return needs_scroll_y_; }

    void scroll_to_visible(Rect rect);

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override { return false; }

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    std::shared_ptr<GooeyNode> child_;
    std::shared_ptr<ScrollBar> v_scroll_;
    std::shared_ptr<ScrollBar> h_scroll_;
    
    int scroll_offset_x_{0};
    int scroll_offset_y_{0};
    int max_scroll_x_{0};
    int max_scroll_y_{0};
    
    Size child_measured_size_{0, 0};
    bool needs_scroll_x_{false};
    bool needs_scroll_y_{false};

    // Drag-scroll tracking
    bool dragging_content_{false};
    int drag_start_x_{0};
    int drag_start_y_{0};
    int drag_start_offset_x_{0};
    int drag_start_offset_y_{0};
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::ScrollContainer;
}
