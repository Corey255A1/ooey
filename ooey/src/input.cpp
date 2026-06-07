#include "ooey/input.hpp"
#include <algorithm>

namespace ooey {

void InputManager::push_pointer_event(const Pointer& pointer) {
    Pointer scaled_pointer = pointer;
    if (scale_ != 1.0f && scale_ > 0.0f) {
        scaled_pointer.x = static_cast<int>(pointer.x / scale_);
        scaled_pointer.y = static_cast<int>(pointer.y / scale_);
    }
    pointer_events_.push_back(scaled_pointer);

    auto it = std::ranges::find_if(pointers_,
                           [&](const Pointer& p) { return p.id == scaled_pointer.id; });
                           
    if (it != pointers_.end()) {
        *it = scaled_pointer;
        if (scaled_pointer.state == PointerState::Released) {
            pointers_.erase(it);
        }
    } else if (scaled_pointer.state == PointerState::Pressed) {
        pointers_.push_back(scaled_pointer);
    }
}

void InputManager::push_key_event(const KeyEvent& key_event) {
    key_events_.push_back(key_event);
    if (key_event.state == KeyState::Pressed) {
        bool found = false;
        for (int k : pressed_keys_) {
            if (k == key_event.key_code) {
                found = true;
                break;
            }
        }
        if (!found) {
            pressed_keys_.push_back(key_event.key_code);
        }
    } else if (key_event.state == KeyState::Released) {
        for (auto it = pressed_keys_.begin(); it != pressed_keys_.end(); ++it) {
            if (*it == key_event.key_code) {
                pressed_keys_.erase(it);
                break;
            }
        }
    }
}

void InputManager::push_text_event(const TextEvent& text_event) {
    text_events_.push_back(text_event);
}

void InputManager::update() {
    pointer_events_.clear();
    key_events_.clear();
    text_events_.clear();
}

} // namespace ooey