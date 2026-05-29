#include "ooey/renderer/gl_render_target.hpp"
#include "ooey/renderer/bitmap_font.hpp"
#include <GL/gl.h>

namespace ooey {

GlRenderTarget::GlRenderTarget(int width, int height, std::function<void()>&& present_callback)
    : width_(width), height_(height), present_callback_(std::move(present_callback)) {
    // Enable blending for transparent primitives
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GlRenderTarget::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void GlRenderTarget::clear(Color color) {
    glViewport(0, 0, width_, height_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width_, height_, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GlRenderTarget::draw_geometry(const Geometry& geometry) {
    if (geometry.vertices.empty()) {
        return;
    }

    if (geometry.type == PrimitiveType::Triangles) {
        glBegin(GL_TRIANGLES);
    } else if (geometry.type == PrimitiveType::Lines) {
        glBegin(GL_LINES);
    } else {
        return;
    }

    if (!geometry.indices.empty()) {
        for (unsigned int idx : geometry.indices) {
            if (idx < geometry.vertices.size()) {
                const auto& vertex = geometry.vertices[idx];
                glColor4f(vertex.color.r / 255.0f, vertex.color.g / 255.0f, vertex.color.b / 255.0f, vertex.color.a / 255.0f);
                glVertex2f(vertex.x, vertex.y);
            }
        }
    } else {
        for (const auto& vertex : geometry.vertices) {
            glColor4f(vertex.color.r / 255.0f, vertex.color.g / 255.0f, vertex.color.b / 255.0f, vertex.color.a / 255.0f);
            glVertex2f(vertex.x, vertex.y);
        }
    }

    glEnd();
}

Size GlRenderTarget::measure_text(const std::string& text, const Font& font) {
    return BitmapFont::measure_text(text, font.size);
}

void GlRenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    BitmapFont::draw_text(text, font.size, position, [color](int x, int y, int w, int h) {
        glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glBegin(GL_QUADS);
        glVertex2f(static_cast<float>(x), static_cast<float>(y));
        glVertex2f(static_cast<float>(x + w), static_cast<float>(y));
        glVertex2f(static_cast<float>(x + w), static_cast<float>(y + h));
        glVertex2f(static_cast<float>(x), static_cast<float>(y + h));
        glEnd();
    });
}

void GlRenderTarget::present() {
    if (present_callback_) {
        present_callback_();
    }
}

} // namespace ooey
