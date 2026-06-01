# 13. Responsive Sizing & DPI-Aware Scaling Guide

This guide details the architectural design and developer API for building High-DPI aware, responsive, and cross-platform user interfaces in the OOEY framework.

---

## 1. High-DPI Scaling System

High-DPI screens (Retina, 4K desktop, or high-density mobile displays) pack many physical pixels per inch. Rendering layouts with raw physical coordinates results in tiny, unreadable text and interactive buttons. OOEY solves this via a decoupled **Logical-to-Physical Coordinate Mapping** using a decorator design pattern.

### 1.1 Sizing Philosophy: Logical Pixels

All elements inside OOEY (layout measurements, margins, padding, text, and font sizes) are computed and specified in **logical pixels**.
* The parent application resolves the active DPI scale factor $S$.
* The physical window size $(W_{phys}, H_{phys})$ is mapped to logical bounds:
  $$(W_{logical}, H_{logical}) = \left(\frac{W_{phys}}{S}, \frac{H_{phys}}{S}\right)$$
* The layout engine measures and arranges the views using $(W_{logical}, H_{logical})$. All UI components store their layout bounds in logical coordinates.

### 1.2 Platform Autodetection

`IWindowBackend` exposes a virtual method `get_content_scale() const` which defaults to `1.0f`. Backends override this to return device-specific DPI scale factors:

* **Android**: Queries screen configuration density via NDK asset manager configuration, divided by the base density value (`160` DPI):
  ```cpp
  float WindowBackend::get_content_scale() const {
      // Fetches density via AConfiguration_getDensity and returns density / 160.0f
  }
  ```
* **WebAssembly (Emscripten)**: Resolves the browser window's pixel ratio using HTML5 runtime queries:
  ```cpp
  float WindowBackend::get_content_scale() const {
      return static_cast<float>(emscripten_get_device_pixel_ratio());
  }
  ```
* **X11**: Queries the root window resource database for `Xft.dpi` (divided by baseline `96` DPI) and falls back to check the `GDK_SCALE` environment variable.
* **Wayland**: Queries standard environment-level desktop scale configurations (e.g. `GDK_SCALE`).

### 1.3 The Renderer Decorator (`ScaledRenderTarget`)

To keep drawing backends (Vulkan, EGL, Software) simple, OOEY utilizes `ScaledRenderTarget` to wrap drawing targets:
* **Geometry**: Vertices $(x, y)$ are multiplied by $S$ before being written to the underlying target.
* **Images**: Destination rectangles are scaled by $S$.
* **Text**: Font sizes and positions are scaled up by $S$ before rendering. Text measurement queries scale font queries up by $S$ and scale the returned physical size down by $1/S$ for layout engine logic.

### 1.4 Input Rescaling

When mouse, touch, or click pointers enter `InputManager`, they are received in physical coordinates. The `InputManager` scales coordinates down to logical values:
$$x_{logical} = \frac{x_{phys}}{S}, \quad y_{logical} = \frac{y_{phys}}{S}$$
This ensures hit-testing against logical boundaries remains pixel-accurate without widget modifications.

### 1.5 Sizing Configuration

* **Programmatic Toggle**: Auto-scaling is enabled by default. You can configure it inside the setup loop:
  ```cpp
  Application app;
  app.set_dpi_scale_enabled(false); // Force default 1.0f scaling
  ```
* **Environment Overrides**: Set the `OOEY_SCALE` environment variable at runtime:
  ```bash
  export OOEY_SCALE=2.0  # Force 2x scaling
  export OOEY_SCALE=off  # Turn off scaling (1.0f)
  ```

---

## 2. Responsive Reflowing Layouts

Aspect ratios differ drastically between widescreen landscape monitors and vertical portrait mobile screens. OOEY provides layout containers that reflow child widgets dynamically as screen constraints change.

### 2.1 The `AdaptiveStack` Layout

`AdaptiveStack` functions as a dual-mode layout container that transitions between a `Row` and a `Column` depending on the available width.

#### Configuration API
* `set_breakpoint(int breakpoint)`: Sets the logical pixel width below which the stack switches to vertical column mode (default: `680`).
* `set_stretch_when_vertical(bool stretch)`: Configures whether child widths are stretched to fill the full container in vertical stacked mode (default: `true`).

#### Layout Mechanics
* **Horizontal Mode (Width > Breakpoint)**:
  * Behaves like a standard horizontal `Row`.
  * Children with `MatchParent` widths divide the remaining width equally.
