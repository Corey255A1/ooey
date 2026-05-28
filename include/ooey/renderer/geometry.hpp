#pragma once

#include "ooey/types.hpp"
#include <vector>

namespace ooey::renderer {

enum class PrimitiveType {
    Triangles,
    Lines
};

struct Vertex {
    float x;
    float y;
    Color color;
};

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    PrimitiveType type{PrimitiveType::Triangles};
};

} // namespace ooey::renderer
namespace ooey {
using renderer::Vertex;
using renderer::Geometry;
using renderer::PrimitiveType;
}
