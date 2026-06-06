#pragma once

#include "ooey/types.hpp"

namespace gooey {

enum class SizePolicy {
    Fixed,
    WrapContent,
    MatchParent,
    Flex,
    Percentage
};

struct LayoutLength {
    SizePolicy policy{SizePolicy::WrapContent};
    float value{0.0f};

    constexpr LayoutLength() = default;
    constexpr LayoutLength(SizePolicy policy, float value = 0.0f) : policy(policy), value(value) {}
};

enum class Align {
    Inherit,
    Start,
    Center,
    End,
    Stretch
};

enum class Justify {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

} // namespace gooey
