# Window Maximization & Interaction Memory Safety

This document details the architectural analysis, root causes, design resolutions, and implementation details for two critical system features:
1. **Interaction Memory Safety**: Eliminating pure virtual function call exceptions and use-after-free bugs during synchronous visual tree transitions.
2. **Native Window Maximization and Restoration**: Extending the custom client-side window chrome and integrating it with X11 and Wayland windowing backends.

---

## 1. Interaction Memory Safety & Lifecycle Auditing

### The Problem: Synchronous Visual Tree Restructuring
In OOEY's MVVMC architecture, user interaction (such as clicking a button) can trigger a command that modifies the view model state, which in turn rebuilds the visual tree. 
For example, clicking the "Start Wizard" button triggers:
```cpp
void Page1ViewModel::on_start_clicked() {
    coordinator_->navigate_to(std::make_shared<Page2ViewModel>(coordinator_));
}
```
This navigation is synchronous and triggers the view to swap:
```cpp
void NavigationShellView::recreate_page_view(std::shared_ptr<gooey::PageViewModelBase> vm) {
    page_container_->clear_children(); // Destroys the active page (Page1View) and the button!
    ...
}
```
At this exact moment:
1. We are still inside `Button::on_pointer_event` on the execution call stack.
2. The button's parent page (`Page1View`) is cleared, and because its reference count drops to 0, it and the button itself are deleted.
3. This leads to two critical vulnerabilities:

#### Vulnerability A: Call Stack Reference Invalidation
`Controller::route_pointer_event` was defined as:
```cpp
bool Controller::route_pointer_event(const Pointer& pointer, const std::shared_ptr<IDrawable>& node);
```
And traversed children using a const reference:
```cpp
const auto& children = view->get_children();
for (auto it = children.rbegin(); it != children.rend(); ++it) {
    if (route_pointer_event(pointer, *it)) { return true; }
}
```
When `clear_children()` is called:
- The parent `children_` vector is cleared, making `children` a dangling reference.
- The `std::shared_ptr` that `*it` refers to is destroyed.
- The iterator `it` is invalidated.
- Returning from `route_pointer_event` accesses these dangling references and causes undefined behavior or crashes.

#### Vulnerability B: Caching Dangling Raw Pointers
The `Controller` previously stored focus and capture targets using raw pointers (`IInteractive* captured_element_` and `focused_element_`).
When the button was destroyed inside its own `on_pointer_event` callback:
1. The destructor ran, updating the object's vtable pointer to point to the base classes.
2. The `Controller` continued to store the raw pointer to the now-deleted memory block.
3. Subsequent event loop iterations or cleanup checks (such as `contains_element(root_view_, captured_element_)` comparing the dangling pointer) would trigger virtual method calls on the deleted object, leading to a **pure virtual function call exception** (`__cxa_pure_virtual`) or segmentation fault.

---

### The Solution: Strong Shared Caching & Stack Frame Preservation
We refactored `Controller` to keep objects alive throughout the lifetime of the event dispatch and to stabilize recursive visual tree traversals.

#### 1. Stack Frame Preservation (Value Passing & Vector Copying)
We changed `route_pointer_event` to take the node by value:
```cpp
bool Controller::route_pointer_event(const Pointer& pointer, std::shared_ptr<IDrawable> node);
```
And modified the children traversal to copy the `std::shared_ptr` vector:
```cpp
auto children = view->get_children(); // Creates a copy, incrementing refcounts
for (auto it = children.rbegin(); it != children.rend(); ++it) {
    if (route_pointer_event(pointer, *it)) {
        return true;
    }
}
```
By copying the vector, all child views are guaranteed to remain alive on the call stack, and the iterator `it` remains perfectly valid, even if the parent view clears its internal `children_` vector mid-loop.

#### 2. Caching via `std::shared_ptr`
We changed `captured_element_` and `focused_element_` to `std::shared_ptr<IDrawable>`:
```cpp
std::shared_ptr<IDrawable> focused_element_{nullptr};
std::shared_ptr<IDrawable> captured_element_{nullptr};
```
If a button is clicked and removed from the active visual tree, its reference count is kept at $\ge 1$ by `captured_element_`. The button is kept alive until the click is completed (i.e. a `Released` pointer state is received), ensuring no virtual methods are ever executed on freed memory.

Once the interaction is completed or the next event loop begins, the controller runs `contains_element` to check if the element is still attached to the visual tree:
```cpp
if (captured_element_ && !contains_element(root_view_, dynamic_cast<IInteractive*>(captured_element_.get()))) {
    captured_element_ = nullptr;
}
```
If it is no longer in the tree, setting the `std::shared_ptr` to `nullptr` drops the refcount and safely reclaims the memory.

---

## 2. Native Window Maximization & Restoration

