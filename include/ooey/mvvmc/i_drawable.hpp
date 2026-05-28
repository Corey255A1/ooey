#pragma once

#include "ooey/renderer/i_render_target.hpp"

namespace ooey::mvvmc {

class IDrawable {
public:
    virtual ~IDrawable() = default;

    // Draw this drawable to the target
    virtual void draw(renderer::IRenderTarget& target) const = 0;
};

} // namespace ooey::mvvmc
namespace ooey {
using mvvmc::IDrawable;
}
