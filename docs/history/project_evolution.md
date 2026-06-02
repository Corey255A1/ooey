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

## 15. WebAssembly/Emscripten Compatibility & Refinements
To support seamless cross-compilation of the entire framework to WebAssembly/HTML5 via Emscripten, we resolved several system-level configuration issues:
- **Vulkan Driver Guarding:** Isolated the Vulkan rendering engine (`VulkanRenderTarget`) inside CMake and added preprocessor guards to prevent compiling with native Vulkan headers on the WASM target.
- **Ambiguity and Header Clashes:** Resolved conflicting forward declarations of the `Image` class inside the `gooey` controls namespace.
- **Backend & Target Isolation:** Excluded framebuffer testing targets (which depend on `<linux/fb.h>`) and platform-specific X11 sample targets from building when compiling for WebAssembly.
- **Demanded API Compliance:** Updated invalid calls to primitive drawing shapes (such as `SinusoidPrimitive`) in Emscripten examples to match standard layout signatures.
- **Emulation Runtime Fix:** Fixed a WebAssembly runtime crash in legacy GL emulation context setup by explicitly forcing initialization of the `GLImmediate` state via `EM_ASM` inside the Emscripten window backend.
- **WebGL Texture Format Compatibility:** Resolved WebGL invalid internalformat errors (`GL_RGBA8` vs `GL_RGBA`) on Emscripten targets by conditionalizing `glTexImage2D` calls in `gl_render_target.cpp` to use the unsized `GL_RGBA` format.
- **Canvas Scaling Correction:** Set the internal resolution of the HTML5 canvas element (`canvas.width` and `canvas.height`) in `window_backend.cpp` using the requested window dimensions to prevent the browser from blurry-stretching a default-sized canvas.

## 16. Pure C++ Android NDK Porting & NativeActivity Integration
To support deploying OOEY applications to mobile platforms without Android Studio, we finalized the pure C++ NDK build pipeline:
- **Desktop Backend Disabling:** Configured CMake to automatically disable desktop-only backends (X11, Wayland, and Framebuffer) when compiling for `ANDROID`, aligning with the existing WebAssembly constraints.
- **NDK Path Discovery:** Improved dynamic discovery of `ANDROID_NDK_HOME` in `CMakeLists.txt` using the compiler toolchain-defined `CMAKE_ANDROID_NDK` and `ANDROID_NDK` cache variables, with a fallback to the host shell environment variables.
- **GL Target Isolation on Mobile:** Excluded the desktop-legacy `gl_render_target.cpp` from compiles on Android since the Android window backend relies entirely on `SoftwareRenderTarget` and `ANativeWindow_Buffer` locking, bypassing the lack of desktop immediate-mode OpenGL headers (`GL/gl.h`).
- **Private Dependency Mapping:** Mapped `gooey/include` as a private include path for the `ooey` library under Android, enabling `ooey` to compile `android_main.cpp` (which instantiates the top-level `gooey::Application` and intercepts OS NativeActivity commands) without circular header links.
- **Linker Entry Point Export:** Fixed a runtime crash (`ANativeActivity_onCreate not found`) caused by the static linker dropping unreferenced entry point code by adding `target_link_options(ooey INTERFACE "-Wl,-u,ANativeActivity_onCreate")`. This forces the linker to preserve and export the JNI entry point in all downstream shared library executable targets.
- **Dynamic C++ Entry Routing:** Refactored `android_main` to store the NDK context globally and invoke the application's standard `main()` function directly. Updated `create_default_window_backend()` to retrieve this global pointer, enabling cross-platform C++ application entries (like `hello_ooey.cpp`) to run completely unchanged on mobile.
- **Build Script & Signer Fixes:** Refined `build_apk.sh` to grab the compiled shared object library from `build_android_arm64-v8a/examples/` (its actual output location) rather than `lib/`, and updated the `apksigner` parameters from `--keystore` to `--ks` to comply with the standard Android SDK signing CLI signatures.
- **Native Window Geometry and Format Synchronization:** Resolved a critical startup segmentation fault (`SIGSEGV` / `SEGV_ACCERR` inside `present_software_frame`) caused by mismatched default pixel format assumptions in the locked OS window buffer. By calling `ANativeWindow_setBuffersGeometry` explicitly upon window creation and resize, the framework enforces the target `WINDOW_FORMAT_RGBA_8888` pixel layout alongside dynamic device resolutions, ensuring memory-safe row-by-row layout copies.

