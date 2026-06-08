# Responsive Menu and MenuBar System

This document describes the design, implementation, and integration of the responsive Menu and MenuBar system in the `gooey` library.

---

## 1. Architectural Overview

The menu system consists of two primary controls:
*   `MenuBar`: A container control representing the top-level horizontal bar containing menu headers/categories (e.g., File, Edit, Help).
*   `Menu`: A pop-up menu control representing dropdowns, submenus, and checkable lists.

```mermaid
graph TD
    MenuBar -->|creates on selection| Menu
    Menu -->|creates on hover| SubMenu[Menu (Submenu)]
    Menu -->|cycles sibling headers| MenuBar
```

### Z-Order Rendering & Root Mounting
In most UI layout engines, layout components draw their children sequentially. If a dropdown menu were rendered strictly as a child of the `MenuBar` inside the layout flow, any subsequent siblings of the `MenuBar` (such as main content panels) would render on top of it, creating Z-order overlap issues.

To solve this, when a menu category is selected:
1.  The `MenuBar` traverses the tree up to the window's root container (`find_root_node`).
2.  The `Menu` dropdown is added as a child of the root container.
3.  Since absolute positioning (`set_absolute(true)`) is enabled, the menu is laid out relative to the root node's padding.
4.  Because it is the last child of the root node, it is rendered last, guaranteeing that it appears on top of all other controls.

---

## 2. Responsive Hamburger Collapse Model

The menu system dynamically transitions between a standard horizontal desktop layout and a vertical mobile hamburger navigation menu.

```
Width > Breakpoint (e.g., 600px):
+------------------------------------------+
| File   Edit   Theme   Help               |
+------------------------------------------+

Width <= Breakpoint:
+--------------------------------------+---+
|                                      | ☰ |  <-- Hamburger Button
+--------------------------------------+---+
| File                                     |  <-- Expanded Categories
| Edit                                     |
| Theme                                    |
| Help                                     |
+------------------------------------------+
```

### Sizing and Measure Flow
*   **Horizontal Mode**: The `MenuBar` measures with a fixed height of `40px` and places categories horizontally.
*   **Hamburger Closed**: Height measures to `40px`, rendering only the top bar with a premium custom-drawn three-line hamburger icon button at the right.
*   **Hamburger Open**: Height measures to `40px + 40px * N` (where `N` is the number of categories). This pushes any elements below it down in the layout flow, adapting to vertical layouts.

---

## 3. Keyboard Navigation and Event Routing

To provide an accessible and premium experience, the menu system supports complete keyboard controls:

| Key | Context | Action |
| :--- | :--- | :--- |
| **Up / Down** | Active dropdown menu | Moves the hovered item focus up or down |
| **Enter** | Active dropdown menu | Executes the action of the hovered item / toggles checkbox |
| **Escape** | Active menu or menubar | Dismisses any open menu and returns focus to the bar |
| **Left / Right** | Submenu | Closes the submenu and returns focus to parent |
| **Left / Right** | Main dropdown menu | Cycles categories (e.g., pressing Right inside the *File* menu automatically closes it and opens *Edit*) |

---

## 4. MVVM Integration

The menu system is designed to fit the MVVMC (Model-View-ViewModel-Controller) paradigm. Menu item categories and their properties (such as checkmarks or label text) are defined as models, bound to properties in the `ViewModel`, and observed by the `View`.

### Example Bindings
```cpp
// Update menu items/checkmarks dynamically when the active theme cycles
view_model_->active_theme_prop.subscribe([this](const std::string&) {
    menu_bar_->set_categories(build_menu_categories());
});
```

---

## 5. Styling & Themes

The menu system automatically adapts to active themes and styles:
*   **MenuBar Styling**: Configurable background fill color, border line separator color, and text colors.
*   **Dropdown / Submenu Styling**: Distinct panel background with elegant borders, highlighted hover states, and clear checkbox borders.
*   **Modern Aesthetics**: Adapts to dark mode, light mode, and high-contrast styling by overriding the `apply_style` lifecycle method.

---

## 6. Memory Safety and Deferral (AddressSanitizer Resolution)

During testing under AddressSanitizer (`-fsanitize=address`), a critical heap-use-after-free crash was discovered and resolved.

### Detailed Investigation and Debugging Steps

