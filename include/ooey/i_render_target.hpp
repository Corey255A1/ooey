#pragma once

#include "ooey/types.hpp"
#include "ooey/geometry.hpp"

namespace ooey {

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    // Clears the target with the specified color
    virtual void clear(Color color) = 0;

    // Draw generic geometry
    virtual void draw_geometry(const Geometry& geometry) = 0;

    // Swap the frame buffers or present the render output
    virtual void present() = 0;
};

} // namespace ooey
