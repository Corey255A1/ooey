# Vulkan Graphics Pipeline & Wayland Subclassing — Implementation Notes

**Date:** 2026-05-30

## 1. Summary
*   **Vulkan Render Target:** Designed and implemented a modern graphics render target ([VulkanRenderTarget](file:///home/corey/code/ooey/ooey/include/ooey/renderer/vulkan_render_target.hpp)) that compiles and binds geometric layouts using the Vulkan graphics API.
*   **Wayland Subclass Refactoring:** Simplified the Wayland backend setup by refactoring EGL/OpenGL code out of the base [WindowBackend](file:///home/corey/code/ooey/ooey/include/ooey/platform/wayland/window_backend.hpp). The base class now provides common Wayland input/window properties and defaults to software SHM.
*   **EGL Subclass:** Created [EglWindowBackend](file:///home/corey/code/ooey/ooey/include/ooey/platform/wayland/egl_window_backend.hpp) specifically to contain OpenGL ES/EGL window hooks, maintaining automatic software rasterization fallback if context initialization fails.
*   **Vulkan Subclass:** Created [VulkanWindowBackend](file:///home/corey/code/ooey/ooey/include/ooey/platform/wayland/vulkan_window_backend.hpp) to initialize Vulkan instances, physical/logical devices, graphics queues, and Wayland window surface bindings.
*   **Example Application:** Created [hello_wayland_vulkan.cpp](file:///home/corey/code/ooey/examples/hello_wayland_vulkan.cpp) to verify and showcase Vulkan rendering inside a Wayland compositor.

---

## 2. File Modifications

*   `ooey/include/ooey/renderer/vulkan_render_target.hpp` & `ooey/src/renderer/vulkan_render_target.cpp` — Added the Vulkan rendering engine class implementing [IRenderTarget](file:///home/corey/code/ooey/ooey/include/ooey/renderer/i_render_target.hpp). Handles swapchain state, command buffers, synchronization fences, pipeline allocations, and draw recordings.
*   `ooey/include/ooey/platform/wayland/window_backend.hpp` & `ooey/src/platform/wayland/window_backend.cpp` — Refactored members to `protected` scope and added virtual lifecycle functions (`init_graphics_context`, `cleanup_graphics_context`, `recreate_render_target`) to support subclassing. Removed EGL dependencies from the base class.
*   `ooey/include/ooey/platform/wayland/egl_window_backend.hpp` & `ooey/src/platform/wayland/egl_window_backend.cpp` — Subclassed base Wayland backend to implement standard EGL context initialization and window surface resizing.
*   `ooey/include/ooey/platform/wayland/vulkan_window_backend.hpp` & `ooey/src/platform/wayland/vulkan_window_backend.cpp` — Subclassed base Wayland backend to build Vulkan instances, select GPU families, hook Wayland window surfaces via `vkCreateWaylandSurfaceKHR`, and bind the [VulkanRenderTarget](file:///home/corey/code/ooey/ooey/include/ooey/renderer/vulkan_render_target.hpp).
*   `ooey/include/ooey/renderer/vulkan_shaders.hpp` — Contains embedded vertex and fragment SPIR-V shader bytecode compiled offline to allow self-contained application execution without runtime shader file dependencies.
*   `examples/hello_wayland_vulkan.cpp` — Interactive C++ example program running on the Vulkan Wayland backend.
*   `CMakeLists.txt` & `examples/CMakeLists.txt` — Added Vulkan package lookup (`find_package`), linked `Vulkan::Vulkan` to target `ooey`, and wired compilation targets.
*   `ooey/src/platform.cpp` — Registered subclass instantiations. Added dynamic environment variable parsing (`OOEY_WAYLAND_BACKEND=vulkan/egl/shm`).

---

## 3. Dependencies

*   **Runtime Drivers:** Mesa GPU drivers supporting Vulkan (such as `intel_icd`, `radeon_icd`, or `virtio_icd`) or the CPU software rasterizer `lvp` (Mesa lavapipe).
*   **Compile-time Packages (Linux/Crostini):**
    ```bash
    sudo apt-get install -y libvulkan-dev glslang-tools spirv-tools
    ```
    *   `libvulkan-dev`: Vulkan header definitions (`<vulkan/vulkan.h>`) and linker targets.
    *   `glslang-tools` / `spirv-tools`: Used during development to validate and compile GLSL shader files into binary SPIR-V bytecode arrays.

---

## 4. Problems Encountered & Solved

*   **Incomplete Shader Compilation Chain:** 
    *   *Problem:* The virtual build sandbox lacked offline GLSL compilers (`glslc` or `glslangValidator`), making standard CMake shader build commands fail.
    *   *Solution:* Installed `glslang-tools` on the environment. Wrote a Python script to compile [shader.vert](file:///home/corey/code/ooey/build/shader.vert) and [shader.frag](file:///home/corey/code/ooey/build/shader.frag) to SPIR-V, formatting the bytecode outputs into standard `uint32_t` arrays in [vulkan_shaders.hpp](file:///home/corey/code/ooey/ooey/include/ooey/renderer/vulkan_shaders.hpp). This allows compiling OOEY without any runtime GLSL compiler dependencies.
*   **Missing standard library headers:** 
    *   *Problem:* Compile errors occurred due to missing type definitions for `std::numeric_limits` on unsigned integers.
    *   *Solution:* Included `<limits>` in [vulkan_render_target.cpp](file:///home/corey/code/ooey/ooey/src/renderer/vulkan_render_target.cpp).
*   **Vulkan color component naming mismatch:** 
    *   *Problem:* Compile errors from using incorrect flag constants in color blending states (`VK_COLOR_WRITE_MASK_R_BIT` was undefined).
    *   *Solution:* Substituted with standard enum names `VK_COLOR_COMPONENT_R_BIT`, `VK_COLOR_COMPONENT_G_BIT`, `VK_COLOR_COMPONENT_B_BIT`, and `VK_COLOR_COMPONENT_A_BIT`.
*   **Blank/Black Rendering under Crostini/Linux:**
    *   *Problem:* The initial prototype window showed black because the geometry drawing interface `draw_geometry` was empty, recording only swapchain image clearing.
    *   *Solution:* Designed a complete Vulkan graphics pipeline:
        *   Added a pipeline layout using push constants to pass screen sizes (eliminating complex uniform descriptor pools).
        *   Compiled two separate graphics pipelines for `Triangle` list and `Line` list topologies.
        *   Implemented dynamic VBO/IBO buffer allocation and mapped vertex structures during present frames.
        *   Recorded `vkCmdDrawIndexed` commands for active draw collections inside the swapchain render pass.
*   **Silent Vulkan Failures / Missing validation:**
    *   *Problem:* GPU virtualization layers (like VM/Crostini environments) can silently fail swapchain creation or ignore surface commits without throwing crashes, resulting in black screens.
    *   *Solution:* Enabled Vulkan validation layers (`VK_LAYER_KHRONOS_validation`) if the user runs the application with the environment variable `OOEY_VULKAN_VALIDATION=1`. Also, wrapped all initialization tasks inside a global `try-catch` block in [hello_wayland_vulkan.cpp](file:///home/corey/code/ooey/examples/hello_wayland_vulkan.cpp) to report detailed driver/device information to `stderr` upon crashes.

---

## 5. Lessons Learned & Discoveries

*   **Pixel Coordinate space translation:** Vulkan expects Normalized Device Coordinates (NDC) ranging from `[-1..1]`. To support OOEY's pixel-based scene graph, push constants are used to pass screen width/height to the vertex shader on every draw call, converting positions via `x_ndc = (x / width) * 2.0 - 1.0`.
*   **Mesa/Crostini driver quirks:** In virtualized environments (like Chromebook Crostini running `virgl` or `venus`), Vulkan presents buffers asynchronously. Synchronization fences and semaphores are highly critical; missing checks can immediately lock the compositor or return blank screens.
*   **Zero-Descriptor push constant efficiency:** Standard Vulkan rendering uses descriptor sets for matrices or UBOs. By leveraging push constants for the viewport size, we avoided all descriptor set layout, pool, and allocation overhead, simplifying the pipeline code and improving rendering performance.
