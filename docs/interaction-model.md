# Interaction Model: Bridging Input and the Scene Graph

The OOEY GUI Engine relies on a Retained Mode Scene Graph and a Unified Abstract Input System. But how do we connect the two? How does a `Pointer` moving across a screen trigger a `Button`'s click event? And how does a keyboard interact with a UI without a mouse?

This document outlines the core interaction model.

## 1. The `IInteractive` Interface
Not all `IDrawable` objects care about input (e.g., a background `RectPrimitive` is purely visual). Objects that *do* care about input must implement the `IInteractive` interface.

To be interactive, an object must provide:
- **Hit-Testing:** A `bool contains(Point p)` method or a `Rect bounds()` to define its interactive physical space.
- **Event Handlers:** Callbacks like `on_pointer_event(PointerEvent& e)` and `on_key_event(KeyEvent& e)`.

## 2. The Controller Layer
The `Controller` acts as the bridge between the `InputManager` and the `View` hierarchy (the Scene Graph).
Every frame, the `Controller` queries the `InputManager` for new pointer and key events. It then routes those events into the visual hierarchy.

## 3. Pointer Routing: Top-Down Hit Testing (Tunneling/Bubbling)
When the `Controller` detects a `Pointer` state change (moved, pressed, released):

1. **Traversal in Reverse Z-Order:** The `Controller` queries the `root_view` and iterates through the tree from front-to-back (top-most elements first).
2. **Hit-Testing:** It checks if the `Pointer`'s current X/Y coordinates fall within the `IInteractive` element's bounding box.
3. **Event Consumption:** If an element is hit, its `on_pointer_event` is called. If the element *handles* the event (e.g., a Button is clicked), it returns `true`. The `Controller` then stops routing that event to prevent objects behind it from also receiving the click.
4. **Bubbling/Tunneling:** If the top-most object ignores the event, the `Controller` passes it down to the next object in the visual stack.

## 4. Keyboard Routing: Focus Management
For devices without pointer tracking (keyboards, gamepads, external hardware), we rely on **State-Based Focus**.

1. **The Focused Element:** The `Controller` maintains a reference to a single *Focused* `IInteractive` element in the scene graph.
2. **Direct Routing:** When a `KeyEvent` occurs, no hit-testing is performed. The event is sent *directly* to the focused element's `on_key_event`.
3. **Focus Navigation:** If the focused element ignores a key event (e.g., it ignores "Arrow Down"), the `Controller` intercepts it. The `Controller` can then calculate the spatial layout of the UI and move focus to the nearest interactable object below the currently focused one.

## Summary
By keeping pointer routing strictly geometric (hit-testing) and keyboard routing strictly state-based (focus management), OOEY can natively support everything from an X11 desktop with a mouse, to an embedded Raspberry Pi controlled solely by hardware buttons.

## 5. Input Routing Memory Safety and Dynamic Lifecycle Auditing

In a dynamic MVVMC environment (such as the wizard navigation flow in `hello_wizard`), user interactions (e.g., clicking a "Start" or "Continue" button) often trigger state updates that mutate the visual scene graph immediately. For example, a button click can trigger a page transition, which clears the parent container's children and destroys the clicked button itself.

### The Danger: Dangling Pointers and Iterator Invalidation
Because event loops dispatch multiple events in sequence and the `Controller` caches interactive elements (such as `captured_element_` for pointer drags and `focused_element_` for key focus), page transitions can result in:
1. An event handler executing navigation and clearing/destroying the active view object.
2. The traversal stack holding a dangling reference to a deleted child `shared_ptr` or an iterator over a destroyed child vector.
3. Subsequent events routing to a deleted object or calling virtual functions on a dangling pointer, leading to a **pure virtual function call exception** or a segmentation fault.

### The Remedy: Strong Shared Pointer Ownership and Stack Frame Preservation
To guarantee complete memory safety and lifecycle synchronization across dynamic view-swapping gestures, the `Controller` implements a two-fold safety mechanism:

1. **Stack Frame Preservation via Value Passing & Vector Copying**:
   - In `Controller::route_pointer_event`, the visual tree node is passed by value (`std::shared_ptr<IDrawable> node`), incrementing its reference count.
   - Before traversing children, the controller copies the children vector: `auto children = view->get_children()`. This ensures that even if a child callback clears the parent's children (mutating the original vector), the copy on the stack keeps all children alive and maintains iterator stability.

2. **Strong Shared Reference Caching**:
   - The `captured_element_` and `focused_element_` are stored as `std::shared_ptr<IDrawable>` instead of raw pointers. This prevents their reference count from dropping to 0 and keeps the elements alive until the routing phase or the current interaction is complete.
   - If an element is removed from the active visual tree, the `Controller` detects it on the next iteration via `contains_element(root_view_, element.get())` and safely nullifies the reference, at which point the memory is cleaned up.

This architecture ensures complete runtime memory safety and eliminates the risk of use-after-free, pointer reuse (ABA), and pure virtual call exceptions.

