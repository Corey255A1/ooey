# Reactive Two-Pass Layout Engine Implementation

## 1. Overview
The reactive layout engine introduces automatic, parent-child constraint-based sizing and positioning to the OOEY/Gooey UI framework. Instead of placing elements at hardcoded specific coordinates, widgets can now specify size policies (`Fixed`, `WrapContent`, `MatchParent`), alignments (`Align::Start`, `Align::Center`, `Align::End`, `Align::Stretch`), padding, and margins. Containers dynamically flow and resize elements in response to window scaling events.

---

## 2. Architecture & The Two-Pass System
The layout system resides directly within `gooey::View` and relies on a recursive two-pass traversal:

### Pass 1: Measure
*   **Method:** `virtual Size measure(Size constraints)`
*   **Purpose:** Determines the desired dimensions of a view (and its descendants) given constraint bounds.
*   **Widget Implementation:** 
    *   `Label` measures text length and line heights using the active bitmap font.
    *   `Button` and `TextBox` calculate heights and widths based on content size, margins, and padding.
    *   Containers (`Column`, `Row`, `Grid`) accumulate the measured sizes of their children.

### Pass 2: Layout / Arrange
*   **Method:** `virtual void layout(Rect bounds)`
*   **Purpose:** Positions and sizes each child within the computed coordinate bounds.
*   **Positioning Logic:** Coordinates are resolved in parent-local space and stored in the absolute `layout_bounds` of each `View`, which is used during mouse/pointer hit testing and drawing passes.

---

## 3. Layout Containers
Three layout containers were implemented under `gooey::controls`:

### Column (`gooey::Column`)
Organizes children in a vertical stack.
*   **Measure:** Aggregates child heights plus vertical margins/padding; calculates width as the maximum child width plus horizontal margins/padding.
*   **Layout:** Positions children sequentially along the Y-axis. Supports horizontal stretching (`Align::Stretch`).

### Row (`gooey::Row`)
Organizes children in a horizontal row.
*   **Measure:** Aggregates child widths plus horizontal margins/padding; calculates height as the maximum child height plus vertical margins/padding.
*   **Layout:** Positions children sequentially along the X-axis. Supports vertical stretching (`Align::Stretch`).

### Grid (`gooey::Grid`)
Arranges children in a configured row-by-column grid layout.
*   **Measure & Layout:** Computes cell widths and heights dynamically from the total available layout boundaries. Places children based on their grid cell indices.

---

## 4. Main Loop Integration
The layout pass is triggered automatically inside `gooey::Application::run_iteration()` immediately before clearing and drawing the scene graph. 

```cpp
if (root_view_) {
    Size size = window_backend_->get_size();
    if (auto chrome = window_backend_->get_window_chrome()) {
        size.width -= 2 * chrome->get_border_width();
        size.height -= (2 * chrome->get_border_width() + chrome->get_title_bar_height());
    }
    root_view_->measure(size);
    root_view_->layout(Rect{0, 0, size.width, size.height});
    root_view_->draw(*target);
}
```

This guarantees that whenever the window changes size (e.g. Wayland configuration event, X11 configure event), the entire UI immediately responds to the new dimensions in the next frame.

---

## 5. Verification & Demos
*   **Unit Tests:** Configured in `tests/test_layout.cpp` validating constraint calculations for Column, Row, and Grid layouts.
*   **Examples:** Created `examples/hello_layout.cpp` displaying:
    *   A main Column matching the window.
    *   A Row with actions (Submit/Cancel Buttons).
    *   A Grid displaying dynamic telemetry analytics widgets.
    *   Responsive text boxes.
