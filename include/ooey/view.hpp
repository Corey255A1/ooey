#pragma once

#include "ooey/i_drawable.hpp"
#include <vector>
#include <memory>

namespace ooey {

class View : public IDrawable {
public:
    View() = default;

    void add_child(std::shared_ptr<IDrawable>&& child);

    const std::vector<std::shared_ptr<IDrawable>>& get_children() const;

    std::vector<Geometry> generate_geometry() const override;

private:
    std::vector<std::shared_ptr<IDrawable>> children_;
};

} // namespace ooey