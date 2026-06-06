#pragma once

#include "gooey/mvvmc/gooey_element.hpp"
#include <memory>

namespace tooey {

class HitTester {
public:
    static std::shared_ptr<gooey::mvvmc::GooeyElement> hit_test(
        const std::shared_ptr<gooey::mvvmc::GooeyElement>& element, 
        int x, int y);
};

} // namespace tooey
