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

## Summary
The current architecture of OOEY represents a modern, C++20 reactive UI framework. By starting with a solid abstraction layer, adopting a retained mode scene graph, structuring the codebase for modularity, and layering a decoupled MVVM-C reactive system on top, OOEY provides a robust, explicit, and scalable foundation for cross-platform UI development.