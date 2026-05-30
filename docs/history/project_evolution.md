# OOEY Architecture & Project Evolution

This document chronicles the steps taken, the refinements made, and the overall journey of the OOEY architecture from its inception to its current state. Keeping track of this history helps maintain the context behind key architectural decisions.

## 1. Initial Foundation
We started by establishing a core architecture that separated platform-specific details from application logic:
- `Application`: The central orchestrator running the main loop.
- `IWindowBackend` and `IRenderTarget`: Interfaces abstracting window management (e.g., X11) and rendering. This paved the way for cross-platform support.
- We defined strict C++20 conventions, focusing on modern memory management and strict naming rules.

## 2. API Refinement & Explicit Ownership
As the framework's API took shape, we recognized the need for crystal-clear ownership transfer to avoid memory lifecycle issues.
- **Refinement:** We enforced explicit move semantics (`&&`) for API methods that take ownership of objects. For example, `Application::set_window_backend(std::unique_ptr<IWindowBackend>&& backend)`.
- This change aligned with our goal of making the C++ API safe and hyper-explicit, signaling to the API consumer that the framework is taking ownership.

## 3. Retained Mode & Render Loop
Initially, rendering was handled through an arbitrary, user-defined render callback.
- **Refinement:** We shifted from an immediate-mode generic callback to a **Retained Mode Scene Graph**. The `Application` now holds a collection of geometries/drawables (`IDrawable`), taking responsibility for clearing the screen and iterating over these objects automatically.
- We provided optional hooks (`before_render_callback_` and `after_render_callback_`) for users who need custom logic just before or after the framework draws the scene graph, keeping the setup declarative but flexible.

## 4. Interaction Model & Input Routing
With visual elements in place, we needed a robust way to handle user input across different paradigms (touch, mouse, keyboard).
- We introduced `IInteractive` to define UI elements capable of receiving input.
- We established an event-bubbling hierarchy where pointer events (clicks, hovers) hit-test against visual boundaries from the top-down (Z-order) until handled.
- For environments lacking pointer devices (e.g., keyboard-only setups), we framed the role of a `Controller` to manage focus state and handle navigation inputs independently of raw screen coordinates.

## 5. Structural Reorganization & Modularity
As the codebase grew, we shifted focus to maintainability and modular structure.
- **Header vs. Source:** We cleaned up monolithic headers and separated implementation details into `.cpp` files to reduce compilation times and adhere to standard C++ practices.
- **Logical Grouping:** We created dedicated folders like `controls/` (for UI elements like `Button`) and `primitives/` (for basic shapes).
- **Library Scalability:** We structured components so that modules like `ooey_controls` could eventually be compiled as optional libraries. This allows scaling down for platforms that don't need complex UI controls.

## 6. The MVVM-C Framework
To support complex, state-driven UI interactions cleanly, we introduced the Model-View-ViewModel-Controller (MVVM-C) pattern.
- We implemented a reactive `Property<T>` template that broadcasts changes to subscribers.
- We proved the pattern with the `hello_ooey_mvvmc.cpp` example: clicking one box updates a `ViewModel` property, which automatically notifies the other box's `View` to update its visual color.
- This pattern completely separated visual representation (`View`), application logic (`ViewModel`/`Controller`), and state, making the UI highly testable and loosely coupled.

## 7. Memory Management & MVVM-C Refinement
In validating the MVVM-C features, we addressed critical ergonomic and memory concerns.
- **Memory Leaks:** We resolved memory leaks in the event pipeline and subscription tracking.
- **Ergonomics:** We improved the subscription model. Initially, users had to call a clunky `add()` method on a `SubscriptionSink`. We refined `ScopedSubscription` and `SubscriptionSink` to be more intuitive, managing lifecycle automatically.
- **Organization:** We moved all reactive components (`property.hpp`, `scoped_subscription.hpp`, `subscription_sink.hpp`) into a dedicated `mvvmc/` directory and ensured their implementations were appropriately split between headers and source files where template mechanics allowed.

## 8. Wayland Subclassing & Vulkan Pipeline Integration
To support advanced graphics APIs and modularize platform backends, we refactored the Wayland window implementation:
- **Subclassing:** Separated EGL/OpenGL code out of the base `WindowBackend` class. The base class now provides common Wayland input/window features, while `EglWindowBackend` and `VulkanWindowBackend` handle EGL/GL and Vulkan context setups respectively as clean subclasses.
- **Vulkan Graphics Pipeline:** Implemented `VulkanRenderTarget` to support hardware-accelerated Vulkan rendering. It includes push constants for coordinate mapping, dedicated pipelines for Triangles and Lines, dynamic VBO/IBO buffers, and embedded SPIR-V shader bytecode for self-contained compilation.
- **Runtime Selection:** Updated `platform.cpp` to dynamically select the Wayland subclass at runtime via the `OOEY_WAYLAND_BACKEND` environment variable.

