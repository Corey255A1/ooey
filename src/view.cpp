#include "ooey/view.hpp"

namespace ooey {

void View::add_child(std::shared_ptr<IDrawable>&& child) {
    children_.push_back(std::move(child));
}

const std::vector<std::shared_ptr<IDrawable>>& View::get_children() const {
    return children_;
}

void View::draw(IRenderTarget& target) const {
    for (const auto& child : children_) {
        child->draw(target);
    }
}

} // namespace ooey