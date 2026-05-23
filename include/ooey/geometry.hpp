#pragma once

#include "ooey/types.hpp"
#include <vector>

namespace ooey {

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

} // namespace ooey