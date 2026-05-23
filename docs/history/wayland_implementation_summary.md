# Wayland Backend Implementation — Summary

Date: 2026-05-23

This brief document summarizes the challenges encountered while implementing the `wl_shm` Wayland backend, how they were resolved, and a concise checklist of the basic concepts and steps required to implement a Wayland window backend.

## Challenges encountered — concise recap
- Missing generated protocol headers (`xdg-shell`): solved by invoking `wayland-scanner` during CMake configure and compiling the generated `.c` into the static library.
- CMake/C compilation integration: the project needed `C` language enabled so generated C objects compile; fixed by using `project(... LANGUAGES C CXX)`.
- xdg configure/ack requirements: the compositor may reject or warn if the client attaches non-empty content before ACKing configure; fixed by committing an empty surface, waiting for configure, ACKing it, then creating and attaching buffers.
- wl_shm buffer sizing: buffers must be page-aligned; fixed by rounding shm pool size up to the system page size before `ftruncate` and pool creation.
- Buffer lifecycle and synchronization: needed to wait for `wl_buffer` release from the compositor to avoid reusing buffers prematurely; implemented a buffer-release listener and `wl_display_dispatch` loop in `present()`.
- Protocol listener compatibility: missing/no-op listener slots (newer opcodes) caused crashes; added full listener callbacks (no-ops where unused) to match compositor expectations.

## Wayland basics — how this backend implements a window (overview)
This checklist summarizes the minimal steps and concepts used when implementing a Wayland window backend (wl_shm software path), based on the implementation in `WaylandWindowBackend`.

- Connect to the Wayland compositor: call `wl_display_connect(nullptr)` and obtain the global registry via `wl_display_get_registry()`.
- Bind the required globals from the registry: typically `wl_compositor` (surfaces), `wl_shm` (software buffers), `wl_seat` (input). For window management bind `xdg_wm_base` from `wayland-protocols`.
- Create a `wl_surface` using `wl_compositor_create_surface()`.
- If `xdg_wm_base` is present, create an `xdg_surface` and an `xdg_toplevel` for window management. Add listeners for `xdg_surface` and `xdg_toplevel` and set window metadata (e.g., `xdg_toplevel_set_title()`, `xdg_toplevel_set_app_id()`).
- Prompt the compositor for an initial configure by committing an empty surface. Wait for `xdg_surface` `configure` callback and ACK it with `xdg_surface_ack_configure()` before attaching non-empty content.
- For `wl_shm` rendering:
  - Create an anonymous shared memory file (prefer `memfd_create`, fallback to `shm_open`). Round the size up to the system page size before `ftruncate`.
  - `mmap()` the file, create a `wl_shm_pool` via `wl_shm_create_pool()`, and create a `wl_buffer` with `wl_shm_pool_create_buffer()` using the chosen pixel format.
  - Draw pixels into the mapped memory (respecting stride). To present: call `wl_surface_attach()`, `wl_surface_damage()`, `wl_surface_commit()` and `wl_display_flush()`.
  - Add a `wl_buffer_listener` to detect when the compositor releases the buffer; wait for release before reusing/destroying the buffer.
- Input handling:
  - Obtain `wl_pointer` and `wl_keyboard` via `wl_seat_get_pointer()` / `wl_seat_get_keyboard()` and add `wl_pointer_listener` / `wl_keyboard_listener` callbacks. Provide the full callback tables to remain compatible with compositor versions.
  - Convert `wl_fixed_t` coordinates with `wl_fixed_to_int()` and push pointer events into the engine's input system.
  - Use `xkbcommon` to parse the keymap delivered via the keyboard `keymap` event, create an `xkb_state`, and derive keysyms/UTF-8 text events.
- Protocol generation & build steps:
  - Use `wayland-scanner` to generate client headers and optional C wrappers for protocols like `xdg-shell`. Compile generated `.c` code into the library so protocol interfaces are linked.
- Decorations & titlebars:
  - Decorations are negotiated between client and compositor; to get server-side decorations you must implement or negotiate `xdg-decoration` (or use `libdecor`). Many compositors rely on client-side decorations — implement your own chrome if you need a titlebar immediately.

## Next steps (recommended)
- Add an `EGLRenderTarget` using `wl_egl_window` + EGL + GLES2 for GPU-accelerated rendering.
- Implement a proper triangle rasterizer or integrate a software raster library to fully support `IRenderTarget::draw_geometry` semantics.
- Improve `xdg_toplevel` handling (resize, maximize, close), set `app_id`, and harden resource lifecycle and listener cleanup.


(Added after interactive debugging and tests on GNOME Wayland.)
