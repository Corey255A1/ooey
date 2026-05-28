#pragma once

#include "ooey/mvvmc/scoped_subscription.hpp"
#include <vector>

namespace ooey::mvvmc {

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

} // namespace ooey::mvvmc
namespace ooey {
using mvvmc::SubscriptionSink;
}
