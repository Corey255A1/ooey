# Architecture Refinements & Scalability Roadmap

This document outlines the evolutionary improvements made to the OOEY GUI Engine's architecture to support greater scalability, performance, and stability.

## 1. Preventing Subscription Memory Leaks (RAII Binding)
**The Problem:** `Property<T>` originally returned a `uint32_t` subscription ID. Views subscribing to Properties were forced to manually unsubscribe when destroyed. If a `View` forgot to unsubscribe and was destroyed before the `ViewModel`, the `Property` would fire a callback into a dangling memory address, causing a segfault. Additionally, if the `Property` was moved or copied, the memory address of `this` would change, breaking the unsubscribe mechanism.

**The Solution:**
- Introduced `ScopedSubscription` and `SubscriptionSink`.
- `Property<T>::subscribe` now returns a move-only `ScopedSubscription` that automatically triggers the `unsubscribe` logic in its destructor.
- `Property<T>` utilizes a `std::shared_ptr<bool> alive_flag_`. If the `ViewModel` (and thus the `Property`) is destroyed *before* the `View`, the `ScopedSubscription` will gracefully ignore the unsubscribe attempt.
- Disabled copy and move constructors on `Property<T>` to guarantee memory address stability for its internal listeners.

## Future Refinements Needed

### 2. Layout and Sizing Engine (Critical)
Currently, every primitive requires hardcoded absolute coordinates.
**Improvement:** Implement a layout system (Flexbox, Grid). Views should specify a `Size` (e.g., "Wrap Content", fixed pixels) and layout containers (`VBox`, `HBox`) should calculate absolute coordinates during a dedicated "Layout Pass".

### 3. Dirty Geometry & Optimization
Currently, `Application::run()` rebuilds the entire scene graph geometry *every single frame*.
**Improvement:** Implement a hierarchical "Dirty Flag" system. If a button changes color, only its geometry should be regenerated and patched into the cached buffer, rather than rebuilding the entire screen.

### 4. Local Coordinate Systems
Hit-testing currently occurs in absolute screen-space coordinates.
**Improvement:** Implement transform matrices on `View` nodes. Pointer coordinates must be converted from screen-space to local-space as they traverse down the tree, so child elements exist at `(0,0)` relative to their parents.

### 5. CMake Options and Libraries
Current the CMake file structure is very basic and when we get to compiling on another platform it isn't going to work. Additionally I want to structure the project such that you can include only the libraries that you need.