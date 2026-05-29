#pragma once

#include "ooey/renderer/i_render_target.hpp"

namespace ooey {

class IDrawable {
public:
    virtual ~IDrawable() = default;
    virtual void draw(IRenderTarget& target) const = 0;
};

} // namespace ooey
