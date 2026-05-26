# Framebuffer Platform Backend

To support execution on headless, embedded, and console-only Linux devices without X11 or Wayland display servers, OOEY provides a raw Linux Framebuffer (`/dev/fb0`) platform backend.

## 1. Concepts & Architecture

Unlike desktop display servers that manage windows, compositing, and acceleration, the Linux Framebuffer is a simple memory-mapped interface representing the screen's raw pixel buffer. The OOEY Framebuffer platform implements a custom software rasterizer to draw widgets directly to this pixel buffer.

### Linux Framebuffer Device (`/dev/fb0`)
1. **Device Access**: The backend opens the framebuffer device file (typically `/dev/fb0`) with read/write access.
2. **Screen Information**: It queries the screen configuration using the standard Linux `ioctl` API:
   - `FBIOGET_VSCREENINFO` (struct `fb_var_screeninfo`): Retrieves variable screen features such as resolution (`xres`, `yres`), bits per pixel (`bits_per_pixel`), and color channel bit shifts/offsets.
   - `FBIOGET_FSCREENINFO` (struct `fb_fix_screeninfo`): Retrieves fixed screen features like line length/stride in bytes (`line_length`).
3. **Memory Mapping**: The backend maps the physical video memory into the process address space using `mmap()`, allowing direct pointer manipulation of the pixels.

### Screen Rotation
On many embedded devices (e.g. mobile or portrait panels), the physical LCD hardware is mounted at a rotation (e.g., a physical 480x800 screen rotated 90 degrees). To accommodate this, the backend maps the **logical coordinate space** (used by the GUI layout) to the **physical coordinate space** (written to memory) using the following transformations:

| Rotation | Logical Width | Logical Height | Physical X (`px`) | Physical Y (`py`) |
|---|---|---|---|---|
| **0°** | `phys_w` | `phys_h` | `lx` | `ly` |
| **90°** | `phys_h` | `phys_w` | `phys_w - 1 - ly` | `lx` |
| **180°** | `phys_w` | `phys_h` | `phys_w - 1 - lx` | `phys_h - 1 - ly` |
| **270°** | `phys_h` | `phys_w` | `ly` | `phys_h - 1 - lx` |

By running all pixel drawing functions through this coordinate mapper, the application automatically gains full layout rotation support without modifying view definitions or coordinates.

### Double Buffering
Writing directly to the memory-mapped video memory (`/dev/fb0`) can lead to visual artifacts, such as flickering and screen tearing. This happens because video hardware reads from the buffer asynchronously while the application is drawing.

To solve this, the backend implements **double buffering**:
1. All drawing operations (clearing, rasterizing lines/triangles, and text rendering) write to a private **backbuffer** allocated in system memory.
2. During the `present()` call, the complete backbuffer is copied to the physical framebuffer mapping (`fb_mem_`) using a highly optimized `std::memcpy`.

---

## 2. Software Rasterization

Because the raw framebuffer does not support GPU acceleration or APIs like OpenGL, the rendering target must perform all drawing operations in software.

### Pixel Format Encoding
Different framebuffer devices configure channels differently. The software renderer dynamically queries the color offsets and lengths from the `fb_var_screeninfo` struct and formats pixels accordingly:

```cpp
// 32-bit pixel compilation (ARGB/BGRA/RGBA)
uint32_t r = (color.r >> (8 - vinfo.red.length)) << vinfo.red.offset;
uint32_t g = (color.g >> (8 - vinfo.green.length)) << vinfo.green.offset;
uint32_t b = (color.b >> (8 - vinfo.blue.length)) << vinfo.blue.offset;
uint32_t pixel = r | g | b | a;
```

It supports 32-bit (ARGB/BGRA/RGBA), 24-bit (RGB888), and 16-bit (RGB565) display panels.

### Line Drawing (Bresenham's Algorithm)
Lines are drawn pixel-by-pixel using an integer-only version of Bresenham's line algorithm. This is extremely fast because it avoids floating-point arithmetic:

```cpp
int error = delta_x + delta_y;
while (true) {
    draw_pixel(start_x, start_y, color);
    if (start_x == end_x && start_y == end_y) break;
    int error2 = 2 * error;
    if (error2 >= delta_y) { error += delta_y; start_x += step_x; }
    if (error2 <= delta_x) { error += delta_x; start_y += step_y; }
}
```

### Triangle Rasterization (Scanline Fill)
Rather than simple bounding box fill approximations, OOEY implements a mathematically correct software scanline triangle rasterizer:
1. **Sort Vertices**: Sort the three vertices of a triangle by their Y coordinate ($y_0 \le y_1 \le y_2$).
2. **Special Cases**: If the triangle has a flat bottom ($y_1 == y_2$) or a flat top ($y_0 == y_1$), it draws scanlines from left to right.
3. **General Triangles**: Otherwise, it splits the triangle into a flat-bottom and a flat-top triangle by computing a new vertex $P_3$ along the long edge $P_0 P_2$ at height $y_1$:
   $$x_3 = x_0 + \frac{y_1 - y_0}{y_2 - y_0} \times (x_2 - x_0)$$
   It then rasterizes both sub-triangles.

---

## 3. How to Use & Configure

### Configuration via Environment Variables

The Framebuffer backend can be controlled dynamically via environment variables without re-compiling the application code:

* **`OOEY_USE_FRAMEBUFFER`**: Forces the application to select the Framebuffer backend, bypassing desktop displays (X11/Wayland).
* **`OOEY_FB_DEVICE`**: Specifies the device path. (Default: `/dev/fb0`).
* **`OOEY_FB_ROTATION`**: Specifies the screen rotation. Supported values: `0`, `90`, `180`, `270`. (Default: `0`).

*Example:* Run a hello world demo on a portrait screen rotated 90 degrees:
```bash
OOEY_USE_FRAMEBUFFER=1 OOEY_FB_ROTATION=90 ./build/examples/hello_ooey
```

### Programmatic Construction

Alternatively, you can instantiate the backend directly in your application code:

```cpp
#include "ooey/platform/framebuffer/window_backend.hpp"

int main() {
    ooey::Application app;
    
    // Explicitly configure portrait rotated 90°
    auto backend = std::make_unique<ooey::framebuffer::WindowBackend>(90, "/dev/fb0");
    backend->create({800, 480}, "Framebuffer Demo");
    
    app.set_window_backend(std::move(backend));
    // ...
}
```

### Build Option

The Framebuffer backend is configured via CMake. It is enabled by default on Linux platforms:

```bash
# Force compile option
cmake -DOOEY_BUILD_FRAMEBUFFER=ON ..
```
