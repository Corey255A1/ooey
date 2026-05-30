#pragma once

#include "ooey/types.hpp"

namespace gooey {

enum class SizePolicy {
    Fixed,
    WrapContent,
    MatchParent
};

struct LayoutLength {
    SizePolicy policy{SizePolicy::WrapContent};
    float value{0.0f};

    constexpr LayoutLength() = default;
    constexpr LayoutLength(SizePolicy policy, float value = 0.0f) : policy(policy), value(value) {}
};

enum class Align {
    Start,
    Center,
    End,
    Stretch
};

} // namespace gooey
