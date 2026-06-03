#pragma once

#include "gooey/mvvmc/gooey_node.hpp"

namespace gooey::controls {

class FlowLayout : public GooeyNode {
public:
    FlowLayout() = default;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::FlowLayout;
}
