# Phase 3: Platform Implementation

## Goal
Implement the first concrete implementations of our abstractions to get a window on the screen.

## Platform Dependencies (Linux / Chromebook)
To support our X11 and OpenGL implementation on Debian-based systems (like ChromeOS Crostini), the following packages must be installed:

- **`libx11-dev`**: Provides the Xlib headers and libraries. This is essential for communicating with the X Window System to create the application window, capture input events, and handle window lifecycle (e.g., listening for window close events). We chose X11 as the initial backend because of its ubiquity and robust compatibility.
- **`libgl1-mesa-dev`**: Provides the core OpenGL rendering headers and libraries (Mesa implementation). We use this to hardware-accelerate our `IRenderTarget` interface, allowing us to perform fast rendering operations like clearing the screen and drawing primitives (like rectangles and lines).
- **`libglx-dev`**: Provides the GLX (OpenGL Extension to the X Window System) interface. This acts as the critical bridge between the X11 window system and the OpenGL rendering context, allowing us to attach our OpenGL drawing commands directly to the native X11 window we create.

*Installation command:*
```bash
sudo apt-get install -y libx11-dev libgl1-mesa-dev libglx-dev
```

## Action Items

1. **Linux/Chromebook Window Backend (Completed)**
   - Chosen backend: X11.
   - Implemented the `IWindowBackend` interface (`X11WindowBackend`).
   - Handles window creation, destruction, and the basic event polling loop.

2. **Initial Render Target (Completed)**
   - Implemented `IRenderTarget` for the window backend using OpenGL (`OpenGLRenderTarget`).
   - Mapped context creation tightly to the X11 Window via GLX.
   - Added `present()` functionality to properly swap the double buffers.

3. **Expanded Rendering Targets (CPU & File) (Completed)**
   - Implemented software CPU rendering in the **Linux Framebuffer backend** (`docs/08-framebuffer-platform.md`).
   - Supports raw pixel formats, Bresenham line drawing, and scanline triangle rasterization with screen rotation.
