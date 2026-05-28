#include "ooey/mvvmc/view.hpp"

namespace ooey::mvvmc {

void View::add_child(std::shared_ptr<IDrawable>&& child) {
    children_.push_back(std::move(child));
}

const std::vector<std::shared_ptr<IDrawable>>& View::get_children() const {
    return children_;
}

void View::draw(renderer::IRenderTarget& target) const {
    for (const auto& child : children_) {
        child->draw(target);
    }
}

void View::clear_children() {
    children_.clear();
}

} // namespace ooey::mvvmc