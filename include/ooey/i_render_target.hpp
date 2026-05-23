#pragma once

#include "ooey/types.hpp"
#include "ooey/geometry.hpp"
#include <string>

namespace ooey {

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    // Clears the target with the specified color
    virtual void clear(Color color) = 0;

    // Draw generic geometry
    virtual void draw_geometry(const Geometry& geometry) = 0;

    // Text functions
    virtual Size measure_text(const std::string& text, const Font& font) = 0;
    virtual void draw_text(const std::string& text, const Font& font, const Point& position, Color color) = 0;

    // Swap the frame buffers or present the render output
    virtual void present() = 0;
};

} // namespace ooey
