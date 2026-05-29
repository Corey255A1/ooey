#pragma once

namespace ooey {}


#include "ooey/renderer/i_render_target.hpp"

namespace gooey::mvvmc {
    using namespace ooey;

class IDrawable {
public:
    virtual ~IDrawable() = default;

    // Draw this drawable to the target
    virtual void draw(ooey::IRenderTarget& target) const = 0;
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::IDrawable;
}
