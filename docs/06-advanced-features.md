# Phase 6: Media, Assets, and Advanced Features (Pending)

## Goal
Expand the engine's capabilities to include rich media, declarative UI definitions, and broader platform support.

## Action Items

1. **Image Integration (Completed)**
   - Created the `IImageDecoder` API interface supporting dynamic signature-based format sniffing.
   - Built a modular built-in `BmpDecoder` and conditional, native `PngDecoder` linking `libpng` if present.
   - Extended `IRenderTarget` with `draw_image` implemented for CPU (Software), OpenGL, Vulkan (downsampled), and decorated Chrome rendering.
   - Added the `ImageControl` UI layout component.

2. **Audio and Video (Pending)**
   - Explore and integrate `alsa` or `pulseaudio` for audio playback.
   - Explore `ffmpeg` or `gstreamer` for video decoding and rendering into the view hierarchy.

3. **Declarative UI Definition (JSON) (Pending)**
   - Integrate the `nlohmann/json` library.
   - Create a parser that reads a JSON UI definition file and constructs the C++ `View` hierarchy automatically, establishing bindings to a provided `ViewModel`.

4. **WebAssembly / Emscripten (Long-term) (Pending)**
   - Refactor build system and implementations to support compiling the memory buffer or OpenGL output to a web canvas using Emscripten.
