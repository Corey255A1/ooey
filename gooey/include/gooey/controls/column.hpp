#pragma once

#include "gooey/mvvmc/view.hpp"

namespace gooey::controls {

class Column : public View {
public:
    Column() = default;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::Column;
}
