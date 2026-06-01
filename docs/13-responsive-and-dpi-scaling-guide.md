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
bottom_row->set_height(SizePolicy::WrapContent); // Prevents swallowing vertical space from sibling widgets

// Process grid container
auto grid_container = std::make_shared<Column>();
grid_container->set_width(SizePolicy::MatchParent);
grid_container->set_height(SizePolicy::WrapContent); // Set to WrapContent to resolve vertical collisions

auto proc_grid = std::make_shared<DataGrid>(Rect{0, 0, 510, 240}, 26, Font{"monospace", 12});
proc_grid->set_absolute(false);
proc_grid->set_width(SizePolicy::MatchParent);
proc_grid->set_height(SizePolicy::WrapContent); // WrapContent height enforces absolute row constraints in layout calculations
grid_container->add_child(proc_grid);

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

// Top level card containing the main content
auto main_card = std::make_shared<StyledPanel>();
main_card->set_width(SizePolicy::MatchParent);
main_card->set_height(SizePolicy::MatchParent); // Fills available viewport vertical area
main_card->add_child(metrics_row);
main_card->add_child(bottom_row);

// Footnote description at the bottom
auto footnote = std::make_shared<Label>(
    "Note: Processes grid view is fully virtualized and refreshed once per second. Responsive DataGrid layout enabled.",
    Font{"sans-serif", 11},
    Point{0, 0},
    Color{110, 110, 120}
);
footnote->set_absolute(false);
footnote->set_width(SizePolicy::MatchParent); // Stretch horizontally
footnote->set_overflow(TextOverflow::Wrapped); // Wraps text onto multiple lines
main_card->add_child(footnote);
```

---

## 4. Visual Clipping Stack

To prevent views and child components from rendering outside their visual parents (such as inside a `ScrollContainer` viewport or inside custom layout boundaries), OOEY incorporates a recursive, stack-based clipping system.

### 4.1 Sizing Philosophy

Views can enable clipping by setting `set_clip_children(true)`. By default, this is disabled for standard layout views to optimize render-cycle throughput, but is automatically activated for viewport containers like `ScrollContainer`.

*   `virtual void push_clip(const Rect& rect) = 0;`: Computes the intersection of `rect` with the current clipping rectangle at the top of the stack and pushes it as the active clipping rectangle.
*   `virtual void pop_clip() = 0;`: Pops the current clipping bounds from the stack, restoring the previous clip.

### 4.2 Renderer Implementations

Each rendering backend implements scissor testing optimized for its hardware pipeline:
*   **Vulkan (`VulkanRenderTarget`)**: Because drawing is buffered as `DrawCall` objects, scissor boundaries are attached to each `DrawCall`. During rendering pass recording, `vkCmdSetScissor` is called dynamically per draw call.
*   **OpenGL (`GlRenderTarget`)**: Manipulates `glScissor` coordinates. Since OpenGL systems use a bottom-left origin, top-left logical coordinates are translated:
    $$y_{gl} = H_{target} - (y + H_{rect})$$
    Scissoring is temporarily suspended during buffer clearing so the entire screen is reset.
*   **Software Renderer (`SoftwareRenderTarget`)**: Clamps rasterization bounds in pixel-drawing loops. Lines, triangles, images, filled rectangles, and glyph arrays are cropped to the active scissor rectangle before memory buffer writes.
*   **Decorators (`ScaledRenderTarget` & `ChromeRenderTarget`)**: `ScaledRenderTarget` scales the clip rectangle up by the DPI scaling factor $S$. `ChromeRenderTarget` translates the clip rectangle by window border and title bar offsets $(dx, dy)$ before delegating.

---

## 5. Text Overflow Policies

When labels and text fields contain text exceeding the visual boundaries of their container, developers can customize the handling policy via the `TextOverflow` enum property on the `Label` control:

```cpp
enum class TextOverflow {
    None,     // Allows text to overflow bounds normally
    Clipped,  // Clips text drawing bounds using the target's clipping stack
    Shrunk,   // Scales down the font size proportionally to fit the bounds
    Wrapped   // Wraps the text by space tokens into multiple lines
};
```

### 5.1 API Configuration

*   `set_overflow(TextOverflow policy)`: Chains the policy directly to the `Label` control.
*   `set_overflow_policy(TextOverflow policy)`: Explicitly sets the overflow mode on the control.

### 5.2 Layout and Drawing Resolution

*   **Wrapped Sizing (`do_measure`)**: When `TextOverflow::Wrapped` is selected, `Label::do_measure` computes text lines by checking character widths using target-aware measurement metrics. The layout engine returns the height scaled by the line-count:
    $$H_{measured} = \text{line\_count} \times H_{line}$$
*   **Shrunk Sizing**: Measures the text at the default font size. During draw cycles, if the text's bounding width/height exceeds the allocated container layout bounds, the rendering scale factor is calculated:
    $$scale = \min\left(1.0, \frac{W_{avail}}{W_{text}}, \frac{H_{avail}}{H_{text}}\right)$$
    The font size is scaled down dynamically:
    $$\text{size}_{font} = \max(1, \text{size}_{default} \times scale)$$

---

## 6. Layout Resolution Pitfalls & Rendering Bug Fixes

Building responsive, multi-device layouts introduces specific UI rendering bugs when views size-collapse or compete for space in layout stacks. This section documents specific resolution bugs and their fixes:

### 6.1 Collapsed Scrollbar Artifacts (Fixed)

**The Bug:** In both `ScrollContainer` and `DataGrid`, when scrollbars are not required, they are laid out with collapsed bounds `Rect{0, 0, 0, 0}`. However, because `ScrollBar` clamps its slider handle to a minimum thickness (12px) for user accessibility, calling `update_thumb_bounds()` on a collapsed scrollbar would compute internal bounds such as `Rect{2, 0, 4, 12}`. Under default `View::draw` implementation, these track and thumb primitives were rendered in the top-left corner of the screen `(0, 0)`.

**The Fix:** Overrode the `draw(ooey::IRenderTarget& target) const` method in the `ScrollBar` control:
```cpp
void ScrollBar::draw(ooey::IRenderTarget& target) const {
    if (bounds_.width <= 0 || bounds_.height <= 0) {
        return; // Suppress rendering for collapsed bounds
    }
    View::draw(target);
}
```
This halts drawing recursion for any scrollbar widget that has been collapsed by its layout manager, preventing invalid rendering calls entirely.

### 6.2 Vertical Spacing Collisions & Missing Sibling Widgets (Fixed)

**The Bug:** Stacking layouts with height constraints (like the main vertical `Column` in a dashboard) process height allocations sequentially:
1. When a child container in a vertical `Column` is configured with `MatchParent` height (such as the original `bottom_row` adaptive stack), the column allocator assigns **all remaining available vertical layout height** to that container.
2. When the layout engine progresses to subsequent sibling widgets (such as the bottom `footnote` label), the available layout height `avail_h` is already reduced to `0`. Consequently, those siblings measure and layout with height `0` and vanish from the screen.

**The Fix:**
* Enforce `WrapContent` height on all intermediate vertical layout nodes (`bottom_row`, `grid_container`, and the virtualized `proc_grid`) so they stack snugly based on their content boundaries.
* Change the parent dashboard view (`SystemMonitorView`) height from `WrapContent` to `MatchParent` (in [view.cpp](file:///home/corey/code/ooey/examples/sysinfo/view.cpp#L18)). This ensures the view stretches to fill the physical screen viewport when room permits, but naturally falls back to scrolling heights in smaller screen ratios under `ScrollContainer` unconstrained height measurement rules.

### 6.3 Responsive Text Wrapping

**The Design Pattern:** When displaying text elements under constrained layouts (such as on portrait phone displays), default labels with `WrapContent` width do not wrap and get clipped horizontally. Configuring header titles, subtitles, and footer text with:
```cpp
label->set_width(SizePolicy::MatchParent);
label->set_overflow(TextOverflow::Wrapped);
```
tells the text engine to split text lines by spaces relative to parent container width constraints, dynamically increasing container height and reflowing sentences.