* **Vertical Mode (Width <= Breakpoint)**:
  * Behaves like a vertical `Column`.
  * Children with `MatchParent` heights divide the remaining height proportionally. This is crucial for scrollable controls (like `DataGrid`), allowing them to stretch to fill empty mobile vertical space rather than clipping.
  * Children with fixed widths or wrap-content policies are stretched to take up the full container width (minus margins) if `stretch_when_vertical` is enabled.

### 2.2 Reflowing Grid Patterns (`FlowLayout`)

`FlowLayout` places children horizontally until they exceed the container width, then wraps them onto the next line.
* This is ideal for groups of options or buttons (like theme selectors).
* **Responsive Pattern**: Designing buttons with a size of, say, 160px allows them to stack vertically on narrow desktop sidebars (where width is 220px), but reflow into a clean 2x2 grid on mobile screens (where width is 360px+).

### 2.3 Viewport Scrolling (`ScrollContainer`)

When a layout's total vertical height exceeds the physical screen boundaries (common in mobile portrait views), wrapping the dashboard layout in a `ScrollContainer` provides vertical scrolling:
* **Dynamic Scrollbars**: Automatically attaches and manages a vertical scrollbar on the right.
* **Direct Touch/Click Drag**: Allows dragging directly on the container background with touch/mouse inputs to slide the dashboard up and down.
* **Offset Coordinate Mapping**: Slices vertical boundaries dynamically by positioning child elements offset by `-scroll_offset_y`. Hit-testing coordinates are mapped transparently through absolute layout coordinates.
* **Implicit Pointer Capture**: When an interactive widget (e.g., a scrollbar thumb) consumes a `Pressed` event, it captures the pointer. All subsequent `Moved` and `Released` events are routed directly to it, preventing loss of drag tracking if the cursor deviates outside the control's bounds.
* **Parent Drag Interception**: To enable fluid mobile scrolling, if a pointer drag gesture exceeds a vertical threshold (8 logical pixels) starting on a child component (like a button, text card, or grid), the ancestor `ScrollContainer` intercepts the touch sequence. The controller cancels the child's touch state (sending a cancel/release event) and transfers pointer capture to the `ScrollContainer` to slide the viewport.
* **Unconstrained Height Sizing Fallback**: Inside a `ScrollContainer`, height constraints are unconstrained (exceeding 50,000 logical pixels). The `View::resolve_height` function detects this and automatically treats `MatchParent` height policies as wrap-content, allowing stacked panels and grids to package to their natural content heights rather than stretching infinitely.

---

## 3. Implementation Example: Responsive System Dashboard

The `sysinfo` dashboard incorporates these controls to layout system cards, process grids, and theme button cards seamlessly across desktop and mobile form factors.

```cpp
// 1. Create the Metrics Row (CPU, RAM, Disk Cards)
auto metrics_row = std::make_shared<AdaptiveStack>();
metrics_row->set_breakpoint(740);
metrics_row->set_width(SizePolicy::MatchParent);
metrics_row->set_height(SizePolicy::WrapContent);

// Add cards with fixed size policies. 
// On desktop, they layout side-by-side (3 * 235px).
// On mobile, they reflow vertically, stretching to fill 100% width.
metrics_row->add_child(cpu_card);
metrics_row->add_child(ram_card);
metrics_row->add_child(disk_card);

// 2. Create the Bottom Section (Process Grid and Theme Card)
auto bottom_row = std::make_shared<AdaptiveStack>();
bottom_row->set_breakpoint(740);
bottom_row->set_width(SizePolicy::MatchParent);
bottom_row->set_height(SizePolicy::MatchParent);

// Process grid takes all remaining space
auto grid_container = std::make_shared<Column>();
grid_container->set_width(SizePolicy::MatchParent);
grid_container->set_height(SizePolicy::MatchParent);

// Theme card has a fixed width of 220px on desktop
auto theme_card = std::make_shared<StyledPanel>();
theme_card->set_width(SizePolicy::Fixed, 220.0f);
theme_card->set_height(SizePolicy::WrapContent);
theme_card->set_align_self(Align::Stretch); // Match grid height on desktop

// Wrap theme buttons in a FlowLayout.
// On desktop (sidebar), they stack vertically.
// On mobile (portrait column), they reflow into a 2x2 layout.
auto button_flow = std::make_shared<FlowLayout>();
button_flow->set_width(SizePolicy::MatchParent);
button_flow->set_height(SizePolicy::WrapContent);

auto btn_dark = std::make_shared<Button>(Rect{0, 0, 160, 34}, Color{45, 45, 52});
btn_dark->set_margin(0, 0, 8, 8); // Handles gaps in flow reflows
button_flow->add_child(btn_dark);
// (add remaining buttons...)

theme_card->add_child(button_flow);
bottom_row->add_child(grid_container);
bottom_row->add_child(theme_card);
```
