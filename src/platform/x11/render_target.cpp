#include "ooey/platform/x11/render_target.hpp"
#include "ooey/bitmap_font.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <iostream>

namespace ooey::x11 {

RenderTarget::RenderTarget(Display* display, unsigned long window)
    : display_(display), window_(window) {
    font_info_ = XLoadQueryFont(display_, "fixed");
    if (!font_info_) {
        font_info_ = XLoadQueryFont(display_, "-*-fixed-*-*-*-*-*-*-*-*-*-*-*");
    }

    if (font_info_) {
        XFontStruct* font = static_cast<XFontStruct*>(font_info_);
        font_base_ = glGenLists(256);
        if (font_base_ != 0) {
            glXUseXFont(font->fid, 0, 256, font_base_);
        }
    }
}

RenderTarget::~RenderTarget() {
    if (font_base_) {
        glDeleteLists(font_base_, 256);
        font_base_ = 0;
    }
    if (font_info_) {
        XFreeFont(display_, static_cast<XFontStruct*>(font_info_));
        font_info_ = nullptr;
    }
}

void RenderTarget::clear(Color color) {
    XWindowAttributes window_attributes;
    XGetWindowAttributes(display_, window_, &window_attributes);
    glViewport(0, 0, window_attributes.width, window_attributes.height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_attributes.width, window_attributes.height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderTarget::draw_geometry(const Geometry& geometry) {
    if (geometry.vertices.empty()) {
        return;
    }

    if (geometry.type == PrimitiveType::Triangles) {
        glBegin(GL_TRIANGLES);
    } else {
        glBegin(GL_LINES);
    }

    for (unsigned int index : geometry.indices) {
        if (index < geometry.vertices.size()) {
            const auto& vertex = geometry.vertices[index];
            glColor4f(vertex.color.r / 255.0f, vertex.color.g / 255.0f, vertex.color.b / 255.0f, vertex.color.a / 255.0f);
            glVertex2f(vertex.x, vertex.y);
        }
    }
    glEnd();
}

Size RenderTarget::measure_text(const std::string& text, const Font& font) {
    if (font_info_) {
        XFontStruct* font_struct = static_cast<XFontStruct*>(font_info_);
        int width = XTextWidth(font_struct, text.data(), static_cast<int>(text.length()));
        int height = font_struct->ascent + font_struct->descent;
        return Size{width, height};
    }
    return BitmapFont::measure_text(text, font.size);
}

void RenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    if (text.empty()) {
        return;
    }

    if (font_base_ != 0) {
        glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glRasterPos2f(static_cast<GLfloat>(position.x), static_cast<GLfloat>(position.y + font.size));
        glListBase(font_base_);
        glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_BYTE,
                    reinterpret_cast<const GLubyte*>(text.c_str()));
        return;
    }

    // Fallback to BitmapFont
    BitmapFont::draw_text(text, font.size, position, [color](int x, int y, int w, int h) {
        glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
    });
}

void RenderTarget::present() {
    glXSwapBuffers(display_, window_);
}

} // namespace ooey::x11