## 9. Reactive Two-Pass Layout Engine
To move beyond hardcoded coordinates and enable responsive UI design, we implemented a reactive, constraint-based two-pass layout system (Measure & Arrange) in `gooey::View`:
- **Base View Properties:** Added `SizePolicy`, margins, padding, and alignments to the base `View` class.
- **Layout Containers:** Introduced `Column` (vertical stacks), `Row` (horizontal stacks), `Grid` (tabular layouts), and `FlowLayout` (wrapping flexbox layout) as first-class containers.
- **Main Loop Integration:** Integrated the layout engine passes (`measure` and `layout`) directly into `Application::run_iteration()` immediately before rendering, allowing the UI layout to automatically reflow dynamically in response to window resize events.

## 10. Decoupled Style-Name Theme Manager
To provide maximum flexibility and remove global state, we refactored the theme system:
- **Singleton Removal:** Replaced the global `ThemeManager` singleton with dynamic instances owned by the application or view model.
- **De-hardcoding:** Decoupled themes from the framework source. Users register custom themes and styles dynamically in their bootstrapper.
- **Style Mapping:** Added `set_style_name()` and `apply_style()` to bind controls (such as `Button`, `Label`, and `TextBox`) to named style configurations that automatically apply visual updates when the active theme cycles.

## 11. Modular Image Decoding & Rendering Subsystem
To support rich media layouts, we added a modular image loading and rendering framework:
- **Unified Raw Buffer:** Introduced `Image` holding a standard 32-bit RGBA pixel array.
- **API Decouplers:** Added `IImageDecoder` and `ImageDecoderRegistry` to verify file format magic bytes (BMP, PNG) and decode them dynamically.
- **Optional Dependencies:** Conditionalized the PNG decoder with CMake and `libpng` detection, keeping the core engine clean of unneeded third-party libraries.
- **Target Drawing:** Extended `IRenderTarget` with `draw_image` and implemented it with nearest-neighbor alpha blending in CPU (Software), GL texture binding in OpenGL, geometric fallback grid rendering in Vulkan, and coordinate offsets in Chrome decorations.
- **Layout Element:** Built the `ImageControl` view component to load, scale, and render images inside row, column, and flow layouts.

## 12. Performance Optimization & Flickering Mitigation
To support complex UI layouts without bottlenecks, we undertook a rendering performance push:
- **Command Batching & Caching:** Hoisted rendering boundaries (like `glBegin`/`glEnd` in OpenGL) out of tight character glyph loops. In Vulkan, we introduced an `image_geometry_cache_` to cache unit-space downsampled quads and batched drawing commands, resulting in a **70% speedup** in Vulkan image rendering benchmarks.
- **Dynamic Memory Safety:** Implemented dynamic Vulkan vertex and index buffer resizing to prevent crashes and memory overflows under high geometry loads.
- **Empty-Primitive Flicker Fix:** Diagnosed and fixed a driver-specific (especially LLVMpipe software rasterizer) rendering bug where empty `glBegin`/`glEnd` blocks (triggered by drawing empty string text in `TextBox`) caused subsequent rendering commands (like the greeting label) to flicker. Prevented this by adding early returns for empty text in all backends (`draw_text`).

## 13. Dynamic Cross-Platform Font Rendering
To support high-fidelity text rendering with native system fonts, we implemented a modular, cross-platform font loading and rasterization subsystem:
- **Modular Backend:** Defined the `IFontBackend` interface to isolate platform-specific font loading and drawing.
- **Dynamic Loading:** Implemented `LinuxFontBackend`, which uses `dlopen`/`dlsym` at runtime to resolve symbols from standard system libraries (`libfontconfig.so.1` and `libfreetype.so.6`) without compile-time link dependencies. It matches requested font weights and styles to actual TTF/OTF files on the filesystem.
- **Cross-Platform Extensibility:** Stubbed out `Win32FontBackend` to define the architecture for a Windows DirectWrite implementation.
- **Unified Fallback:** Integrated an automatic fallback to the static `BitmapFont` implementation if dynamic loading of system APIs or matching fonts fails.
- **Control Integration:** Updated standard UI components (`Label`, `TextBox`, `Button`, `ListControl`) and rendering targets to support dynamic font measuring, layout alignment, vertical centering, and custom family/style/weight settings.

## 14. Real-time Cross-Platform System Monitor Dashboard
We developed a highly responsive, styled system metrics visualizer showing hardware health in real-time:
- **Unified OS Metrics API**: Built system data collectors querying `/proc/stat` and `/proc/meminfo` on Linux, coupled with native Win32 `GetSystemTimes` and `GlobalMemoryStatusEx` on Windows, and utilizing C++17 `<filesystem>` for cross-platform disk capacity statistics.
- **Process List Harvesting**: Implemented process parsing extracting running PIDs, process names (from `comm`), states, and resident set sizes (RSS bytes), sorting them by RAM consumption.
- **Dynamic Component Styling**: Overrode `apply_style` in `ListControl` to dynamically map foreground, background, border, selection highlight, and text colors inside the theme manager.
- **Interactive Multi-Theme Support**: Configured four visually distinct styles (Dark, Light, Hacker green, Lofi warm pastel) that map active vs. inactive button highlights and layouts declaratively purely through style names, requiring zero procedural switch logic.

## Summary
The current architecture of OOEY represents a modern, C++20 reactive UI framework. By starting with a solid abstraction layer, adopting a retained mode scene graph, structuring the codebase for modularity, and layering a decoupled MVVM-C reactive system on top, OOEY provides a robust, explicit, and scalable foundation for cross-platform UI development.