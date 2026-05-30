#pragma once

#include "gooey/mvvmc/view.hpp"

namespace gooey::controls {

class Grid : public View {
public:
    Grid(int rows, int columns);

    Size measure(Size constraints) override;
    void layout(Rect bounds) override;

private:
    int rows_{1};
    int columns_{1};
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::Grid;
}
