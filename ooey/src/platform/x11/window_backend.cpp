#include "ooey/platform/x11/window_backend.hpp"
#include "ooey/renderer/gl_render_target.hpp"
#include "ooey/renderer/window_chrome.hpp"
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
    width_ = size.width;
    height_ = size.height;
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

    render_target_ = std::make_unique<GlRenderTarget>(size.width, size.height, [this]() {
        glXSwapBuffers(display_, window_);
    });

    if (window_chrome_) {
        decorated_render_target_ = std::make_unique<ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width_, height_});
    }

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
    if (!display_ || should_close_) {
        return false;
    }
    XEvent event;
    while (XPending(display_)) {
        XNextEvent(display_, &event);
        if (event.type == ClientMessage) {
            if (static_cast<unsigned long>(event.xclient.data.l[0]) == wm_delete_window_) {
                return false;
            }
        } else if (event.type == DestroyNotify) {
            return false;
        } else if (event.type == ConfigureNotify) {
            width_ = event.xconfigure.width;
            height_ = event.xconfigure.height;
            if (render_target_) {
                render_target_->resize(width_, height_);
            }
            if (decorated_render_target_) {
                decorated_render_target_->resize(width_, height_);
            }
        } else if (input_manager_) {
            if (event.type == MotionNotify) {
                ooey::Pointer p{0, event.xmotion.x, event.xmotion.y, PointerState::Moved};
                if (window_chrome_) {
                    if (window_chrome_->handle_pointer_event(p, Size{width_, height_}, this)) {
                        continue;
                    }
                    p.x -= window_chrome_->get_border_width();
                    p.y -= (window_chrome_->get_border_width() + window_chrome_->get_title_bar_height());
                }
                input_manager_->push_pointer_event(p);
            } else if (event.type == ButtonPress) {
                ooey::Pointer p{0, event.xbutton.x, event.xbutton.y, PointerState::Pressed};
                if (window_chrome_) {
                    if (window_chrome_->handle_pointer_event(p, Size{width_, height_}, this)) {
                        continue;
                    }
                    p.x -= window_chrome_->get_border_width();
                    p.y -= (window_chrome_->get_border_width() + window_chrome_->get_title_bar_height());
                }
                input_manager_->push_pointer_event(p);
            } else if (event.type == ButtonRelease) {
                ooey::Pointer p{0, event.xbutton.x, event.xbutton.y, PointerState::Released};
                if (window_chrome_) {
                    if (window_chrome_->handle_pointer_event(p, Size{width_, height_}, this)) {
                        continue;
                    }
                    p.x -= window_chrome_->get_border_width();
                    p.y -= (window_chrome_->get_border_width() + window_chrome_->get_title_bar_height());
                }
                input_manager_->push_pointer_event(p);
            } else if (event.type == KeyPress) {
                char buffer[32];
                KeySym key_symbol;
                int length = XLookupString(&event.xkey, buffer, sizeof(buffer), &key_symbol, nullptr);
                input_manager_->push_key_event({static_cast<int>(key_symbol), KeyState::Pressed});
                if (length > 0) {
                    for (int i = 0; i < length; ++i) {
                        unsigned char ch = static_cast<unsigned char>(buffer[i]);
                        if (ch >= 32 || ch == '\n' || ch == '\t') {
                            input_manager_->push_text_event({static_cast<char32_t>(ch)});
                        }
                    }
                }
            } else if (event.type == KeyRelease) {
                KeySym key_symbol;
                XLookupString(&event.xkey, nullptr, 0, &key_symbol, nullptr);
                input_manager_->push_key_event({static_cast<int>(key_symbol), KeyState::Released});
            }
        }
    }
    return true;
}

void WindowBackend::poll_input() {
    // Input is currently polled alongside window events in poll_events() for X11.
}

IRenderTarget* WindowBackend::get_render_target() {
    if (decorated_render_target_) {
        return decorated_render_target_.get();
    }
    return render_target_.get();
}

void WindowBackend::set_window_chrome(std::shared_ptr<WindowChrome> chrome) {
    window_chrome_ = chrome;
    if (window_chrome_ && render_target_) {
        decorated_render_target_ = std::make_unique<ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width_, height_});
    } else {
        decorated_render_target_.reset();
    }
}

void WindowBackend::start_interactive_move() {
    if (!display_ || !window_) {
        return;
    }
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    if (XQueryPointer(display_, window_, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask)) {
        XClientMessageEvent xev{};
        xev.type = ClientMessage;
        xev.window = window_;
        xev.message_type = XInternAtom(display_, "_NET_WM_MOVERESIZE", False);
        xev.format = 32;
        xev.data.l[0] = root_x;
        xev.data.l[1] = root_y;
        xev.data.l[2] = 8; // _NET_WM_MOVERESIZE_MOVE
        xev.data.l[3] = Button1;
        xev.data.l[4] = 1;

        XSendEvent(display_, DefaultRootWindow(display_), False, SubstructureRedirectMask | SubstructureNotifyMask, reinterpret_cast<XEvent*>(&xev));
        XUngrabPointer(display_, CurrentTime);
    }
}

void WindowBackend::start_interactive_resize(WindowResizeEdge edge) {
    if (!display_ || !window_) {
        return;
    }
    long action = -1;
    switch (edge) {
        case WindowResizeEdge::TopLeft:     action = 0; break;
        case WindowResizeEdge::Top:         action = 1; break;
        case WindowResizeEdge::TopRight:    action = 2; break;
        case WindowResizeEdge::Right:       action = 3; break;
        case WindowResizeEdge::BottomRight: action = 4; break;
        case WindowResizeEdge::Bottom:      action = 5; break;
        case WindowResizeEdge::BottomLeft:  action = 6; break;
        case WindowResizeEdge::Left:        action = 7; break;
        default: return;
    }
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    if (XQueryPointer(display_, window_, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask)) {
        XClientMessageEvent xev{};
        xev.type = ClientMessage;
        xev.window = window_;
        xev.message_type = XInternAtom(display_, "_NET_WM_MOVERESIZE", False);
        xev.format = 32;
        xev.data.l[0] = root_x;
        xev.data.l[1] = root_y;
        xev.data.l[2] = action;
        xev.data.l[3] = Button1;
        xev.data.l[4] = 1;

        XSendEvent(display_, DefaultRootWindow(display_), False, SubstructureRedirectMask | SubstructureNotifyMask, reinterpret_cast<XEvent*>(&xev));
        XUngrabPointer(display_, CurrentTime);
    }
}

} // namespace ooey::x11
