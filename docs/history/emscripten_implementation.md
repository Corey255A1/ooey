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