## 17. Android Vulkan Hardware Acceleration & Fallback Subclassing
To enable high-performance hardware-accelerated rendering on mobile platforms, we integrated a clean Vulkan subclassing architecture:
- **Modular Refactoring:** Refactored the base Android `WindowBackend` class by extracting the software rendering buffer allocation and lock-and-copy routines into virtual hooks (`init_graphics_context`, `cleanup_graphics_context`, and `recreate_render_target`).
- **Android Vulkan Subclass:** Created `VulkanWindowBackend` inheriting from `WindowBackend` to initialize a Vulkan instance with `VK_KHR_android_surface` support, bind the underlying window to a Vulkan surface via `vkCreateAndroidSurfaceKHR`, and run rendering through the shared `VulkanRenderTarget`.
- **Runtime Selection and Transparent Fallback:** Integrated dynamic backend selection using the `OOEY_ANDROID_BACKEND` environment variable. Also designed a robust fallback mechanism that automatically reverts to base software rendering if Vulkan context creation fails at launch, preserving runtime stability.

## 18. SysInfo Dashboard Modularization & Android Application Integration
To improve maintainability and clean structure, we modularized the monolithic `hello_sysinfo.cpp` example and configured it as the default Android application:
- **Dedicated Subdirectory**: Created `examples/sysinfo/` to house distinct, clean architectural segments of the system metrics application.
- **Isolating Core Metrics**: Moved the system queries into `metrics.hpp` and `metrics.cpp`. The code maps Win32 system times on Windows, alongside `/proc/stat` and `/proc/meminfo` parsing on Linux and Android, allowing real-time device stats to load natively and transparently.
- **Custom Control Extraction**: Isolated the custom panel decorator `StyledPanel` (implementing background graphics rendering using `RoundedRectPrimitive` layout bounds) inside `styled_panel.hpp` and `styled_panel.cpp`.
- **View & ViewModel Separation**: Segmented state bindings into `view_model.hpp`/`view_model.cpp` (exposing reactive properties like `cpu_text`, `ram_text`, `disk_text`, and virtualized `process_rows`) and layout declarations into `view.hpp`/`view.cpp` (establishing rows/columns/grids and connecting visual highlights).
- **Style Registry**: Modularized visual themes inside `themes.hpp` and `themes.cpp` to register the custom visual aesthetics (Dark, Light Clean, Hacker Green, Soft Lofi) with the active `ThemeManager`.
- **Android Integration**: Swapped compile targets in `build_apk.sh` to build `hello_sysinfo` and copy the resulting `libhello_sysinfo.so` shared library. Modified `AndroidManifest.xml` to load `hello_sysinfo` on boot, enabling high-fidelity hardware-accelerated Vulkan dashboards to run natively on mobile devices.

## 19. DPI-Aware Scaling System & High-Density Display Support
To resolve tiny UI layouts and text sizes on high-DPI screens (such as high-density mobile and Retina displays), we integrated a renderer-agnostic scaling pipeline:
- **Logical Canvas Sizing:** Sized application layout calculations, margins, padding, and font sizes in logical pixels, dividing the physical window size by the active DPI scale factor.
- **DPI Autodetection:** Overrode the virtual `get_content_scale()` method on target backends. On Android, the backend queries screen density via the NDK `AConfiguration_getDensity` API. On WebAssembly, it reads the browser's device pixel ratio via `emscripten_get_device_pixel_ratio()`. On X11, it queries the X resource database for `Xft.dpi` and falls back to `GDK_SCALE`. On Wayland, it reads `GDK_SCALE` for scale factor detection. Other platforms fallback to a default scale of `1.0f`.
- **DPI Decorator (`ScaledRenderTarget`):** Created a decorator for `IRenderTarget` that scales drawing operations (rectangles, lines, text sizes, image rectangles, and text measurements) by the resolved scale factor, keeping the underlying renderer code completely scale-independent.
- **Input Coordinate Rescaling:** Integrated coordinates mapping inside the `InputManager` to translate incoming physical pointer events (clicks, drags, touches) back into logical canvas coordinates, ensuring hit-testing remains pixel-perfect.
- **Configuration Control:** Enabled scaling by default while exposing simple programmatic toggles (`set_dpi_scale_enabled`) and supporting custom manual scaling factors or runtime disablers via the `OOEY_SCALE` environment variable.
- **Comprehensive Unit Testing:** Authored a complete unit test suite (`test_dpi_scaling.cpp`) validating target geometry scaling, image rectangle scaling, font sizing and text measurements scaling, pointer coordinates adjustment in `InputManager`, and application environment overrides.

