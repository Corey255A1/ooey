# Architectural Design Decisions

This document chronicles the major design decisions and architectural choices made during the development of the OOEY GUI Engine. It is intended to help contributors (and users) understand the "why" behind the code structure.

## 1. C++20 and Standard Library Choices
- **Why C++20?** C++20 introduces concepts, ranges, and smart pointers that heavily reduce boilerplate and make memory management safer. However, we explicitly chose to **avoid C++20 Modules** for now to keep the build system simple and maximize compatibility across different tooling ecosystems (like clangd and CMake).
- **Dependency Management:** We use CMake's `FetchContent` instead of Git Submodules or package managers (like vcpkg/conan) to ensure the project is highly portable and builds out-of-the-box without requiring users to install separate package managers.

## 2. Interface Segregation (The Core Interfaces)
The engine strictly separates the "idea" of a window and renderer from the "implementation".
- **`IWindowBackend`**: Represents the OS-level window. It is completely decoupled from graphics APIs. Its only job is to get a window on the screen, handle the OS event loop, and provide a surface to draw on.
- **`IRenderTarget`**: Represents the canvas. It has no idea if it's drawing to a screen, a block of CPU memory, or a `.png` file.

**Why?** This guarantees that if we want to add a `WindowsBackend` or a `WaylandWindowBackend` later, we don't have to rewrite any core application logic. It also allows us to build "headless" apps (e.g., rendering UI into memory for a server-side image generator) by swapping the implementation.

## 3. The `std::function` Render Callback
Instead of having the user inherit from the `Application` class and override an `on_render()` method, we chose a composition approach:
```cpp
app.set_render_callback([](ooey::IRenderTarget* target) { ... });
```
**Why?**
1. **Flexibility:** It allows the application logic to be defined locally, often inside `main()`, making simple examples much easier to read.
2. **Modern C++:** Modern C++ prefers composition over inheritance. Utilizing lambdas makes state capture (closures) extremely easy compared to passing state through class members.

## 4. X11 and OpenGL (Initial Linux Platform)
- **X11 over Wayland:** While Wayland is the future of Linux graphics, X11 is still the most universally compatible windowing system, especially inside environments like ChromeOS Crostini or WSL2.
- **OpenGL via GLX:** OpenGL provides universal hardware acceleration. `GLX` was chosen because it is the standard "glue" that binds an OpenGL rendering context to an X11 Window, bypassing the CPU to draw directly on the GPU.

## 5. Strict MVVM-C (Future)
We are aiming for a highly opinionated **Model-View-ViewModel-Controller** structure.
- **Why Opinionated?** Many C++ UI frameworks (like Qt or ImGui) are incredibly flexible but allow developers to easily tangle business logic with rendering code. OOEY will strictly enforce data binding to prevent state bugs. The View will *only* reflect the ViewModel, and the Controller will *only* update the ViewModel.
