#include "gooey/mvvmc/controller.hpp"
#include "gooey/mvvmc/i_drawable.hpp"
#include "gooey/controls/scroll_container.hpp"
#include "gooey/controls/scrollbar.hpp"
#include <cmath>
#include <ranges>

namespace {

bool contains_element(const std::shared_ptr<gooey::IDrawable>& node, gooey::mvvmc::IInteractive* target) {
    if (!node || !target) return false;
    if (dynamic_cast<gooey::mvvmc::IInteractive*>(node.get()) == target) {
        return true;
    }
    auto* view = dynamic_cast<gooey::GooeyNode*>(node.get());
    if (view) {
        for (const auto& child : view->get_children()) {
            if (contains_element(child, target)) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<gooey::IDrawable> find_shared_ptr(const std::shared_ptr<gooey::IDrawable>& node, gooey::mvvmc::IDrawable* target) {
    if (!node || !target) return nullptr;
    if (node.get() == target) {
        return node;
    }
    auto* view = dynamic_cast<gooey::GooeyNode*>(node.get());
    if (view) {
        for (const auto& child : view->get_children()) {
            auto found = find_shared_ptr(child, target);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

} // namespace

namespace gooey::mvvmc {
    using namespace ooey;

Controller::Controller(InputManager& input_manager, std::shared_ptr<GooeyNode> root_view)
    : input_manager_(input_manager), root_view_(std::move(root_view)) {}

void Controller::process_events() {
    if (captured_element_ && !contains_element(root_view_, dynamic_cast<IInteractive*>(captured_element_.get()))) {
        captured_element_ = nullptr;
        captured_element_stolen_ = false;
    }
    if (focused_element_ && !contains_element(root_view_, dynamic_cast<IInteractive*>(focused_element_.get()))) {
        focused_element_ = nullptr;
    }

    for (const auto& pointer_event : input_manager_.get_pointer_events()) {
        if (captured_element_ && !contains_element(root_view_, dynamic_cast<IInteractive*>(captured_element_.get()))) {
            captured_element_ = nullptr;
            captured_element_stolen_ = false;
        }

        if (captured_element_) {
            // Check if we can intercept the drag for ScrollContainer
            if (pointer_event.state == PointerState::Moved && !captured_element_stolen_) {
                int dy = pointer_event.y - pointer_pressed_y_;
                if (std::abs(dy) >= 8) {
                    auto* curr = dynamic_cast<GooeyElement*>(captured_element_.get());
                    gooey::controls::ScrollContainer* scroll_container = nullptr;
                    while (curr) {
                        auto* scroll = dynamic_cast<gooey::controls::ScrollContainer*>(curr);
                        if (scroll && scroll->needs_scroll()) {
                            scroll_container = scroll;
                            break;
                        }
                        curr = curr->get_parent();
                    }

                    if (scroll_container && captured_element_.get() != scroll_container &&
                        !dynamic_cast<gooey::controls::ScrollBar*>(captured_element_.get())) {
                        
                        // Cancel current captured element by sending a Released event outside
                        auto* cap_interactive = dynamic_cast<IInteractive*>(captured_element_.get());
                        if (cap_interactive) {
                            cap_interactive->on_pointer_event(Pointer{.id=pointer_event.id, .x=-9999, .y=-9999, .state=PointerState::Released});
                        }
                        
                        // Switch capture to ScrollContainer
                        auto scroll_shared = find_shared_ptr(root_view_, scroll_container);
                        if (scroll_shared) {
                            captured_element_ = scroll_shared;
                            captured_element_stolen_ = true;
                            
                            // Initialize ScrollContainer drag
                            scroll_container->on_pointer_event(Pointer{.id=pointer_event.id, .x=pointer_pressed_x_, .y=pointer_pressed_y_, .state=PointerState::Pressed});
                        }
                    }
                }
            }

            // Route to captured (or stolen) element
            if (captured_element_) {
                auto* cap_interactive = dynamic_cast<IInteractive*>(captured_element_.get());
                if (cap_interactive) {
                    cap_interactive->on_pointer_event(pointer_event);
                }
            }

            if (pointer_event.state == PointerState::Released) {
                captured_element_ = nullptr;
                captured_element_stolen_ = false;
            }
        } else {
            route_pointer_event(pointer_event, root_view_);
        }
    }

    for (const auto& key_event : input_manager_.get_key_events()) {
        if (focused_element_ && !contains_element(root_view_, dynamic_cast<IInteractive*>(focused_element_.get()))) {
            focused_element_ = nullptr;
        }
        if (focused_element_) {
            auto* focus_interactive = dynamic_cast<IInteractive*>(focused_element_.get());
            if (focus_interactive) {
                focus_interactive->on_key_event(key_event);
            }
        }
    }

    for (const auto& text_event : input_manager_.get_text_events()) {
        if (focused_element_ && !contains_element(root_view_, dynamic_cast<IInteractive*>(focused_element_.get()))) {
            focused_element_ = nullptr;
        }
        if (focused_element_) {
            auto* focus_interactive = dynamic_cast<IInteractive*>(focused_element_.get());
            if (focus_interactive) {
                focus_interactive->on_text_event(text_event);
            }
        }
    }
}

void Controller::set_focused_element(std::shared_ptr<IDrawable> element) {
    focused_element_ = std::move(element);
}

bool Controller::route_pointer_event(const Pointer& pointer, const std::shared_ptr<IDrawable>& node) {
    if (!node) return false;

    // Traverse children first (top-most elements usually drawn last, so reverse order)
    auto* view = dynamic_cast<GooeyNode*>(node.get());
    if (view) {
        auto children = view->get_children();
        for (auto & it : std::ranges::reverse_view(children)) {
            if (route_pointer_event(pointer, it)) {
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
                    set_focused_element(node);
                    captured_element_ = node;
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