# Optional Emscripten Platform Target Implementation

Date: 2026-05-28

This document outlines the architectural strategy, file changes, and compilation workflow to support cross-compiling the `ooey` GUI framework to WebAssembly/HTML5 via Emscripten.

## 1. Architectural Strategy

In desktop environments, `ooey` runs a blocking while loop inside `Application::run()`. However, web browsers run an asynchronous event-driven loop on a single thread. Blocking this thread freezes the browser tab.

To accommodate this, we refactored the application loop:
- **`Application::run_iteration()`**: Extracted a single frame tick (polling events, updating controller state, executing render callbacks, drawing the scene graph, and presenting the frame buffer).
- **Asynchronous Main Loop**: When compiled with the Emscripten compiler (`__EMSCRIPTEN__` flag), `Application::run()` registers `run_iteration` via `emscripten_set_main_loop_arg()`. Emscripten's scheduler invokes this callback at the browser's refresh rate (typically $60\text{ fps}$) while allowing the browser main thread to remain responsive.

## 2. Emscripten WindowBackend & RenderTarget

We added a new platform module under `platform/emscripten`:

### HTML5 Input Event Mapping
Using Emscripten's HTML5 API, `WindowBackend` registers callbacks directly on the HTML canvas element (`#canvas`):
- **Mouse & Touch**: Mouse movement, mouse down, and mouse up events are captured. Their canvas-relative positions (`mouse_event->targetX`, `mouse_event->targetY`) are retrieved and pushed to the `InputManager` as `Pointer` events.
- **Keyboard**: Keyboard keydown and keyup events are registered globally on the document. Common control keys (like `Backspace` and `Delete`) are mapped to their corresponding physical keycodes. Printable characters are decoded from the UTF-8 key representation and pushed as standard `TextEvent` codepoints.

### WebGL Legacy GL Emulation
The `RenderTarget` manages rendering into a WebGL context:
- A WebGL 1.0 context is created on the canvas element.
- Emscripten compiles legacy OpenGL fixed-function pipeline calls (e.g. `glBegin`, `glEnd`, `glVertex2f`, `glColor4f`, `glOrtho`) by emulating them over WebGL shader programs under the hood (configured via the `-sLEGACY_GL_EMULATION=1` flag).
- Text rendering is performed by the software `BitmapFont` engine. It draws character glyphs as rectangular blocks mapped to WebGL legacy quads.

## 3. Setting Up the Emscripten SDK (emsdk)

To compile the `ooey` library to WebAssembly, you need the Emscripten toolchain set up on your machine. Follow these steps to install and activate it:

1. **Clone the SDK Repository**:
   Clone the official Emscripten SDK repository from GitHub to a directory of your choice:
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ```

2. **Download and Install the Latest Toolchain**:
   Run the installation scripts to fetch the latest stable version of the compiler:
   ```bash
   ./emsdk install latest
   ```

3. **Activate the SDK**:
   Activate the installed SDK components for your user account:
   ```bash
   ./emsdk activate latest
   ```

4. **Configure Environment Variables**:
   Source the environment activation script to place `emcc`, `emcmake`, and other toolchain binaries into your current terminal session path:
   ```bash
   source ./emsdk_env.sh
   ```
   *Tip: To make this configuration persistent, you can append `source /path/to/emsdk/emsdk_env.sh > /dev/null` to your shell's rc file (e.g. `~/.bashrc` or `~/.zshrc`).*

5. **Verify Installation**:
   Verify that the Emscripten compiler is accessible:
   ```bash
   emcc --version
   ```

## 4. How to Compile and Run

To cross-compile the target to WebAssembly, you need the Emscripten SDK (emsdk) active in your environment.

### Compile Commands
1. Create a build directory and configure with `emcmake`:
   ```bash
   emcmake cmake -B build-wasm
   ```
2. Build the targets:
   ```bash
   cmake --build build-wasm
   ```
This generates the WebAssembly binary `hello_emscripten.wasm`, the JavaScript glue code `hello_emscripten.js`, and the modern HTML page `hello_emscripten.html` (which uses the custom `examples/shell.html` layout).

### Serving the Web App
WebAssembly applications cannot be loaded directly from the local file system (`file://`) due to browser security restrictions. You must serve them via a local HTTP server:
```bash
python3 -m http.server -d build-wasm/examples 8080
```
Open your browser and navigate to `http://localhost:8080/hello_emscripten.html` to run the GUI demo.

## 5. WebAssembly Compatibility Refinements (2026-05-31)

To ensure the Emscripten/WASM compilation succeeds cleanly without environment mismatches or header dependency leaks, several architectural refinements were made:

