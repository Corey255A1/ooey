#include "ooey/renderer/gl_render_target.hpp"
#include "ooey/renderer/bitmap_font.hpp"
#include "ooey/renderer/image.hpp"
#include <GL/gl.h>

namespace ooey {

GlRenderTarget::GlRenderTarget(int width, int height, std::function<void()>&& present_callback)
    : width_(width), height_(height), present_callback_(std::move(present_callback)) {
    // Enable blending for transparent primitives
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

GlRenderTarget::~GlRenderTarget() {
    for (auto const& [img, tex_id] : texture_cache_) {
        glDeleteTextures(1, &tex_id);
    }
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

void GlRenderTarget::draw_image(const Image& image, const Rect& dest_rect) {
    unsigned int tex_id = 0;
    auto it = texture_cache_.find(&image);
    if (it != texture_cache_.end()) {
        tex_id = it->second;
    } else {
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Map edge behavior to avoid border artifacts
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F /* GL_CLAMP_TO_EDGE */);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F /* GL_CLAMP_TO_EDGE */);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data().data());
        texture_cache_[&image] = tex_id;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(static_cast<float>(dest_rect.x), static_cast<float>(dest_rect.y));
    glTexCoord2f(1.0f, 0.0f); glVertex2f(static_cast<float>(dest_rect.x + dest_rect.width), static_cast<float>(dest_rect.y));
    glTexCoord2f(1.0f, 1.0f); glVertex2f(static_cast<float>(dest_rect.x + dest_rect.width), static_cast<float>(dest_rect.y + dest_rect.height));
    glTexCoord2f(0.0f, 1.0f); glVertex2f(static_cast<float>(dest_rect.x), static_cast<float>(dest_rect.y + dest_rect.height));
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

} // namespace ooey
