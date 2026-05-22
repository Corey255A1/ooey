# Phase 2: Core Architecture & Abstractions (Completed)

## Goal
Define the fundamental interfaces and data structures that will allow OOEY to render to various targets (window, memory, file) cross-platform.

## Action Items

1. **Basic Data Structures (Completed)**
   - Implement primitive types in C++ (`include/ooey/types.hpp`):
     - `Color` (RGBA representation).
     - `Point` (X, Y coordinates).
     - `Size` (Width, Height).
     - `Rect` (X, Y, Width, Height).

2. **Abstract RenderTarget Interface (Completed)**
   - Define `IRenderTarget` class with pure virtual methods for drawing operations (e.g., `draw_rect`, `draw_line`, `clear`, `present`).
   - This interface will be implemented differently depending on where we are rendering.

3. **Abstract WindowBackend Interface (Completed)**
   - Define `IWindowBackend` class.
   - Methods for creating a window, handling the event loop, polling input, and retrieving the `IRenderTarget`.
   - Ensure it is decoupled from the actual OS (Wayland, X11, Windows, macOS).

4. **Engine Core (Completed)**
   - Create an `Application` class that manages the main loop, holds a reference to the `IWindowBackend`, handles a per-frame render callback (`std::function<void(IRenderTarget*)>`), and coordinates timing.
