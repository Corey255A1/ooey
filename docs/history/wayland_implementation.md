# Wayland Backend Implementation — Notes

Date: 2026-05-23

Summary
- Implemented a native Wayland window backend prototype (`WaylandWindowBackend`) using `wl_shm` for software rendering and `xkbcommon` for keyboard input.
- Added a minimal `ShmRenderTarget` that presents pixel buffers via `wl_shm` and integrated it with the existing `IRenderTarget` API so the rest of the engine is unchanged.
- Created an example `examples/hello_wayland` mirroring `hello_ooey` so the backend can be exercised manually.

What I changed (files)
- `include/ooey/platform/wayland/wayland_window_backend.hpp` — new header with interface and members for Wayland objects, xdg objects, and input plumbing.
- `src/platform/wayland/wayland_window_backend.cpp` — new implementation:
  - Wayland registry, compositor, shm, seat, xdg surface/toplevel handling
  - `ShmRenderTarget` implementation with simple rasterization
  - xkbcommon keyboard handling and text event pushing
  - pointer event handling (motion + button) feeding `InputManager`
  - lifecycle and cleanup hooks
- `CMakeLists.txt` — added `OOEY_BUILD_WAYLAND` option, pkg-config lookups for `wayland-client`, `wayland-protocols`, and `xkbcommon`; used `wayland-scanner` to generate xdg protocol headers/C; added build wiring for generated C as an OBJECT target.
- `examples/hello_wayland.cpp` — new example using the Wayland backend.

Problems encountered and fixes
- Missing `xdg-shell-client-protocol.h`:
  - Fix: add `wayland-protocols` detection and run `wayland-scanner` during CMake configure to generate the header and a companion `.c` implementation; include the build directory so the header is found.
- Undefined `xdg_wm_base_interface` / link errors:
  - Fix: generate `xdg-shell-client-protocol.c` and compile it as an OBJECT library, then include its objects into the `ooey` static library.
- CMake error about `CMAKE_C_COMPILE_OBJECT` (internal variable) when adding generated C sources to a C++-only project:
  - Fix: enable C language in the project (`project(... LANGUAGES C CXX)`) so C compilation variables are created.
- Compositor warning: "committed initial non-empty content without acknowledging configuration":
  - Cause: xdg-shell requires the client to ACK configure before presenting non-empty content.
  - Fix: delay creating/attaching buffers until after xdg_surface/xdg_toplevel configure is received and acknowledged; only then create shm buffer and present.
- "Data too big for buffer" / Broken pipe (buffer rejected):
  - Cause: shm pool size not aligned to page size, causing a buffer size mismatch.
  - Fix: round shm pool size up to system page size (sysconf) before `ftruncate` so the compositor accepts the buffer.
- Flickering four small squares / incorrect rendering:
  - Initially rasterized geometry as single pixel dots at vertices. Fixed to simple bounding-box fill for primitives (sufficient for the example). A full rasterizer or GPU backend is planned.

Recent debugging & stabilization steps
- Window would appear but either flicker or the compositor rejected buffers. Fixed by:
  - Committing the `wl_surface` after creating `xdg_surface`/`xdg_toplevel` to prompt an initial configure, then ACK before attaching non-empty content.
  - Passing `wl_display*` into `ShmRenderTarget` and waiting for `wl_buffer` release in `present()` (added a buffer release listener). This avoids busy-looping and ensures we don't reuse buffers before the compositor is ready.
  - Adding no-op handlers for newer protocol opcodes (keyboard repeat-info, pointer frame/axis_source/axis_stop/axis_discrete) so listeners match compositor expectations and do not crash when the compositor invokes newer opcodes.
  - Fixing pointer button handling to use the last known pointer coordinates from motion events so click events are delivered at the correct location.
  - Moving xdg configure logic into instance methods to avoid static functions accessing private members (cleaner and safer design).

Runtime observations
- On GNOME (Wayland), the backend now creates a stable window (no more immediate crashes). The simple software renderer shows filled primitives and input (clicks/keys) is delivered to the engine's `InputManager`. Some compositor log messages were resolved by the fixes above.

Lessons learned
- Always align `wl_shm` buffers to page boundaries; small mismatches can silently cause the compositor to drop or kill the client.
- The xdg configure/ack handshake is mandatory before presenting non-empty content — many compositors will work around a missing ack, but it's unsafe and may cause unexpected behavior.
- Wayland protocol versions evolve: listeners must include the full set of callbacks expected by the compositor headers in use (we added missing no-op callbacks to avoid NULL listener crashes).


