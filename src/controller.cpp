#include "ooey/controller.hpp"
#include "ooey/i_drawable.hpp"

namespace ooey {

Controller::Controller(InputManager& input_manager, std::shared_ptr<View> root_view)
    : input_manager_(input_manager), root_view_(std::move(root_view)) {}

void Controller::process_events() {
    for (const auto& pointer_event : input_manager_.get_pointer_events()) {
        route_pointer_event(pointer_event, root_view_);
    }

    for (const auto& key_event : input_manager_.get_key_events()) {
        if (focused_element_) {
            focused_element_->on_key_event(key_event);
        }
    }
}

void Controller::set_focused_element(IInteractive* element) {
    focused_element_ = element;
}

bool Controller::route_pointer_event(const Pointer& pointer, const std::shared_ptr<IDrawable>& node) {
    if (!node) return false;

    // Traverse children first (top-most elements usually drawn last, so reverse order)
    auto* view = dynamic_cast<View*>(node.get());
    if (view) {
        const auto& children = view->get_children();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (route_pointer_event(pointer, *it)) {
                return true;
            }
        }
    }

    auto* interactive = dynamic_cast<IInteractive*>(node.get());
    if (interactive) {
        Rect b = interactive->bounds();
        if (pointer.x >= b.x && pointer.x <= b.x + b.width &&
            pointer.y >= b.y && pointer.y <= b.y + b.height) {
            return interactive->on_pointer_event(pointer);
        }
    }

    return false;
}

} // namespace ooey