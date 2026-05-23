#pragma once

#include "ooey/i_drawable.hpp"
#include "ooey/mvvmc/subscription_sink.hpp"
#include "ooey/mvvmc/property.hpp"
#include <vector>
#include <memory>

namespace ooey {

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

    void draw(IRenderTarget& target) const override;

private:
    std::vector<std::shared_ptr<IDrawable>> children_;
    SubscriptionSink sink_;
};

} // namespace ooey