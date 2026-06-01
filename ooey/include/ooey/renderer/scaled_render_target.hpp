#pragma once

#include "ooey/renderer/i_render_target.hpp"

namespace ooey::renderer {

class ScaledRenderTarget : public IRenderTarget {
public:
    ScaledRenderTarget(IRenderTarget* wrapped, float scale)
        : wrapped_(wrapped), scale_(scale) {}

    void clear(Color color) override {
        wrapped_->clear(color);
    }

    void draw_geometry(const Geometry& geometry) override {
        if (scale_ == 1.0f) {
            wrapped_->draw_geometry(geometry);
            return;
        }
        Geometry scaled_geom = geometry;
        for (auto& v : scaled_geom.vertices) {
            v.x *= scale_;
            v.y *= scale_;
        }
        wrapped_->draw_geometry(scaled_geom);
    }

    void draw_geometry(const Geometry& geometry, const void* cache_key, bool is_dirty) override {
        if (scale_ == 1.0f) {
            wrapped_->draw_geometry(geometry, cache_key, is_dirty);
            return;
        }
        Geometry scaled_geom = geometry;
        for (auto& v : scaled_geom.vertices) {
            v.x *= scale_;
            v.y *= scale_;
        }
        wrapped_->draw_geometry(scaled_geom, cache_key, is_dirty);
    }

    void draw_image(const Image& image, const Rect& dest_rect) override {
        if (scale_ == 1.0f) {
            wrapped_->draw_image(image, dest_rect);
            return;
        }
        Rect scaled_rect{
            static_cast<int>(dest_rect.x * scale_),
            static_cast<int>(dest_rect.y * scale_),
            static_cast<int>(dest_rect.width * scale_),
            static_cast<int>(dest_rect.height * scale_)
        };
        wrapped_->draw_image(image, scaled_rect);
    }

    Size measure_text(const std::string& text, const Font& font) override {
        if (scale_ == 1.0f) {
            return wrapped_->measure_text(text, font);
        }
        Font scaled_font = font;
        scaled_font.size = static_cast<int>(font.size * scale_);
        Size physical_size = wrapped_->measure_text(text, scaled_font);
        return Size{
            static_cast<int>(physical_size.width / scale_),
            static_cast<int>(physical_size.height / scale_)
        };
    }

    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override {
        if (scale_ == 1.0f) {
            wrapped_->draw_text(text, font, position, color);
            return;
        }
        Font scaled_font = font;
        scaled_font.size = static_cast<int>(font.size * scale_);
        Point scaled_pos{
            static_cast<int>(position.x * scale_),
            static_cast<int>(position.y * scale_)
        };
        wrapped_->draw_text(text, scaled_font, scaled_pos, color);
    }

    void resize(int width, int height) override {
        wrapped_->resize(width, height);
    }

    void present() override {
        wrapped_->present();
    }

    void push_clip(const Rect& rect) override {
        if (scale_ == 1.0f) {
            wrapped_->push_clip(rect);
            return;
        }
        Rect scaled_rect{
            static_cast<int>(rect.x * scale_),
            static_cast<int>(rect.y * scale_),
            static_cast<int>(rect.width * scale_),
            static_cast<int>(rect.height * scale_)
        };
        wrapped_->push_clip(scaled_rect);
    }

    void pop_clip() override {
        wrapped_->pop_clip();
    }

private:
    IRenderTarget* wrapped_;
    float scale_;
};

} // namespace ooey::renderer
namespace ooey {
using renderer::ScaledRenderTarget;
}