### The Layout Option in Custom Window Chrome
To support standard desktop windows, we added a **Maximize / Restore** toggle button directly into the custom client-side window chrome decoration layout ([window_chrome.cpp](file:///home/corey/code/ooey/ooey/src/renderer/window_chrome.cpp)).

#### Geometry Positioning:
The title-bar buttons are laid out from right to left:
- **Close Button**: `w - bw - 30` to `w - bw` (Width: 30)
- **Maximize Button**: `w - bw - 60` to `w - bw - 30` (Width: 30)
- **Minimize Button**: `w - bw - 90` to `w - bw - 60` (Width: 30)

#### Symbol Drawing Logic:
Based on the window maximized state, the button draws one of two vector symbols:
- **Maximize Symbol (Not Maximized)**: Draws a single, clean 10x10 square.
- **Restore Symbol (Maximized)**: Draws two overlapping 8x8 squares offset by 3px.

```cpp
bool max_state = is_maximized || maximized_;
if (max_state) {
    // Restore Symbol (overlapping double boxes)
    int bx1 = max_x + 12;
    int by1 = max_cy - 3;
    // ... draw back box ...
    int bx2 = max_x + 9;
    int by2 = max_cy - 6;
    // ... draw front box ...
} else {
    // Maximize Symbol (single box)
    // ... draw single box ...
}
```

---

### Integration with X11 Backend (EWMH)
The X11 Window Backend manages maximization using **Extended Window Manager Hints (EWMH)**.

#### 1. Querying Maximized State
We check if the window is currently maximized by querying the `_NET_WM_STATE` property for the vert and horz maximized atoms:
```cpp
bool WindowBackend::is_maximized() const {
    if (!display_ || !window_) return false;
    Atom actual_type;
    int actual_format;
    unsigned long num_items, bytes_after;
    unsigned char* prop_to_free = nullptr;
    Atom state_atom = XInternAtom(display_, "_NET_WM_STATE", True);
    if (state_atom == None) return false;

    bool is_max = false;
    if (XGetWindowProperty(display_, window_, state_atom, 0, 1024, False, XA_ATOM,
                           &actual_type, &actual_format, &num_items, &bytes_after, &prop_to_free) == Success) {
        if (prop_to_free) {
            Atom* atoms = reinterpret_cast<Atom*>(prop_to_free);
            Atom max_vert = XInternAtom(display_, "_NET_WM_STATE_MAXIMIZED_VERT", True);
            Atom max_horz = XInternAtom(display_, "_NET_WM_STATE_MAXIMIZED_HORZ", True);
            bool vert = false, horz = false;
            for (unsigned long i = 0; i < num_items; ++i) {
                if (atoms[i] == max_vert) vert = true;
                if (atoms[i] == max_horz) horz = true;
            }
            is_max = (vert && horz);
            XFree(prop_to_free);
        }
    }
    return is_max;
}
```

#### 2. Requesting Maximization / Restoration
We toggle the state by sending a `ClientMessage` of type `_NET_WM_STATE` to the root window, passing `1` to maximize (Add) or `0` to restore (Remove):
```cpp
void WindowBackend::change_maximized_state(bool maximize) {
    if (!display_ || !window_) return;
    XClientMessageEvent xev{};
    xev.type = ClientMessage;
    xev.window = window_;
    xev.message_type = XInternAtom(display_, "_NET_WM_STATE", False);
    xev.format = 32;
    xev.data.l[0] = maximize ? 1 : 0; // 1 = Add, 0 = Remove
    xev.data.l[1] = XInternAtom(display_, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    xev.data.l[2] = XInternAtom(display_, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    xev.data.l[3] = 1; // normal source
    xev.data.l[4] = 0;

    XSendEvent(display_, DefaultRootWindow(display_), False, 
               SubstructureRedirectMask | SubstructureNotifyMask, reinterpret_cast<XEvent*>(&xev));
}
```

---

### Integration with Wayland Backend
The Wayland Window Backend manages maximization using the `xdg_toplevel` shell extension protocol.

#### 1. State Tracking via Configure Events
Wayland notifies the client of maximized state changes via the `xdg_toplevel_configure` event. The state is represented as a `wl_array` containing enum values.
Because the standard `wl_array_for_each` macro performs an implicit `void*` conversion (which is illegal in C++ without explicit casts), we manually loop over the array data:
```cpp
static void xdg_toplevel_configure(void* data, xdg_toplevel* /*toplevel*/, int32_t width, int32_t height, wl_array* states) {
    WindowBackend* backend = static_cast<WindowBackend*>(data);
    if (!backend) return;

    bool maximized = false;
    if (states) {
        uint32_t* states_data = static_cast<uint32_t*>(states->data);
        size_t count = states->size / sizeof(uint32_t);
        for (size_t i = 0; i < count; ++i) {
            if (states_data[i] == XDG_TOPLEVEL_STATE_MAXIMIZED) {
                maximized = true;
            }
        }
    }
    backend->handle_xdg_toplevel_configure(width, height, maximized);
}
```
This synchronized state is cached in `is_maximized_` and passed down to `WindowChrome`.

#### 2. Requesting Maximization / Restoration
We dispatch requests using Wayland top-level protocol methods:
- **Maximize**: `xdg_toplevel_set_maximized(xdg_toplevel_)`
- **Restore**: `xdg_toplevel_unset_maximized(xdg_toplevel_)`

---

## 3. Verification & Compatibility Results

### Unit Testing Strategy
We implemented two dedicated unit tests in `tests/test_core_architecture.cpp` and `tests/test_primitives.cpp` to verify stability:
1. **`OoeyMvvmc.RealRealElementDeletions`**: Builds a nested view hierarchy and clicks a button that triggers synchronous parent and child destruction, verifying that the recursive traversal stack and references do not crash.
2. **`WindowChromeTest.HitTesting & HandlePointerEventMoveResizeClose`**: Simulates pointer interaction (Pressed & Released) on coordinates mapped to the new Maximize/Restore button, asserting that correct events are routed to the underlying backend.

All 64 tests successfully pass:
```
[ RUN      ] OoeyMvvmc.RealRealElementDeletions
[       OK ] OoeyMvvmc.RealRealElementDeletions (0 ms)
[ RUN      ] WindowChromeTest.HandlePointerEventMoveResizeClose
[       OK ] WindowChromeTest.HandlePointerEventMoveResizeClose (0 ms)
...
[==========] 64 tests from 14 test suites ran. (123 ms total)
[  PASSED  ] 64 tests.
```

### Android Compatibility
The NDK package compilation was validated to compile and sign successfully (`build_android_arm64-v8a/app.apk`), confirming that adding maximization APIs to desktop backends does not affect mobile graphics compilation paths.
