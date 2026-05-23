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

## Copilot Wayland Plan
Plan: Implement Native Wayland Platform
TL;DR — Add a new Wayland IWindowBackend and a matching IRenderTarget (initially software or EGL/GLES). Wire Wayland event handling to the existing InputManager, update CMake to optionally build the backend, and add an example. This preserves the existing app API and enables GPU or CPU render paths.

Steps
Implement Wayland backend: create include/ooey/platform/wayland/wayland_window_backend.hpp and src/platform/wayland/wayland_window_backend.cpp implementing IWindowBackend and IInputProvider (set_input_manager, create, destroy, poll_events, get_render_target).
Add render-target(s): add either a software ShmRenderTarget using wl_shm (easy, no GPU) at [src/platform/wayland/shm_render_target.cpp] or an EGL/GLES EGLRenderTarget using wl_egl_window and EGL at [src/platform/wayland/egl_render_target.cpp]. Implement clear, draw_geometry, measure_text, draw_text, present per IRenderTarget. See i_render_target.hpp.
Wire input & xdg-shell: in the backend, implement Wayland registry, wl_compositor, wl_shm/wl_egl_window, and xdg_wm_base (xdg-shell); map pointer/keyboard/text to InputManager via set_input_manager() and input.hpp APIs. Handle xkbcommon for keyboard events.
Update build: modify CMakeLists.txt to detect Wayland/EGL/GLES/xkbcommon and add new sources conditionally. Provide OOEY_BUILD_WAYLAND option and link to wayland-client, wayland-egl, EGL, GLESv2, xkbcommon.
Examples & testing: add an example in examples showing using the Wayland backend, and extend tests/examples CMake to build it when dependencies are present. Add simple runtime checks in hello_ooey.cpp to choose backend.
Iteration: first land a minimal wl_shm software backend (small effort) to validate event loop and input mapping, then follow with EGL/GLES GPU backend (medium/large effort).
Further Considerations
Backend choice: Option A — start with wl_shm (software) for faster integration; Option B — implement EGL/GLES immediately for hardware acceleration.
Key features: xdg-shell, xkbcommon support, text input methods (IM), clipboard, and fractional scaling—prioritize based on target compositors.
Questions

Do you want GPU-accelerated rendering (EGL/GLES) from the start, or prefer a software wl_shm prototype first?
Which compositors or environments should we validate on (weston, sway, GNOME/Wayland, XWayland interoperability)?
Is xkbcommon and IME/clipboard support required for the first pass?
Relevant files & references

i_window_backend.hpp — backend interface
i_render_target.hpp — render target API
input.hpp — input mapping and InputManager
x11_window_backend.hpp and x11_window_backend.cpp — existing X11 reference implementation
CMakeLists.txt — current build integrates X11/GLX
03-platform-implementation.md and tutorial-building-a-backend.md — design notes and tutorial
Next: I can draft the exact file skeletons, CMake changes, and a minimal wl_shm render-target prototype. Which option do you prefer (software wl_shm first, or GPU EGL/GLES)?