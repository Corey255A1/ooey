# MVVMC Page Navigation Implementation

Date: 2026-05-27

This document outlines the design and implementation of the MVVMC page navigation framework in the `ooey` library, demonstrating how backward/forward states, page transitions, and sub-page branching are managed natively while keeping the Model-View-ViewModel-Coordinator architecture decoupled and clean.

## 1. Architectural Strategy: ViewModel-First Navigation Stack

In complex GUI applications, routing and navigation should remain decoupled from platform rendering views. To achieve this in `ooey`:
- **ViewModel-First Stack**: The central state of navigation is represented as a history of ViewModels (`PageViewModelBase` instances) managed by a central coordinator (`NavigationCoordinator`).
- **Coordinator Pattern**: The `NavigationCoordinator` serves as the router/coordinator, exposing properties like `current_viewmodel`, `can_go_back`, and `can_go_forward` for data binding, while encapsulating transition rules.
- **Dynamic View Recreation**: The shell view (`NavigationShellView`) binds to the coordinator's `current_viewmodel` property. Whenever the active ViewModel changes, the shell clears the active sub-view area (`page_container_->clear_children()`) and recreates/allocates the corresponding page view matched to the active ViewModel.

## 2. Navigation Coordinator Details

The `NavigationCoordinator` class (`include/ooey/mvvmc/navigation_coordinator.hpp`) manages linear and branched page flows using a history buffer:
- `history_`: Vector of active `PageViewModelBase` shared pointers.
- `current_index_`: Integer index tracking the active position in history.
- **Push / Linking (`navigate_to`)**: Appends the new ViewModel to history. If the user had navigated backward in history (i.e. `current_index_` was not at the end), all forward history items are discarded (matching browser behavior).
- **Go Back / Go Forward**: Decrements/increments `current_index_` and updates the active `current_viewmodel` state.
- **State Updates**: Synchronizes `can_go_back` and `can_go_forward` boolean properties.

## 3. Interactive Wizard Example Application

The example `hello_wizard` (`examples/hello_wizard.cpp`) implements a multi-step setup flow demonstrating complex navigation semantics:

### Layout Shell (`NavigationShellView`)
- Composes static elements: outer frame card, Title Label, and standard Back/Forward navigation buttons in the top header.
- Back and Forward buttons bind to the coordinator's `can_go_back` and `can_go_forward` properties to change colors and hide labels when inactive.
- Forward button acts as a contextual "Next >" action when at the end of the history stack, advancing to the next logical wizard step.

### Page Flow
1. **Page 1 (Welcome)**: Draws a welcome greeting and a "Start Wizard" button inside the page. Clicking it navigates to Page 2.
2. **Page 2 (Animal Selection)**: Displays a `ListControl` with animals. Clicks update the `selected_animal_index` in the ViewModel. Forward button goes to Page 3.
3. **Page 3 (Clock)**: Displays a fully animated analog/digital clock.
   - Button **"Check this out"** branches forward to Page 4.
   - Button **"Continue"** branches forward to Page 5.
4. **Page 4 (Sinusoid)**: Displays an animated scrolling sinusoid wave. The only option is a "Go Back" button that returns to Page 3 via `coordinator->go_back()`.
5. **Page 5 (End)**: Displays big text "The End" fading smoothly through the colors of the rainbow. Provides an "Exit" button that calls the exit callback, calling `Application::quit()`.

## 4. Time Loops and Color Fading

- **Active ViewModel Tick**: In the main update loop, `before_render_callback` queries `coordinator->current_viewmodel.get()` and calls its virtual `update(dt)` method. This allows whichever page is currently active to run high-resolution animations (like updating clock hands, scrolling sinusoid phase, or computing fading colors) without active updates leaking from inactive pages.
- **Color Fading Formula**: Page 5 cycles colors by computing RGB values from shifted sine waves based on elapsed time:
  $$R = (\sin(t \cdot 2.0) + 1.0) \cdot 127.5$$
  $$G = (\sin(t \cdot 2.0 + \frac{2\pi}{3}) + 1.0) \cdot 127.5$$
  $$B = (\sin(t \cdot 2.0 + \frac{4\pi}{3}) + 1.0) \cdot 127.5$$
