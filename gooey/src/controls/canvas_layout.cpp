#include "gooey/controls/canvas_layout.hpp"

namespace gooey::controls {
    using namespace ooey;

CanvasLayout::CanvasLayout() {
    is_absolute = false;
}

Rect CanvasLayout::bounds() const {
    return layout_bounds;
}

Size CanvasLayout::do_measure(Size constraints) {
    return GooeyNode::do_measure(constraints);
}

void CanvasLayout::do_layout(Rect bounds) {
    layout_bounds = bounds;
    GooeyNode::do_layout(bounds);
}

bool CanvasLayout::on_pointer_event(const Pointer& e) {
    if (e.x >= layout_bounds.x && e.x <= layout_bounds.x + layout_bounds.width &&
        e.y >= layout_bounds.y && e.y <= layout_bounds.y + layout_bounds.height) {
        
        if (on_canvas_pointer) {
            on_canvas_pointer(e);
        }
        return true;
    }
    return false;
}

bool CanvasLayout::on_key_event(const KeyEvent& /*e*/) {
    return false;
}

} // namespace gooey::controls
