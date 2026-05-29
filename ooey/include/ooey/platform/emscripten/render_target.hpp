#pragma once

#include "ooey/renderer/i_render_target.hpp"
#include "ooey/types.hpp"

namespace ooey::emscripten {

class RenderTarget : public renderer::IRenderTarget {
public:
    RenderTarget(int width, int height);
    ~RenderTarget() override = default;

    void clear(Color color) override;
    void draw_geometry(const renderer::Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

private:
    int width_{0};
    int height_{0};
};

} // namespace ooey::emscripten
