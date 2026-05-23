#pragma once

#include "ooey/i_render_target.hpp"

namespace ooey {

class IDrawable {
public:
    virtual ~IDrawable() = default;

    // Draw this drawable to the target
    virtual void draw(IRenderTarget& target) const = 0;
};

} // namespace ooey