What I learned about Wayland
- xdg-shell protocol requires a configure/ack dance before presenting non-empty content.
- Generated protocol headers (from `wayland-protocols`) are a normal part of building Wayland clients — `wayland-scanner` can emit both a header and C code.
- `wl_shm` clients must be careful with buffer sizes and alignment (page rounding) to avoid compositor rejections.
- Input requires wiring wl_pointer/wl_keyboard and mapping coordinates and keymaps (xkbcommon) into the engine's `InputManager`.

Quick build & run notes (Fedora 43)
1. Install dependencies:
```bash
sudo dnf install -y wayland-devel wayland-protocols-devel libxkbcommon-devel
```
2. Configure and build with Wayland enabled:
```bash
cmake -S . -B build -DOOEY_BUILD_WAYLAND=ON
cmake --build build -j
```
3. Run example (in a Wayland GNOME session):
```bash
./build/examples/hello_wayland
```

On Chromebook for Wayland
```bash
sudo apt install libwayland-dev wayland-protocols libxkbcommon-dev
```


Remaining work / next steps (prioritized)
1. Add an `EGLRenderTarget` using `wl_egl_window` + EGL + GLES2 to provide GPU-accelerated rendering and a proper GL backend. (medium/large)
2. Implement a correct triangle rasterizer (or integrate a small software raster library) so `IRenderTarget::draw_geometry` can render all primitives accurately. (Completed on 2026-05-27)
3. Improve xdg_toplevel handling:
   - Respect configure size hints and respond to resize, maximize, minimize, and close events properly.
   - Set `app_id` and other window properties.
4. Clean up Wayland resource ownership and free listener data to avoid leaks (e.g. pointer_data_ and keyboard_data_ are std::unique_ptr now, but verify all subclasses). (completed pointer/keyboard listener data wrap)
5. Add nested/headless compositor tests in CI.
6. Add IME/text input support, clipboard, and selection handling.

## Keyboard Input & Keysym Fixes Update (2026-05-25)
- **Status:** Done.
- **Accomplishments:**
  - Standardized Wayland input mapping using xkbcommon keysyms (`xkb_state_key_get_one_sym`) instead of raw keycodes, resolving issues where TextBox was unable to capture Backspace keys.
  - Implemented `keyboard_modifiers` updates via `xkb_state_update_mask()`.
  - Filtered raw key controls from leaking into text event streams.
- **Lessons Learned:**
  - Modifiers mapping in xkbcommon is layout-dependent and requires direct update hooks on `modifiers` callbacks from the compositor; without this, keysym resolving will degrade or fail entirely when layouts or modifier transitions (Shift, Caps) occur.

## Wayland Software Rasterizer & Display Fixes (2026-05-27)
- **Status:** Done.
- **Discovery / Challenge:**
  - When rendering complex shape primitives (such as the analog clock's circle, the clock hands' thick line segments, and the sinusoid wave's curves), the shapes rendered incorrectly on the Wayland backend. The clock appeared as a square, the rotating clock hands appeared as squares changing size, and the sinusoid was rendered as a single long rectangle.
  - The root cause was that `RenderTarget::draw_geometry` inside `src/platform/wayland/render_target.cpp` lacked triangle rasterization. When it encountered `PrimitiveType::Triangles`, it simply calculated the axis-aligned bounding box of all vertices and drew a flat filled rectangle over it.
- **Resolution:**
  - Designed and implemented a mathematically correct software scanline triangle rasterizer (similar to the Framebuffer platform's software rasterizer) inside the Wayland `RenderTarget`.
  - Added declarations and definitions for helper functions: `draw_pixel`, `draw_triangle`, `draw_flat_bottom_triangle`, and `draw_flat_top_triangle`.
  - Updated `RenderTarget::draw_geometry` to traverse geometry indices or sequential vertices and rasterize them triangle-by-triangle using the scanline algorithm.
- **Lessons Learned:**
  - A simple bounding box shortcut for rendering geometry is only sufficient for simple axis-aligned rectangles. Rotating or non-rectangular complex shape primitives require a mathematically correct software rasterizer or a hardware-accelerated GPU backend.
  - Code duplication for software rasterizers can be minimized by abstracting drawing helpers or utilizing a shared CPU rasterizer engine across platform backends.