### Vulkan Backend Guarding & Platform Isolation
- **Headers & Source Guarding:** Added `#ifndef __EMSCRIPTEN__` preprocessor guards around [vulkan_render_target.hpp](file:///home/corey/code/ooey/ooey/include/ooey/renderer/vulkan_render_target.hpp) and [vulkan_render_target.cpp](file:///home/corey/code/ooey/ooey/src/renderer/vulkan_render_target.cpp) to prevent any inclusion of `<vulkan/vulkan.h>` under the WASM target (where Vulkan is unsupported).
- **CMake Refinement:** Modified the main [CMakeLists.txt](file:///home/corey/code/ooey/CMakeLists.txt) to conditionally exclude `vulkan_render_target.cpp` from `OOEY_SRCS` when compiling under Emscripten.

### Image Class Namespace Ambiguity Resolution
- **Conflicting Declarations:** In [image_control.hpp](file:///home/corey/code/ooey/gooey/include/gooey/controls/image_control.hpp), the forward declaration of `class Image` was defined directly in `namespace ooey`. This clashed with the `using renderer::Image;` alias imported from rendering targets, causing compilation failures due to ambiguous symbol resolution.
- **Corrected Aliasing:** Forward-declared `Image` inside `namespace ooey::renderer` and declared `using renderer::Image;` in `namespace ooey` to match the core framework structure.

### Example & Test Target Isolation
- **X11 Exclusions:** Wrapped the compilation of the direct X11 example `hello_ooey_text_x11` inside [examples/CMakeLists.txt](file:///home/corey/code/ooey/examples/CMakeLists.txt) with `if(OOEY_BUILD_X11)` to prevent build failures when target platforms do not support X11 (such as Emscripten).
- **Framebuffer Test Exclusions:** Conditionalized `test_framebuffer.cpp` in [tests/CMakeLists.txt](file:///home/corey/code/ooey/tests/CMakeLists.txt) on `if(OOEY_BUILD_FRAMEBUFFER)` to avoid missing `<linux/fb.h>` errors on WASM and non-Linux systems.

### SinusoidPrimitive Constructor Correction
- Fixed an invalid instantiation of `SinusoidPrimitive` inside [hello_emscripten.cpp](file:///home/corey/code/ooey/examples/hello_emscripten.cpp) that attempted to pass a `Rect` instead of the signature-conforming `Point start, Point end` coordinate layout, bringing the WebAssembly demo in line with standard API guidelines.

### WebGL Legacy GL Emulation Runtime Fix
- **Context Initialization Bypasses:** Raw C-level context creation via `emscripten_webgl_create_context` bypassed the JS-level `Browser.createContext()` call, preventing `Browser.useWebGL` from being set to `true` and skipping the invocation of `GLImmediate.init()` callbacks. This caused `glEnable` (and other legacy GL emulation wrappers) to fail with a `TypeError: Cannot read properties of null (reading '0')` when querying texture units.
- **Forced Emulation Bootstrapping:** In [window_backend.cpp](file:///home/corey/code/ooey/ooey/src/platform/emscripten/window_backend.cpp), injected an `EM_ASM` block immediately following `emscripten_webgl_make_context_current` to explicitly set `Browser.useWebGL = true` and call `GLImmediate.init()`, enabling runtime execution of all WASM example pages.
- **WebGL Texture Format Compatibility:** Resolved a WebGL error (`INVALID_VALUE: texImage2D: invalid internalformat`) by conditionalizing `glTexImage2D` calls in [gl_render_target.cpp](file:///home/corey/code/ooey/ooey/src/renderer/gl_render_target.cpp). WebGL 1.0 requires unsized internal formats like `GL_RGBA` instead of the sized `GL_RGBA8` format used on desktop.
- **Canvas Scaling & Stretching Resolution:** Fixed a blurry, zoomed-in rendering issue on WASM examples by updating the `EM_ASM` block in [window_backend.cpp](file:///home/corey/code/ooey/ooey/src/platform/emscripten/window_backend.cpp) to explicitly resize the HTML5 canvas element’s internal resolution (`canvas.width` and `canvas.height`) to match the requested logical size of the window (e.g., 800x600), overriding the browser's default canvas size (300x150).

## 6. Target Consolidation with Android (2026-06-01)

As part of adding Android NDK support, the platform backend configuration in [CMakeLists.txt](file:///home/corey/code/ooey/CMakeLists.txt) was consolidated:
- **Unified Desktop Exclusions:** Changed the `CMakeLists.txt` check from `if(EMSCRIPTEN)` to `if(EMSCRIPTEN OR ANDROID)` to cleanly disable desktop X11, Wayland, and Framebuffer backends for both mobile and web targets.
- **Shared Target Isolation:** The dynamic cross-compilation pipeline isolates standard library dependencies and OpenGL configurations, ensuring that only target-specific code compiles for both Emscripten (using WebGL emulation) and Android (using pure software presentation).
- **Entry Point Abstraction:** Both platforms adapt the standard blocking loop to their respective runtime lifecycles. Emscripten delegates tick loops asynchronously to the browser main thread, while Android NDK's `android_main` dynamically saves the OS application context and boots the standard `main()` function. This allows identical application files to compile and run across web and mobile targets.