## 20. Responsive Reflowing Layout & AdaptiveStack Integration
To ensure the system monitor dashboard remains highly usable on both landscape desktop monitors and portrait mobile screens, we implemented a responsive layout architecture:
- **`AdaptiveStack` Component:** Created a new responsive container (`adaptive_stack.hpp`/`.cpp`) that dynamically switches orientation. It lays out children horizontally like a `Row` when the available width is above a breakpoint (default 680px), and stacks them vertically like a `Column` when the width falls below it.
- **Dynamic Sizing & Height Allocation:** Configured `AdaptiveStack` to stretch children horizontally on mobile by default to fill the screen width safely. In vertical stacked mode, it calculates remaining height and distributes it proportionally among child elements that have `MatchParent` height policies (such as the processes `DataGrid`), avoiding layout overflows.
- **FlowLayout Button Arranger:** Leveraged `FlowLayout` inside the theme selection panel to contain theme buttons. On narrow displays (mobile), the buttons reflow into a neat 2x2 grid, saving significant vertical screen space, while automatically stacking as a column on desktop.
- **Unified Unit Testing:** Added dedicated tests to `test_layout.cpp` to verify correct horizontal and vertical layout sizing, spacing, margins, child stretching, and flex height distribution.

## 21. Viewport Scrolling, Implicit Capture, & Drag Interception
To support scrolling on vertical/portrait displays, we implemented viewport scrolling with touch/pointer drag:
- **`ScrollContainer` Viewport:** Built a vertical scroll container that measures content height with unconstrained boundaries and offsets layouts by `-scroll_offset_y`.
- **Implicit Pointer Capture:** Modified the input routing system to lock subsequent pointer events to the pressed target, preventing loss of drag tracking.
- **Touch-Slop Interception:** Implemented scroll gesture interception. If a user swipes vertically by $\ge 8$ logical pixels starting on a child component (like a button or table), the controller cancels the child press and routes all drag control directly to the parent `ScrollContainer` for seamless scroll transitions.
- **Unconstrained Sizing Fallback:** Fixed layout conflicts inside unconstrained viewports by forcing `MatchParent` sizing policies to fall back to natural content wrap heights when constraint sizes are very large ($\ge 50000$px), resolving vertical infinite stretch loops in controls like `DataGrid`.

## 22. Window Maximize and Restore Chrome Controls
To support standard desktop windows, we added a Maximize/Restore toggle button directly into the custom client-side window chrome decoration layout:
- **Geometry & Icon Rendering:** The title-bar button is positioned left of the close button. Depending on the maximization state, it draws a single 10x10 square (Maximize) or two overlapping 8x8 squares (Restore).
- **X11 Backend (EWMH):** Queries and toggles the maximized state by sending client messages with `_NET_WM_STATE_MAXIMIZED_VERT` and `_NET_WM_STATE_MAXIMIZED_HORZ` atoms to the root window.
- **Wayland Backend (xdg_toplevel):** Hooks into the `xdg_toplevel_configure` event to monitor maximized state flags and triggers state transitions using `xdg_toplevel_set_maximized` and `xdg_toplevel_unset_maximized`.
- **Unit Testing:** Implemented automated test suites simulating pointer hit-testing and event dispatching on the maximize button.

