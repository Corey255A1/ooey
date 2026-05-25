#pragma once

#include "ooey/i_render_target.hpp"

typedef struct _XDisplay Display;

namespace ooey::x11 {

class RenderTarget : public IRenderTarget {
public:
    RenderTarget(Display* display, unsigned long window);
    ~RenderTarget() override;

    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

private:
    Display* display_;
    unsigned long window_;
    void* font_info_{nullptr};
    unsigned int font_base_{0};
};

} // namespace ooey::x11
