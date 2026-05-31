#include "ooey/platform.hpp"
#include "ooey/i_window_backend.hpp"
#include <cstdlib>
#include <fstream>

#ifdef OOEY_BUILD_ANDROID
#include <android/asset_manager.h>
namespace ooey::android {
    extern AAssetManager* g_asset_manager;
}
#endif

#ifdef OOEY_BUILD_WAYLAND
#include "ooey/platform/wayland/window_backend.hpp"
#include "ooey/platform/wayland/egl_window_backend.hpp"
#include "ooey/platform/wayland/vulkan_window_backend.hpp"
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

#ifdef OOEY_BUILD_ANDROID
#include "ooey/platform/android/window_backend.hpp"
#endif

namespace ooey {

std::unique_ptr<IWindowBackend> create_default_window_backend() {
#ifdef OOEY_BUILD_ANDROID
    return nullptr;
#endif

#ifdef __EMSCRIPTEN__
    return std::make_unique<emscripten::WindowBackend>();
#endif

#ifdef OOEY_BUILD_WAYLAND
    if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
        const char* backend_env = std::getenv("OOEY_WAYLAND_BACKEND");
        if (backend_env != nullptr) {
            std::string type(backend_env);
            if (type == "vulkan") {
                return std::make_unique<wayland::VulkanWindowBackend>();
            } else if (type == "shm" || type == "software") {
                return std::make_unique<wayland::WindowBackend>();
            }
        }
        return std::make_unique<wayland::EglWindowBackend>();
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
    const char* backend_env = std::getenv("OOEY_WAYLAND_BACKEND");
    if (backend_env != nullptr) {
        std::string type(backend_env);
        if (type == "vulkan") {
            return std::make_unique<wayland::VulkanWindowBackend>();
        } else if (type == "shm" || type == "software") {
            return std::make_unique<wayland::WindowBackend>();
        }
    }
    return std::make_unique<wayland::EglWindowBackend>();
#else
    return nullptr;
#endif
}

std::vector<uint8_t> read_asset(const std::string& path) {
#ifdef OOEY_BUILD_ANDROID
    if (android::g_asset_manager) {
        AAsset* asset = AAssetManager_open(android::g_asset_manager, path.c_str(), AASSET_MODE_BUFFER);
        if (asset) {
            size_t size = AAsset_getLength(asset);
            std::vector<uint8_t> buffer(size);
            int read_bytes = AAsset_read(asset, buffer.data(), size);
            AAsset_close(asset);
            if (read_bytes >= 0) {
                buffer.resize(read_bytes);
                return buffer;
            }
        }
    }
#endif

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        // Fallback for execution within build/ directory on desktop
        std::ifstream file_fallback("../" + path, std::ios::binary | std::ios::ate);
        if (file_fallback) {
            std::streamsize size = file_fallback.tellg();
            file_fallback.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(size);
            if (file_fallback.read(reinterpret_cast<char*>(buffer.data()), size)) {
                return buffer;
            }
        }
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

} // namespace ooey

