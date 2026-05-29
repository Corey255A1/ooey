#pragma once

namespace ooey {}


#include "gooey/mvvmc/i_drawable.hpp"
#include "gooey/mvvmc/subscription_sink.hpp"
#include "gooey/mvvmc/property.hpp"
#include <vector>
#include <memory>

namespace gooey::mvvmc {
    using namespace ooey;

class View : public IDrawable {
public:
    View() = default;

    void add_child(std::shared_ptr<IDrawable>&& child);

    const std::vector<std::shared_ptr<IDrawable>>& get_children() const;

    // Helper to easily bind to a property and manage its lifecycle
    template <typename T>
    void bind(Property<T>& property, typename Property<T>::Listener listener) {
        sink_.add(property.subscribe(std::move(listener)));
    }

    void draw(ooey::IRenderTarget& target) const override;
    void clear_children();

private:
    std::vector<std::shared_ptr<IDrawable>> children_;
    SubscriptionSink sink_;
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::View;
}
