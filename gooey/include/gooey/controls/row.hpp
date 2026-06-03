#pragma once

#include "gooey/mvvmc/gooey_node.hpp"

namespace gooey::controls {

class Row : public GooeyNode {
public:
    Row() = default;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::Row;
}
