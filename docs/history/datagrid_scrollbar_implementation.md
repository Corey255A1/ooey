# ScrollBar & DataGrid View Implementation History

Date: 2026-05-31

This document outlines the design, layout mathematics, event state machine, style engine integration, and MVVM-C pattern alignment for the custom `ScrollBar` and virtualized `DataGrid` widgets, along with the performance-driven updates to the `hello_sysinfo` dashboard.

---

## 1. Composable ScrollBar Design

The `ScrollBar` is built strictly following the principle of composition over inheritance. Rather than invoking custom canvas lines/pixels, it manages:
- **Track Primitive**: A `RectPrimitive` representing the scrollable groove/track.
- **Thumb Primitive**: A `RoundedRectPrimitive` representing the draggable handle.

### Layout & Value-to-Pixel Mapping
The scrollbar works in a coordinate space bounded by:
- `min_val` (minimum scroll offset, e.g., 0)
- `max_val` (total scrolling extent, e.g., total items or pixels)
- `page_size` (size of the viewport in items/pixels)

The value `value_` is clamped to:
$$\text{clamped\_value} \in [\text{min\_val}, \text{max\_val} - \text{page\_size}]$$

The thumb size is proportional to the visible page ratio of the total scroll range:
$$\text{thumb\_length} = \max\left(12, \frac{\text{bounds.length} \times \text{page\_size}}{\text{max\_val} - \text{min\_val}}\right)$$

The position of the thumb is mapped linearly to the scrollable track space:
$$\text{scrollable\_track} = \text{bounds.length} - \text{thumb\_length}$$
$$\text{ratio} = \frac{\text{value} - \text{min\_val}}{(\text{max\_val} - \text{page\_size}) - \text{min\_val}}$$
$$\text{thumb\_pos} = \text{bounds.start} + \text{ratio} \times \text{scrollable\_track}$$

### Drag State Machine & Track Click Routing
The scrollbar inherits from `IInteractive` and processes `Pointer` events:
1. **Thumb Dragging**:
   - On `PointerState::Pressed` inside `thumb_bounds_`, the drag flag `dragging_thumb_` becomes active, caching `drag_start_offset_` (distance from mouse to thumb edge).
   - On `PointerState::Moved` while dragging, the mouse location offsets are mapped back to value units and clamped, invoking `on_value_changed`.
   - On `PointerState::Released`, the dragging state resets.
2. **Track Page Jump**:
   - Clicking on the track outside the thumb automatically centers the thumb on the click position, recalculating the target scroll value and applying it smoothly.

---

## 2. High-Performance Virtualized DataGrid

To handle thousands of records without degrading frame rate or consuming excessive heap memory, `DataGrid` performs horizontal and vertical viewport virtualization:

### Layout Mathematics & Recycler Pools
- **Visible Row Bounds**: Calculates visible row slots using:
  $$\text{visible\_rows\_count} = \frac{\text{bounds.height} - \text{header\_height} - (\text{need\_h\_scroll} ? 12 : 0)}{\text{row\_height}}$$
- **Visual Primitives Allocation**: Pre-allocates and caches exactly `visible_rows_count` text labels (`cell_texts_`) and background rows (`cell_bgs_`).
- **Scroll Recycler**: When vertical scrolling shifts `scroll_offset_y_`, no new layout nodes or visual primitives are added or destroyed. The grid simply updates the text strings inside the existing cached primitives (`update_cell_values()`), preserving layout tree stability and caching advantages.

### Viewport Clipping
Columns are dynamically culled if their bounding box falls outside the grid's horizontal bounds, skipping text primitive layout and rendering calculations completely.

---

## 3. Style Engine & Theming Customization

Both widgets integrate cleanly into the MVVM-C stylesheet styling model:
- **Default Style Names**:
  - `ScrollBar` calls `set_style_name("scrollbar")` in its constructor.
  - `DataGrid` calls `set_style_name("datagrid")` in its constructor.
- **Style Mapping**:
  - `ScrollBar`: Uses the stylesheet's `fill_color` for the track and `stroke_color` for the draggable thumb handle.
  - `DataGrid`: Uses the stylesheet's `fill_color` for the container background, `text_color` for contents, and `stroke_color` for borders. Zebra-striping colors are calculated dynamically based on offsets from the base background.

### Custom Themes Configuration
The `hello_sysinfo` application registers customized properties for both scrollbars and grids across its themes:
- **Dark Mode**: Charcoal-gray tracks (`Color{25, 25, 30}`) with elevated gray thumbs (`Color{70, 70, 80}`).
- **Light Clean**: Light gray tracks (`Color{240, 240, 245}`) with mid-tone thumbs (`Color{180, 180, 185}`).
- **Hacker Green**: Black tracks (`Color{0,0,0}`) with vivid green indicators (`Color{0, 255, 0}`).
- **Soft Lofi**: Muted pinkish-brown tracks (`Color{235, 224, 217}`) and warm clay thumbs (`Color{160, 140, 132}`).

---

## 4. Sysinfo Example Application Upgrades

The `hello_sysinfo` dashboard has been refactored to show the power of these components:
- **DataGrid Integration**: Replaced the old process list control with a virtualized `DataGrid` displaying: `PID`, `Process Name`, `CPU %`, `Memory`, and `State`.
- **CPU % Computation**:
  - Linux: Reads `/proc/[pid]/stat` to parse system ticks (`utime + stime`), calculating the delta over elapsed frame time ($dt$) divided by the clock speed (via `sysconf(_SC_CLK_TCK)`).
  - Windows: Resolves ticks using `GetProcessTimes` API, dividing the delta by $dt$ to yield accurate per-process CPU usage.
- **Dynamic Key Garbage Collection**: Evicts terminated process IDs from the tick delta cache automatically.
- **Efficiency Gains**: Reduced process list crawling latency by switching from multi-file `/proc/[pid]/status` parses to a single fast parse of the space-separated fields of `/proc/[pid]/stat`.

---

## 5. Grid Divider Styling & Drawing Order Corrections

### The Overlap Issue
In the virtualized `DataGrid`, cell background boxes were previously added to the view hierarchy after the vertical column divider lines. Because children are drawn in insertion order, the cell backgrounds drew over and completely obscured the column lines.

### The Resolution
1. **Z-Order Reconstruction:** Refactored the insertion order in `DataGrid::update_layout_elements()`. The container backgrounds are added first, then the cell backgrounds, followed by the grid divider lines (placed on top of the cell backgrounds), and finally text labels and scrollbars. This guarantees that divider lines remain visible and are never overlapped by cell information or row backgrounds.
2. **Separation Line Styling:** Added direct styling parameters to `DataGrid` to control the visual properties of separation lines:
   * **Toggles:** `show_column_lines` and `show_row_lines` allow turning dividers on/off.
   * **Thickness & Color:** Thickness and color of column and row lines are fully customizable.
   * **Styles (Stippling):** Added support for different line patterns (`LineStyle::Solid`, `LineStyle::Dashed`, and `LineStyle::Dotted`).
3. **Procedural Geometry Generation:** Extended the `LinePrimitive` geometry generation pass (`rebuild_geometry`) to support the new line styles. If dashed or dotted styles are set, the primitive segments the path procedurally into alternating dashes/gaps (using individual line segments for thin lines or individual rectangle quads for thick lines). This enables clean stippled lines across legacy OpenGL, software, and Vulkan backends under a single draw command.
4. **Theme Binding:** Integrated separation line styling with the style system inside `DataGrid::apply_style()`, defaulting divider colors and thicknesses to the stylesheet's `stroke_color` and `stroke_thickness` when defined.

