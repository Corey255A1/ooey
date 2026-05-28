#include "ooey/platform/platform.hpp"
#include "ooey/i_window_backend.hpp"
#include <cstdlib>

#ifdef OOEY_BUILD_WAYLAND
#include "ooey/platform/wayland/window_backend.hpp"
#endif

#ifdef OOEY_BUILD_X11
#include "ooey/platform/x11/window_backend.hpp"
#endif

#ifdef OOEY_BUILD_FRAMEBUFFER
#include "ooey/platform/framebuffer/window_backend.hpp"
#endif

#ifdef __EMSCRIPTEN__
#include "ooey/platform/emscripten/window_backend.hpp"
#endif

namespace ooey {

std::unique_ptr<IWindowBackend> create_default_window_backend() {
#ifdef __EMSCRIPTEN__
    return std::make_unique<emscripten::WindowBackend>();
#endif

#ifdef OOEY_BUILD_WAYLAND
    if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
        return std::make_unique<wayland::WindowBackend>();
    }
#endif

#ifdef OOEY_BUILD_X11
    if (std::getenv("DISPLAY") != nullptr) {
        return std::make_unique<x11::WindowBackend>();
    }
#endif

#ifdef OOEY_BUILD_FRAMEBUFFER
    if (std::getenv("OOEY_USE_FRAMEBUFFER") != nullptr) {
        return std::make_unique<framebuffer::WindowBackend>();
    }
#endif

    // Fallbacks
#if defined(OOEY_BUILD_FRAMEBUFFER)
    return std::make_unique<framebuffer::WindowBackend>();
#elif defined(OOEY_BUILD_X11)
    return std::make_unique<x11::WindowBackend>();
#elif defined(OOEY_BUILD_WAYLAND)
    return std::make_unique<wayland::WindowBackend>();
#else
    return nullptr;
#endif
}

} // namespace ooey

