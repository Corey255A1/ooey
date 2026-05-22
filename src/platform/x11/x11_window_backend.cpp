#include "ooey/platform/x11/x11_window_backend.hpp"
#include "ooey/i_render_target.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <iostream>

namespace ooey {

class OpenGLRenderTarget : public IRenderTarget {
public:
    OpenGLRenderTarget(Display* display, Window window)
        : display_(display), window_(window) {}

    void clear(Color color) override {
        XWindowAttributes gwa;
        XGetWindowAttributes(display_, window_, &gwa);
        glViewport(0, 0, gwa.width, gwa.height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, gwa.width, gwa.height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void draw_rect(const Rect& rect, Color color) override {
        glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glBegin(GL_QUADS);
        glVertex2i(rect.x, rect.y);
        glVertex2i(rect.x + rect.width, rect.y);
        glVertex2i(rect.x + rect.width, rect.y + rect.height);
        glVertex2i(rect.x, rect.y + rect.height);
        glEnd();
    }

    void draw_line(const Point& start, const Point& end, Color color) override {
        glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glBegin(GL_LINES);
        glVertex2i(start.x, start.y);
        glVertex2i(end.x, end.y);
        glEnd();
    }

    void present() override {
        glXSwapBuffers(display_, window_);
    }

private:
    Display* display_;
    Window window_;
};

X11WindowBackend::X11WindowBackend() = default;

X11WindowBackend::~X11WindowBackend() {
    destroy();
}

bool X11WindowBackend::create(const Size& size, const char* title) {
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
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

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

    render_target_ = std::make_unique<OpenGLRenderTarget>(display_, window_);

    return true;
}

void X11WindowBackend::destroy() {
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

bool X11WindowBackend::poll_events() {
    if (!display_) return false;
    XEvent xev;
    while (XPending(display_)) {
        XNextEvent(display_, &xev);
        if (xev.type == ClientMessage) {
            if (static_cast<unsigned long>(xev.xclient.data.l[0]) == wm_delete_window_) {
                return false;
            }
        } else if (xev.type == DestroyNotify) {
            return false;
        }
    }
    return true;
}

IRenderTarget* X11WindowBackend::get_render_target() {
    return render_target_.get();
}

} // namespace ooey