#include "ooey/renderer/gl_render_target.hpp"
#include "ooey/renderer/font_engine.hpp"
#include "ooey/renderer/glyph_atlas.hpp"
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
    clip_stack_.clear();
    glViewport(0, 0, width_, height_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width_, height_, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Disable scissor temporarily for clear
    bool scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);
    if (scissor_enabled) {
        glDisable(GL_SCISSOR_TEST);
    }

    glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (scissor_enabled) {
        glEnable(GL_SCISSOR_TEST);
    }
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

void GlRenderTarget::draw_geometry(const Geometry& geometry, const void* /*cache_key*/, bool /*is_dirty*/) {
    draw_geometry(geometry);
}

Size GlRenderTarget::measure_text(const std::string& text, const Font& font) {
    return FontEngine::measure_text(text, font);
}

void GlRenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    if (text.empty()) {
        return;
    }
    auto atlas = FontEngine::get_glyph_atlas(font);
    if (!atlas || !atlas->get_image()) {
        return;
    }
    const Image& image = *atlas->get_image();
    unsigned int tex_id = 0;
    auto it = texture_cache_.find(&image);
    if (it != texture_cache_.end()) {
        tex_id = it->second;
    } else {
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F /* GL_CLAMP_TO_EDGE */);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F /* GL_CLAMP_TO_EDGE */);
#ifdef __EMSCRIPTEN__
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data().data());
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data().data());
#endif
        texture_cache_[&image] = tex_id;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

    float inv_w = 1.0f / image.width();
    float inv_h = 1.0f / image.height();

    int pen_x = position.x;
    int pen_y = position.y;

    glBegin(GL_QUADS);
    for (char c : text) {
        GlyphMetrics metrics;
        if (atlas->get_glyph_metrics(c, metrics)) {
            if (metrics.width > 0 && metrics.height > 0) {
                float x0 = static_cast<float>(pen_x + metrics.offset_x);
                float y0 = static_cast<float>(pen_y + metrics.offset_y);
                float x1 = x0 + metrics.width;
                float y1 = y0 + metrics.height;

                float u0 = metrics.x * inv_w;
                float v0 = metrics.y * inv_h;
                float u1 = (metrics.x + metrics.width) * inv_w;
                float v1 = (metrics.y + metrics.height) * inv_h;

                glTexCoord2f(u0, v0); glVertex2f(x0, y0);
                glTexCoord2f(u1, v0); glVertex2f(x1, y0);
                glTexCoord2f(u1, v1); glVertex2f(x1, y1);
                glTexCoord2f(u0, v1); glVertex2f(x0, y1);
            }
            pen_x += metrics.advance;
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
}


void GlRenderTarget::push_clip(const Rect& rect) {
    Rect current = rect;
    if (!clip_stack_.empty()) {
        const Rect& parent = clip_stack_.back();
        int x1 = std::max(parent.x, rect.x);
        int y1 = std::max(parent.y, rect.y);
        int x2 = std::min(parent.x + parent.width, rect.x + rect.width);
        int y2 = std::min(parent.y + parent.height, rect.y + rect.height);
        int w = std::max(0, x2 - x1);
        int h = std::max(0, y2 - y1);
        current = Rect{x1, y1, w, h};
    }
    clip_stack_.push_back(current);

    glEnable(GL_SCISSOR_TEST);
    // Translate top-left y boundary to bottom-left y boundary
    glScissor(current.x, height_ - (current.y + current.height), current.width, current.height);
}

void GlRenderTarget::pop_clip() {
    if (!clip_stack_.empty()) {
        clip_stack_.pop_back();
    }
    if (clip_stack_.empty()) {
        glDisable(GL_SCISSOR_TEST);
    } else {
        const Rect& current = clip_stack_.back();
        glScissor(current.x, height_ - (current.y + current.height), current.width, current.height);
    }
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
#ifdef __EMSCRIPTEN__
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data().data());
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data().data());
#endif
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
