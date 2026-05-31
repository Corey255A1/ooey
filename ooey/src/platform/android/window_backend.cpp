#include "ooey/platform/android/window_backend.hpp"
#include "ooey/renderer/software_render_target.hpp"
#include "ooey/input.hpp"
#include <android/native_window.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <iostream>
#include <algorithm>

namespace ooey::android {

AAssetManager* g_asset_manager = nullptr;

WindowBackend::WindowBackend(struct android_app* app) : app_(app) {
    if (app_) {
        app_->userData = this;
        g_asset_manager = app_->activity->assetManager;
    }
}

WindowBackend::~WindowBackend() {
    destroy();
}

void WindowBackend::on_window_created(ANativeWindow* window) {
    native_window_ = window;
    if (native_window_) {
        width_ = ANativeWindow_getWidth(native_window_);
        height_ = ANativeWindow_getHeight(native_window_);
        init_software_surface();
    }
}

void WindowBackend::on_window_destroyed() {
    native_window_ = nullptr;
    render_target_.reset();
}

void WindowBackend::on_window_resized() {
    if (native_window_) {
        width_ = ANativeWindow_getWidth(native_window_);
        height_ = ANativeWindow_getHeight(native_window_);
        init_software_surface();
    }
}

void WindowBackend::init_software_surface() {
    if (width_ <= 0 || height_ <= 0) return;

    // Allocate backbuffer for software rasterization
    int stride = width_ * 4;
    software_buffer_.resize(stride * height_, 0);

    auto sw_target = std::make_unique<SoftwareRenderTarget>();
    sw_target->initialize_buffer(
        software_buffer_.data(),
        width_,
        height_,
        stride,
        [this]() { present_software_frame(); }
    );
    render_target_ = std::move(sw_target);
}

void WindowBackend::present_software_frame() {
    if (!native_window_ || software_buffer_.empty()) return;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(native_window_, &buffer, nullptr) == 0) {
        // Copy software_buffer_ to native window buffer row-by-row to handle pitch/stride adjustments
        uint8_t* src = software_buffer_.data();
        uint8_t* dst = static_cast<uint8_t*>(buffer.bits);
        int src_stride = width_ * 4;
        int dst_stride = buffer.stride * 4;

        int rows = std::min(height_, buffer.height);
        int copy_bytes = std::min(width_, buffer.width) * 4;

        for (int y = 0; y < rows; ++y) {
            std::memcpy(dst + y * dst_stride, src + y * src_stride, copy_bytes);
        }

        ANativeWindow_unlockAndPost(native_window_);
    }
}

bool WindowBackend::create(const Size& size, const char* /*title*/) {
    // Width and height are set dynamically by the Android window system.
    // The request size acts as a fallback default.
    width_ = size.width;
    height_ = size.height;
    return true;
}

void WindowBackend::destroy() {
    on_window_destroyed();
    running_ = false;
}

bool WindowBackend::poll_events() {
    if (!app_ || !running_) {
        return false;
    }

    int ident;
    int events;
    struct android_poll_source* source;

    // If the window is not ready/visible, block indefinitely to avoid spinning the CPU.
    // When the window is visible, poll instantly (0ms timeout).
    int timeout_ms = (native_window_ == nullptr) ? -1 : 0;

    // Poll command/sensor queue and dispatch events
    while ((ident = ALooper_pollOnce(timeout_ms, nullptr, &events, (void**)&source)) >= 0) {
        if (source != nullptr) {
            source->process(app_, source);
        }
        if (app_->destroyRequested != 0) {
            running_ = false;
            break;
        }
        // If we were in blocking mode, process one event and break so the main loop can check states.
        if (timeout_ms == -1) {
            break;
        }
    }

    return running_;
}

void WindowBackend::poll_input() {
    // Input is polled natively and processed immediately in the input queue callback.
    // This is a no-op placeholder for Android.
}

IRenderTarget* WindowBackend::get_render_target() {
    return render_target_.get();
}

void WindowBackend::set_input_manager(InputManager* manager) {
    input_manager_ = manager;
}

void WindowBackend::set_window_chrome(std::shared_ptr<WindowChrome> /*chrome*/) {
    // Android uses full-screen mobile layouts; desktop window decorations/chrome are bypassed.
}

std::shared_ptr<WindowChrome> WindowBackend::get_window_chrome() const {
    return nullptr;
}

void WindowBackend::start_interactive_move() {
    // No-op for mobile full-screen views.
}

void WindowBackend::start_interactive_resize(WindowResizeEdge /*edge*/) {
    // No-op for mobile full-screen views.
}

void WindowBackend::handle_pointer_event(int id, float x, float y, PointerState state) {
    if (input_manager_) {
        input_manager_->push_pointer_event(Pointer{id, static_cast<int>(x), static_cast<int>(y), state});
    }
}

void WindowBackend::handle_key_event(int key_code, KeyState state) {
    if (input_manager_) {
        input_manager_->push_key_event(KeyEvent{key_code, state});
    }
}

void WindowBackend::handle_text_event(char32_t codepoint) {
    if (input_manager_) {
        input_manager_->push_text_event(TextEvent{codepoint});
    }
}

void WindowBackend::request_close() {
    running_ = false;
    if (app_) {
        ANativeActivity_finish(app_->activity);
    }
}

Size WindowBackend::get_size() const {
    return Size{width_, height_};
}

} // namespace ooey::android
