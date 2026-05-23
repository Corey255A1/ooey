#pragma once

#include <vector>
#include <memory>

namespace ooey {

enum class PointerState {
    Released,
    Pressed,
    Moved
};

struct Pointer {
    int id; // Unique ID for multi-touch or multiple mice
    int x;
    int y;
    PointerState state;
};

enum class KeyState {
    Released,
    Pressed
};

struct KeyEvent {
    int key_code; // Platform independent keycode eventually
    KeyState state;
};

class InputManager; // Forward declare

class IInputProvider {
public:
    virtual ~IInputProvider() = default;
    
    virtual void set_input_manager(InputManager* manager) = 0;

    // Abstract interface to fetch events and push them into InputManager
    virtual void poll_input() = 0;
};

class InputManager {
public:
    void push_pointer_event(const Pointer& pointer);
    void push_key_event(const KeyEvent& key_event);

    const std::vector<Pointer>& get_active_pointers() const { return pointers_; }
    const std::vector<Pointer>& get_pointer_events() const { return pointer_events_; }
    const std::vector<KeyEvent>& get_key_events() const { return key_events_; }
    
    // Clear transient states like 'Moved' if no longer moving, etc.
    void update();

private:
    std::vector<Pointer> pointers_; // active pointers
    std::vector<Pointer> pointer_events_; // events for this frame
    std::vector<KeyEvent> key_events_; // For this frame
};

} // namespace ooey