#pragma once

#include "ooey/types.hpp"

namespace ooey {

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    // Clears the target with the specified color
    virtual void clear(Color color) = 0;

    // Draws a filled rectangle
    virtual void draw_rect(const Rect& rect, Color color) = 0;

    // Draws a line between two points
    virtual void draw_line(const Point& start, const Point& end, Color color) = 0;

    // Swap the frame buffers or present the render output
    virtual void present() = 0;
};

} // namespace ooey