#### The Symptom
When clicking certain menu options or when focus shifted outside the menu hierarchy, the application crashed with an AddressSanitizer traceback:
```
SUMMARY: AddressSanitizer: heap-use-after-free in gooey::controls::MenuBar::open_menu(int)::{lambda()#1}::operator()() const
```

#### The Investigation
By capturing and analyzing the stack trace under ASan, we traced the crash back to focus validation inside the `Menu::draw()` rendering loop.

1. **Inline Scene Graph Mutation**:
   Inside `Menu::draw()`, the menu checks if focus is lost and calls `close()` immediately:
   ```cpp
   if (!has_focus && controller->get_focused_element() != nullptr) {
       const_cast<Menu*>(this)->close(); // <-- Inline closure
       return;
   }
   ```
2. **Collection Invalidation**:
   `Menu::close()` calls `parent_node->remove_child(shared_from_this())`, which calls `children_.erase(it)` on the parent `GooeyNode`. However, this occurs while `GooeyNode::draw` is currently iterating over the `children_` vector:
   ```cpp
   for (const auto& child : children_) {
       child->draw(target); // <-- Calling Menu::draw()
   }
   ```
   Mutating the `children_` vector during this iteration invalidates the loop iterators, leading to undefined behavior and memory access errors.

3. **Lifetime Collapse (Heap Use-After-Free)**:
   Inside `Menu::close()`, the `on_close` callback is executed. For the active dropdown menu, this callback clears `MenuBar::active_menu_`:
   ```cpp
   active_menu_->on_close = [this]() {
       active_menu_ = nullptr; // <-- Drop the strong shared_ptr reference
   };
   ```
   Because `Menu::draw` is a `const` member function, it did not have a strong `std::shared_ptr<Menu>` reference on its local call stack. When `active_menu_` was cleared, the reference count of the `Menu` dropped to zero, immediately deallocating (freeing) the `Menu` object.
   When the control flow returned from `on_close()` back to `Menu::close()` and `Menu::draw()`, the subsequent accesses to `this` members or the execution of `shared_from_this()` evaluated against a freed memory address, triggering the heap-use-after-free.

---

### Resolution and Reasoning

To prevent both loop iterator invalidation and lifetime destruction mid-stack, two memory safety improvements were implemented:

1. **Deferred Scene Graph Modification (Event Loop Dispatch)**:
   Instead of modifying the scene graph immediately inside `draw()`, we defer the call to `close()` by capturing a strong `shared_from_this()` copy and queuing it using `Application::dispatch`:
   ```cpp
   if (!has_focus && controller->get_focused_element() != nullptr) {
       auto self = const_cast<Menu*>(this)->shared_from_this();
       gooey::Application::get_instance()->dispatch([self]() {
           self->close();
       });
       return;
   }
   ```
   * **Reasoning**: This allows the current draw loop and rendering phase to complete without modifying the active collections. The menu unparenting and cleanup execute safely at the beginning of the next event loop iteration.

2. **Defensive Self-Reference Keep-Alive**:
   We added a local keep-alive shared pointer inside `Menu::close()` to prevent the object from being freed while its methods are still executing on the stack:
   ```cpp
   void Menu::close() {
       if (!is_open_) return;
       auto self = shared_from_this(); // Keep instance alive during callbacks
       is_open_ = false;
       ...
   }
   ```
   * **Reasoning**: Even if callbacks such as `on_close` or parent modifications clear all external shared references to this menu, `self` keeps the object memory valid until `close()` completes its stack execution and safely unwinds.

---

## 7. Click / Spawning Menu Visibility Fix

### Detailed Investigation and Debugging Steps

#### The Symptom
When the user clicked a menu category header (like "File" or "View") on the `MenuBar`, the dropdown menu did not appear, or disappeared instantly, rendering the menu options invisible.

#### The Investigation
By tracing the controller's event loop and the menu draw cycle, the issue was identified as a focus conflict:
1. **Spawning & Initial Focus**:
   When a category header is clicked, `MenuBar::on_pointer_event` calls `open_menu(idx)`. Inside `open_menu(idx)`, it calls `controller->set_focused_element(active_menu_)` to focus the newly spawned menu dropdown.
