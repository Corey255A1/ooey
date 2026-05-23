#pragma once

#include "ooey/geometry.hpp"
#include <vector>

namespace ooey {

class IDrawable {
public:
    virtual ~IDrawable() = default;

    // Generate geometry for this drawable
    virtual std::vector<Geometry> generate_geometry() const = 0;
};

} // namespace ooey