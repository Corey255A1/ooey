# ListControl Implementation History

Date: 2026-05-27

This document details the implementation of the `ListControl` component, its composition-based design, scrolling logic, dynamic layout calculation, and interactive event routing.

## 1. Design and Architecture: Composable Retained Mode

Following the project standard of composition over inheritance:
- **Retained Composition**: Instead of drawing list items procedurally using ad-hoc canvas calls, the `ListControl` is a subclass of `View` that constructs a sub-tree of child primitives. It embeds one `RoundedRectPrimitive` for the outer frame, and a list of `RectPrimitive` backgrounds and `TextPrimitive` labels for the item slots.
- **Widget Recycling (Virtualization)**: Rather than dynamically instantiating or deleting primitives when the user scrolls, the control allocates a fixed pool of slot primitives corresponding to the number of visible items. When scrolling occurs, the text and background highlights of these existing slots are recycled/updated.

## 2. Dynamic Layout Calculation

### Challenge
- The initial layout design assumed exactly 5 visible items. However, lists require flexible bounds and varied spacing. Hardcoding the visible count makes it impossible to reuse the control for different heights or text sizes.

### Resolution
- The constructor accepts an `item_height` parameter.
- The number of visible slots is calculated dynamically:
  $$\text{visible\_count\_} = \max\left(1, \frac{\text{bounds.height}}{\text{item\_height}}\right)$$
- The pool of background (`item_bgs_`) and text (`item_texts_`) primitives is sized dynamically to match `visible_count_`.
- Loops for rendering, click tracking, keyboard navigation, and scroll bounds shifting utilize `visible_count_` rather than hardcoded bounds.

## 3. Scrolling and Selection Boundaries

The list maintains `scroll_offset_` (first visible element index) and `selected_index_` (currently highlighted item index):
- **Upward Scroll Shift**: If `selected_index_ < scroll_offset_`, the offset shifts up to match the selection:
  $$\text{scroll\_offset\_} = \text{selected\_index\_}$$
- **Downward Scroll Shift**: If the selection moves beyond the visible range ($\text{selected\_index\_} \ge \text{scroll\_offset\_} + \text{visible\_count\_}$), the offset shifts down to keep the selection at the bottom slot of the visible window:
  $$\text{scroll\_offset\_} = \text{selected\_index\_} - (\text{visible\_count\_} - 1)$$

## 4. Interactive Handling: Clicks and Keyboard

The control inherits from `IInteractive` to support user input:
- **Pointer (Click) Selection**:
  - In `on_pointer_event()`, if a `Pressed` event is inside the control's bounds, the clicked item slot is mapped to the active index:
    $$\text{clicked\_visible\_index} = \frac{e.y - \text{bounds.y}}{\text{item\_height}}$$
  - The final index is resolved as $\text{scroll\_offset\_} + \text{clicked\_visible\_index}$ and selected.
- **Keyboard Navigation**:
  - If the control has focus, `on_key_event()` intercepts Up and Down arrow keys (standard X11/xkbcommon keysyms `0xFF52` and `0xFF54`).
  - Pressing Up calls `select_previous()`, and pressing Down calls `select_next()`, shifting highlight and scrolling the list control accordingly.
