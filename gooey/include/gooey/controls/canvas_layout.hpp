#pragma once

#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include <functional>

namespace gooey::controls {
    using namespace ooey;

class CanvasLayout : public GooeyNode, public IInteractive {
public:
    CanvasLayout();
    virtual ~CanvasLayout() override = default;

    Rect bounds() const override;

    bool on_pointer_event(const Pointer& e) override;
    bool on_key_event(const KeyEvent& e) override;

    std::function<void(const Pointer&)> on_canvas_pointer;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;
};

} // namespace gooey::controls
namespace gooey {
using gooey::controls::CanvasLayout;
}
