#pragma once

#include "gooey/mvvmc/gooey_node.hpp"

namespace gooey::controls {

class Grid : public GooeyNode {
public:
    Grid(int rows, int columns);

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    int rows_{1};
    int columns_{1};
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::Grid;
}
