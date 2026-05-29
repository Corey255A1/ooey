#pragma once

#include "ooey/renderer/i_render_target.hpp"
#include <functional>

namespace ooey {

class GlRenderTarget : public IRenderTarget {
public:
    GlRenderTarget(int width, int height, std::function<void()>&& present_callback = nullptr);
    ~GlRenderTarget() override = default;

    void resize(int width, int height);

    // IRenderTarget implementation
    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

protected:
    int width_{0};
    int height_{0};
    std::function<void()> present_callback_;
};

} // namespace ooey
