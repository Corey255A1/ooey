#include "ooey/view.hpp"

namespace ooey {

void View::add_child(std::shared_ptr<IDrawable>&& child) {
    children_.push_back(std::move(child));
}

const std::vector<std::shared_ptr<IDrawable>>& View::get_children() const {
    return children_;
}

std::vector<Geometry> View::generate_geometry() const {
    std::vector<Geometry> result;

    for (const auto& child : children_) {
        auto child_geos = child->generate_geometry();
        result.insert(result.end(), child_geos.begin(), child_geos.end());
    }

    return result;
}

} // namespace ooey