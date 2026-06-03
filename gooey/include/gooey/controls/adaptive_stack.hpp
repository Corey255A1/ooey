#pragma once

#include "gooey/mvvmc/gooey_node.hpp"

namespace gooey::controls {

class AdaptiveStack : public GooeyNode {
public:
    AdaptiveStack() = default;

    AdaptiveStack& set_breakpoint(int breakpoint) { breakpoint_ = breakpoint; return *this; }
    int get_breakpoint() const { return breakpoint_; }

    AdaptiveStack& set_stretch_when_vertical(bool stretch) { stretch_when_vertical_ = stretch; return *this; }
    bool get_stretch_when_vertical() const { return stretch_when_vertical_; }

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    int breakpoint_{680};
    bool stretch_when_vertical_{true};
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::AdaptiveStack;
}
