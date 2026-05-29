#pragma once

namespace ooey {}


#include "gooey/mvvmc/scoped_subscription.hpp"
#include <vector>

namespace gooey::mvvmc {
    using namespace ooey;

// A helper container to hold multiple subscriptions, tying their lifetime
// to the object that owns this sink.
class SubscriptionSink {
public:
    void add(ScopedSubscription&& sub) {
        subscriptions_.push_back(std::move(sub));
    }

    void clear() {
        subscriptions_.clear();
    }

private:
    std::vector<ScopedSubscription> subscriptions_;
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::SubscriptionSink;
}
