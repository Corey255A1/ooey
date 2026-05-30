#pragma once

#include "gooey/mvvmc/view.hpp"

namespace gooey::controls {

class FlowLayout : public View {
public:
    FlowLayout() = default;

    Size measure(Size constraints) override;
    void layout(Rect bounds) override;
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::FlowLayout;
}
