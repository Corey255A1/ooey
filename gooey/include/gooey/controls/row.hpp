#pragma once

#include "gooey/mvvmc/view.hpp"

namespace gooey::controls {

class Row : public View {
public:
    Row() = default;

    Size measure(Size constraints) override;
    void layout(Rect bounds) override;
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::Row;
}
