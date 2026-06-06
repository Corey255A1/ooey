#include "tooey/hit_tester.hpp"
#include "gooey/mvvmc/gooey_node.hpp"

namespace tooey {

std::shared_ptr<gooey::mvvmc::GooeyElement> HitTester::hit_test(
    const std::shared_ptr<gooey::mvvmc::GooeyElement>& element, 
    int x, int y) {
    
    if (!element) return nullptr;
    
    bool hit = (x >= element->layout_bounds.x && x <= element->layout_bounds.x + element->layout_bounds.width &&
                y >= element->layout_bounds.y && y <= element->layout_bounds.y + element->layout_bounds.height);
    if (!hit) return nullptr;
    
    auto node = std::dynamic_pointer_cast<gooey::mvvmc::GooeyNode>(element);
    if (node) {
        for (const auto& child : node->get_children()) {
            auto child_el = std::dynamic_pointer_cast<gooey::mvvmc::GooeyElement>(child);
            auto child_hit = hit_test(child_el, x, y);
            if (child_hit) return child_hit;
        }
    }
    return element;
}

} // namespace tooey