## 23. CPU Profiling and Idle Throttling Engine
To minimize background CPU usage, we transitioned the framework to a fully event-driven layout and rendering loop:
- **Rendering Invalidation:** Render passes are skipped unless a layout dirtiness (`layout_dirty`), window resize event, or explicit render request (`needs_render_`) is active.
- **Adaptive Event Polling:** Resolves VSync beating issues by shifting between an active state (non-blocking event loop with a 1ms yield sleep when no renders are pending) and an idle state (blocking the OS event socket inside `poll` or equivalent kernel wait for up to 100ms).
- **Background Worker Marshalling:** Offloads heavy telemetry operations (such as `/proc` files crawling in `hello_sysinfo`) to background threads. Dispatched tasks are marshaled to the main thread via a mutex-protected queue inside the `Application` main loop.
- **Wayland Starvation Fix:** Utilizes poll-driven file descriptor reads to prevent Wayland socket starvation during idle states.

## 24. General-Purpose RichText and Syntax Highlighting Notepad Example
To support advanced text editing capabilities, we implemented a decoupled multiline rich text architecture:
- **Decoupled Architecture:** Created the general-purpose, syntax-agnostic `RichTextBox` control. The specialized `CodeEditor` inherits from it, running C++ tokenizer/lexer analysis and mapping color schemes using `RichTextBox` formatting APIs.
- **Formatting Ranges:** Created `split_line_into_segments()` to decompose lines into individual styled runs (color, font weight, style, and size) using a sorted format range collection.
- **Style-Aware Coordinate Mapping:** Implemented `get_column_x_offset()` to calculate character offsets using segment-specific font configurations, ensuring perfect caret placement and selection highlighting.
- **Clipboard APIs:** Provided high-level `get_selected_text()` and `insert_text()` methods supporting multiline operations and cursor adjustments.

## 25. Vector Paint Application and CanvasLayout Coordinates Mapping
To enable absolute graphics layout modeling, we introduced canvas layouts and vector drawing controls:
- **`CanvasLayout` View:** An interactive container that maps absolute child boundary coordinates to relative screen-space coordinate positions.
- **`VectorShapeView` Hierarchy:** Shapes (`CircleShapeView`, `PolygonShapeView`) render a selection border with four resize corner handles (TL, TR, BL, BR) when selected. A pointer state machine manages shape dragging and resize transformations.
- **Normalized Vertices Scaling:** Polygons store vertex coordinates as normalized positions $(rx_i, ry_i) \in [0.0, 1.0]$ relative to the shape's original bounding box. This resolves scaling coordinates dynamically in `do_layout` when a shape is resized.
- **Ray-Casting Hit-Testing:** Implemented the even-odd ray-casting algorithm to support pointer hover and click hit-testing for arbitrary closed polygons.

## 26. ScrollContainer Composition Refactoring & Combined Horizontal-Vertical Scrolling
To unify scrolling behavior across the framework, we refactored scrolling mechanics around container composition:
- **Bidirectional ScrollContainer:** Upgraded `ScrollContainer` to support simultaneous vertical and horizontal scrollbars. It resolves measurement conflicts by only clamping child constraints to the viewport dimensions if the child's corresponding policy is `MatchParent`, allowing `WrapContent`/`Fixed` children (like text viewports) to correctly report their overflow sizes and show scrollbars.
- **RichTextBox Scroll Composition:** Removed manual scrolling calculations from `RichTextBox`. The text workspace is nested inside `ScrollContainer` as a `RichTextContentView` child component.
- **Viewport Visibility Mapping:** Added `scroll_to_visible(Rect)` to dynamically scroll viewports to keep cursor rects within visible boundaries.
- **Touch-Slop Gesture Capture:** Configured pointer capturing to intercept drag actions. Exceeding an 8px touch-slop threshold transfers pointer focus from child views (like buttons or cells) to the parent `ScrollContainer` for smooth scroll dragging.
- **TextBox Offset Handling:** Single-line text boxes clip children automatically and offset rendering coordinates horizontally when text overflows the box's boundaries.

## Summary
The current architecture of OOEY represents a modern, C++20 reactive UI framework. By starting with a solid abstraction layer, adopting a retained mode scene graph, structuring the codebase for modularity, layering a decoupled MVVM-C reactive system, and equipping it with DPI-aware auto-scaling, responsive layout controls, and decoupled scrollable rich text and canvas layouts, OOEY provides a robust, explicit, and highly usable foundation for cross-platform UI development (including Linux desktop, WebAssembly/HTML5, and pure Android native hardware-accelerated platforms).