2. **Focus Overwrite in Event Routing**:
   In `Controller::route_pointer_event`, the pointer event is routed to the `MenuBar` container first:
   ```cpp
   if (interactive->on_pointer_event(pointer)) {
       if (pointer.state == PointerState::Pressed) {
           set_focused_element(node); // <-- node is MenuBar!
   ```
   Because `on_pointer_event` returns `true`, the `Controller` overwrites the focused element with the `MenuBar` itself *after* the `MenuBar` had already set focus to the `Menu`.
3. **Instant Dismissal in Focus Validation**:
   During the next rendering pass, `Menu::draw` executes focus validation:
   ```cpp
   auto focused = controller->get_focused_element(); // focused is MenuBar
   // ... loops submenu and parent menu chains ...
   if (!has_focus && controller->get_focused_element() != nullptr) {
       // Dismiss menu if focus shifted completely outside the menu hierarchy
       ...
   }
   ```
   Since the `MenuBar` is not in the menu or submenu chain of the `Menu`, `has_focus` remained `false`. And since the focused element (`MenuBar`) was not null, it dispatched the menu `close()` task immediately, closing the menu before a single frame could be displayed to the user.

### Resolution and Reasoning
We resolved the focus conflict by recognizing that if the `MenuBar` that spawned the menu hierarchy has focus, the menu itself should remain open.

* **MenuBar Focus Exception**: We added a type check in `Menu::draw` to treat `MenuBar` focus as valid:
  ```cpp
  // Also allow focus on the parent MenuBar itself
  if (!has_focus && focused) {
      if (dynamic_cast<gooey::controls::MenuBar*>(focused.get())) {
          has_focus = true;
      }
  }
  ```
* **Reasoning**: When a user clicks a menu header, the focus naturally rests on the `MenuBar` container. Treating it as holding valid focus ensures the menu remains open. If the user clicks elsewhere (like on a button or preview canvas), focus shifts to that interactive control, which correctly triggers the focus validation check and dismisses the menu.

---

## 8. Column and Row Absolute Layout Position Fix

### Detailed Investigation and Debugging Steps

#### The Symptom
Even with the focus validation fix, when clicking a menu option, the menu dropdown was still not visible on the screen. The user questioned if it could be a render order (Z-order) issue.

#### The Investigation
By tracing the coordinates of the spawned menu container in `ooey-gooey-editor`:
1. **Root View Layout**: The editor's root node (`windowRoot`) is defined as a `VBox` (which maps to the `Column` layout class).
2. **Sequential Layout Flow**: In the `Column::do_layout()` and `Column::do_measure()` implementations, the layout loops through all children and places them sequentially along the Y-axis.
3. **Absolute Positioning Ignored**: Although the spawned `Menu` has `is_absolute = true` and `MenuBar::open_menu` sets `active_menu_->set_absolute_bounds(...)`, the `Column` container completely ignored the `is_absolute` flag.
4. **Layout Offset Off-screen**: As a result, the `Column` treated the `Menu` as a normal sequential child and placed it at `cy = bounds.y + padding_top + MenuBar_height + editorRoot_height = 0 + 0 + 40 + 728 = 768`. Since the window height is exactly 768 pixels, the menu was placed entirely off-screen at the bottom of the window, rendering it completely invisible. A similar bug existed in the `Row` layout container.

### Resolution and Reasoning
We resolved the absolute positioning layout bug by bringing `Column` and `Row` layout behaviors in line with the default `GooeyNode` layout behavior.

* **Absolute Layout Handling**: We updated `Column::do_measure`/`Column::do_layout` in `column.cpp` and `Row::do_measure`/`Row::do_layout` in `row.cpp` to check if `child->is_absolute` is true.
* **Bypass Sequence Flow**: When a child is absolute, it is bypassed in the sequential flow layout calculations (so it does not consume spacing or change the sequential Y/X offsets), but it is still measured and laid out directly at its relative `absolute_bounds` offset:
  ```cpp
  if (child_view->is_absolute) {
      int cx = bounds.x + padding_left + child_view->absolute_bounds.x;
      int cy = bounds.y + padding_top + child_view->absolute_bounds.y;
      child_view->layout(Rect{cx, cy, child_view->absolute_bounds.width, child_view->absolute_bounds.height});
      continue;
  }
  ```
* **Reasoning**: This guarantees that dynamically mounted root-level overlays (such as dropdowns, context menus, and tooltips) can be placed at their absolute pixel coordinates regardless of whether the root node container is a sequential flow layout (`Column` or `Row`) or a free-form container (`GooeyNode`).

