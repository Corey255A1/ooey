#pragma once

#include "gooey/mvvmc/gooey_element.hpp"
#include <vector>
#include <memory>

namespace gooey::mvvmc {

class GooeyNode : public GooeyElement {
public:
    GooeyNode();
    virtual ~GooeyNode() override = default;

    virtual void add_child(std::shared_ptr<IDrawable>&& child);
    void remove_child(const std::shared_ptr<IDrawable>& child);
    const std::vector<std::shared_ptr<IDrawable>>& get_children() const;
    void clear_children();

    void draw(ooey::IRenderTarget& target) const override;

    bool clip_children{false};
    int spacing_{0};

    // Builder setters for chaining configuration
    GooeyNode& set_clip_children(bool clip) { clip_children = clip; return *this; }
    GooeyNode& set_spacing(int spacing) { spacing_ = spacing; return *this; }

    void set_theme_manager(std::shared_ptr<ThemeManager> manager) override;

protected:
    Size do_measure(Size constraints) override;
    void do_layout(Rect bounds) override;

private:
    int calculate_content_width(Size child_constraints);
    int calculate_content_height(Size child_constraints);

    std::vector<std::shared_ptr<IDrawable>> children_;
};

} // namespace gooey::mvvmc
namespace gooey {
using gooey::mvvmc::GooeyNode;
}
