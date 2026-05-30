#include "ooey/platform/wayland/egl_window_backend.hpp"
#include "ooey/renderer/gl_render_target.hpp"
#include "ooey/renderer/window_chrome.hpp"
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GL/gl.h>
#include <iostream>

namespace ooey::wayland {

EglWindowBackend::EglWindowBackend() = default;

EglWindowBackend::~EglWindowBackend() {
    EglWindowBackend::cleanup_graphics_context();
}

bool EglWindowBackend::create_egl_display() {
    egl_display_ = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(display_));
    if (egl_display_ == EGL_NO_DISPLAY) {
        std::cerr << "Wayland EGL: eglGetDisplay failed\n";
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(egl_display_, &major, &minor)) {
        std::cerr << "Wayland EGL: eglInitialize failed\n";
        egl_display_ = nullptr;
        return false;
    }

    std::cout << "Wayland EGL initialized: version " << major << "." << minor << "\n";

    if (!eglBindAPI(EGL_OPENGL_API)) {
        std::cerr << "Wayland EGL: eglBindAPI failed\n";
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
        return false;
    }
    return true;
}

bool EglWindowBackend::choose_egl_config() {
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(egl_display_, attribs, &egl_config_, 1, &num_configs) || num_configs < 1) {
        std::cerr << "Wayland EGL: eglChooseConfig failed\n";
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
        return false;
    }
    return true;
}

bool EglWindowBackend::create_egl_context() {
    EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };

    egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT, context_attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
        egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT, nullptr);
    }

    if (egl_context_ == EGL_NO_CONTEXT) {
        std::cerr << "Wayland EGL: eglCreateContext failed\n";
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
        return false;
    }
    return true;
}

bool EglWindowBackend::init_graphics_context() {
    if (!create_egl_display()) {
        return false;
    }
    if (!choose_egl_config()) {
        return false;
    }
    if (!create_egl_context()) {
        return false;
    }
    return true;
}

void EglWindowBackend::cleanup_graphics_context() {
    if (egl_display_) {
        eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_surface_) {
            eglDestroySurface(egl_display_, egl_surface_);
            egl_surface_ = nullptr;
        }
        if (egl_context_) {
            eglDestroyContext(egl_display_, egl_context_);
            egl_context_ = nullptr;
        }
        if (egl_window_) {
            wl_egl_window_destroy(egl_window_);
            egl_window_ = nullptr;
        }
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
    }
}

void EglWindowBackend::recreate_render_target(int width, int height) {
    if (egl_window_) {
        wl_egl_window_resize(egl_window_, width, height, 0, 0);
        if (render_target_) {
            render_target_->resize(width, height);
        }
    } else {
        egl_window_ = wl_egl_window_create(surface_, width, height);
        if (!egl_window_) {
            std::cerr << "Wayland EGL error: Failed to create wl_egl_window. Falling back to software.\n";
            WindowBackend::recreate_render_target(width, height);
            return;
        }
        egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_, reinterpret_cast<EGLNativeWindowType>(egl_window_), nullptr);
        if (egl_surface_ == EGL_NO_SURFACE) {
            std::cerr << "Wayland EGL error: Failed to create EGL surface. Falling back to software.\n";
            wl_egl_window_destroy(egl_window_);
            egl_window_ = nullptr;
            WindowBackend::recreate_render_target(width, height);
            return;
        }
        if (!eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_)) {
            std::cerr << "Wayland EGL error: Failed to make EGL context current. Falling back to software.\n";
            eglDestroySurface(egl_display_, egl_surface_);
            egl_surface_ = nullptr;
            wl_egl_window_destroy(egl_window_);
            egl_window_ = nullptr;
            WindowBackend::recreate_render_target(width, height);
            return;
        }
        render_target_ = std::make_unique<GlRenderTarget>(width, height, [this]() {
            if (egl_display_ && egl_surface_) {
                eglSwapBuffers(egl_display_, egl_surface_);
            }
        });
    }

    if (window_chrome_) {
        decorated_render_target_ = std::make_unique<ooey::ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width, height});
    } else {
        decorated_render_target_.reset();
    }
}

} // namespace ooey::wayland
