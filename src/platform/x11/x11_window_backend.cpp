#include "ooey/platform/x11/x11_window_backend.hpp"
#include "ooey/i_render_target.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <iostream>

namespace ooey {

class OpenGLRenderTarget : public IRenderTarget {
public:
    OpenGLRenderTarget(Display* display, Window window)
        : display_(display), window_(window) {
        font_info_ = XLoadQueryFont(display_, "fixed");
        if (!font_info_) {
            font_info_ = XLoadQueryFont(display_, "-*-fixed-*-*-*-*-*-*-*-*-*-*-*");
        }

        if (font_info_) {
            font_base_ = glGenLists(256);
            if (font_base_ != 0) {
                glXUseXFont(font_info_->fid, 0, 256, font_base_);
            }
        }
    }

    ~OpenGLRenderTarget() override {
        if (font_base_) {
            glDeleteLists(font_base_, 256);
            font_base_ = 0;
        }
        if (font_info_) {
            XFreeFont(display_, font_info_);
            font_info_ = nullptr;
        }
    }

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

    void draw_geometry(const Geometry& geometry) override {
        if (geometry.vertices.empty()) return;

        if (geometry.type == PrimitiveType::Triangles) {
            glBegin(GL_TRIANGLES);
        } else {
            glBegin(GL_LINES);
        }

        for (unsigned int index : geometry.indices) {
            if (index < geometry.vertices.size()) {
                const auto& v = geometry.vertices[index];
                glColor4f(v.color.r / 255.0f, v.color.g / 255.0f, v.color.b / 255.0f, v.color.a / 255.0f);
                glVertex2f(v.x, v.y);
            }
        }
        glEnd();
    }

    Size measure_text(const std::string& text, const Font& font) override {
        return Size{static_cast<int>(text.length() * (font.size * 0.6)), font.size};
    }

    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override {
        if (text.empty()) return;

        if (font_base_ != 0) {
            glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
            glRasterPos2f(static_cast<GLfloat>(position.x), static_cast<GLfloat>(position.y + font.size));
            glListBase(font_base_);
            glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_BYTE,
                        reinterpret_cast<const GLubyte*>(text.c_str()));
            return;
        }

        Size size = measure_text(text, font);
        glBegin(GL_LINE_LOOP);
        glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glVertex2f(position.x, position.y);
        glVertex2f(position.x + size.width, position.y);
        glVertex2f(position.x + size.width, position.y + size.height);
        glVertex2f(position.x, position.y + size.height);
        glEnd();
    }

    void present() override {
        glXSwapBuffers(display_, window_);
    }

private:
    Display* display_;
    Window window_;
    XFontStruct* font_info_{nullptr};
    GLuint font_base_{0};
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
                    for(int i = 0; i < len; ++i) {
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

void X11WindowBackend::poll_input() {
    // Input is currently polled alongside window events in poll_events() for X11.
}

IRenderTarget* X11WindowBackend::get_render_target() {
    return render_target_.get();
}

} // namespace ooey