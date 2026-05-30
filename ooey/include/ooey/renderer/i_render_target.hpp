#pragma once

#include "ooey/types.hpp"
#include "ooey/renderer/geometry.hpp"
#include <string>

namespace ooey::renderer {

class Image;

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    // Clears the target with the specified color
    virtual void clear(Color color) = 0;

    // Draw generic geometry
    virtual void draw_geometry(const Geometry& geometry) = 0;

    // Draw an image
    virtual void draw_image(const Image& image, const Rect& dest_rect) = 0;

    // Text functions
    virtual Size measure_text(const std::string& text, const Font& font) = 0;
    virtual void draw_text(const std::string& text, const Font& font, const Point& position, Color color) = 0;

    // Resize the render target
    virtual void resize(int width, int height) {}

    // Swap the frame buffers or present the render output
    virtual void present() = 0;
};

} // namespace ooey::renderer

namespace ooey {
using renderer::IRenderTarget;
using renderer::Image;
}
