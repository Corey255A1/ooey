namespace ooey {}

#include "gooey/mvvmc/controller.hpp"
#include "gooey/mvvmc/i_drawable.hpp"
#include "gooey/controls/scroll_container.hpp"
#include "gooey/controls/scrollbar.hpp"
#include <cmath>

namespace gooey::mvvmc {
    using namespace ooey;

Controller::Controller(InputManager& input_manager, std::shared_ptr<View> root_view)
    : input_manager_(input_manager), root_view_(std::move(root_view)) {}

void Controller::process_events() {
    for (const auto& pointer_event : input_manager_.get_pointer_events()) {
        if (captured_element_) {
            // Check if we can intercept the drag for ScrollContainer
            if (pointer_event.state == PointerState::Moved && !captured_element_stolen_) {
                int dy = pointer_event.y - pointer_pressed_y_;
                if (std::abs(dy) >= 8) {
                    View* curr = dynamic_cast<View*>(captured_element_);
                    gooey::controls::ScrollContainer* scroll_container = nullptr;
                    while (curr) {
                        auto* scroll = dynamic_cast<gooey::controls::ScrollContainer*>(curr);
                        if (scroll && scroll->needs_scroll()) {
                            scroll_container = scroll;
                            break;
                        }
                        curr = curr->get_parent();
                    }

                    if (scroll_container && captured_element_ != scroll_container &&
                        !dynamic_cast<gooey::controls::ScrollBar*>(captured_element_)) {
                        // Cancel current captured element by sending a Released event outside
                        captured_element_->on_pointer_event(Pointer{pointer_event.id, -9999, -9999, PointerState::Released});
                        
                        // Switch capture to ScrollContainer
                        captured_element_ = scroll_container;
                        captured_element_stolen_ = true;
                        
                        // Initialize ScrollContainer drag
                        scroll_container->on_pointer_event(Pointer{pointer_event.id, pointer_pressed_x_, pointer_pressed_y_, PointerState::Pressed});
                    }
                }
            }

            // Route to captured (or stolen) element
            captured_element_->on_pointer_event(pointer_event);

            if (pointer_event.state == PointerState::Released) {
                captured_element_ = nullptr;
                captured_element_stolen_ = false;
            }
        } else {
            route_pointer_event(pointer_event, root_view_);
        }
    }

    for (const auto& key_event : input_manager_.get_key_events()) {
        if (focused_element_) {
            focused_element_->on_key_event(key_event);
        }
    }

    for (const auto& text_event : input_manager_.get_text_events()) {
        if (focused_element_) {
            focused_element_->on_text_event(text_event);
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
            if (interactive->on_pointer_event(pointer)) {
                if (pointer.state == PointerState::Pressed) {
                    set_focused_element(interactive);
                    captured_element_ = interactive;
                    pointer_pressed_x_ = pointer.x;
                    pointer_pressed_y_ = pointer.y;
                    captured_element_stolen_ = false;
                }
                return true;
            }
        }
    }

    return false;
}

} // namespace gooey::mvvmc