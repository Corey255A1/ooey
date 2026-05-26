#include "ooey/platform/x11/window_backend.hpp"
#include "ooey/platform/x11/render_target.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <iostream>

namespace ooey::x11 {

WindowBackend::WindowBackend() = default;

WindowBackend::~WindowBackend() {
    destroy();
}

bool WindowBackend::create(const Size& size, const char* title) {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        std::cerr << "Cannot open X display\n";
        return false;
    }

    Window root = DefaultRootWindow(display_);

    GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    XVisualInfo* vi = glXChooseVisual(display_, 0, att);
    if (!vi) {
        std::cerr << "No appropriate visual found\n";
        return false;
    }

    Colormap cmap = XCreateColormap(display_, root, vi->visual, AllocNone);

    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;

    window_ = XCreateWindow(display_, root, 0, 0, size.width, size.height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(display_, window_);
    XStoreName(display_, window_, title);

    Atom wm_delete = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wm_delete, 1);
    wm_delete_window_ = wm_delete;

    glc_ = glXCreateContext(display_, vi, nullptr, GL_TRUE);
    glXMakeCurrent(display_, window_, (GLXContext)glc_);
    XFree(vi);

    // Enable blending for alpha
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_target_ = std::make_unique<RenderTarget>(display_, window_);

    return true;
}

void WindowBackend::destroy() {
    if (glc_) {
        glXMakeCurrent(display_, None, nullptr);
        glXDestroyContext(display_, (GLXContext)glc_);
        glc_ = nullptr;
    }
    if (window_) {
        XDestroyWindow(display_, window_);
        window_ = 0;
    }
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

bool WindowBackend::poll_events() {
    if (!display_) {
        return false;
    }
    XEvent xev;
    while (XPending(display_)) {
        XNextEvent(display_, &xev);
        if (xev.type == ClientMessage) {
            if (static_cast<unsigned long>(xev.xclient.data.l[0]) == wm_delete_window_) {
                return false;
            }
        } else if (xev.type == DestroyNotify) {
            return false;
        } else if (input_manager_) {
            if (xev.type == MotionNotify) {
                input_manager_->push_pointer_event({0, xev.xmotion.x, xev.xmotion.y, PointerState::Moved});
            } else if (xev.type == ButtonPress) {
                input_manager_->push_pointer_event({0, xev.xbutton.x, xev.xbutton.y, PointerState::Pressed});
            } else if (xev.type == ButtonRelease) {
                input_manager_->push_pointer_event({0, xev.xbutton.x, xev.xbutton.y, PointerState::Released});
            } else if (xev.type == KeyPress) {
                char buffer[32];
                KeySym keysym;
                int len = XLookupString(&xev.xkey, buffer, sizeof(buffer), &keysym, nullptr);
                input_manager_->push_key_event({static_cast<int>(keysym), KeyState::Pressed});
                if (len > 0) {
                    for (int i = 0; i < len; ++i) {
                        unsigned char ch = static_cast<unsigned char>(buffer[i]);
                        if (ch >= 32 || ch == '\n' || ch == '\t') {
                            input_manager_->push_text_event({static_cast<char32_t>(ch)});
                        }
                    }
                }
            } else if (xev.type == KeyRelease) {
                KeySym keysym;
                XLookupString(&xev.xkey, nullptr, 0, &keysym, nullptr);
                input_manager_->push_key_event({static_cast<int>(keysym), KeyState::Released});
            }
        }
    }
    return true;
}

void WindowBackend::poll_input() {
    // Input is currently polled alongside window events in poll_events() for X11.
}

IRenderTarget* WindowBackend::get_render_target() {
    return render_target_.get();
}

} // namespace ooey::x11
