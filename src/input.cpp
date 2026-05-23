#include "ooey/input.hpp"
#include <algorithm>

namespace ooey {

void InputManager::push_pointer_event(const Pointer& pointer) {
    pointer_events_.push_back(pointer);

    auto it = std::find_if(pointers_.begin(), pointers_.end(),
                           [&](const Pointer& p) { return p.id == pointer.id; });
                           
    if (it != pointers_.end()) {
        *it = pointer;
        if (pointer.state == PointerState::Released) {
            pointers_.erase(it);
        }
    } else if (pointer.state != PointerState::Released) {
        pointers_.push_back(pointer);
    }
}

void InputManager::push_key_event(const KeyEvent& key_event) {
    key_events_.push_back(key_event);
}

void InputManager::update() {
    pointer_events_.clear();
    key_events_.clear();
}

} // namespace ooey