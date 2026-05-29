#pragma once

namespace ooey {}


#include <functional>
#include <map>
#include <cstdint>
#include <memory>
#include "gooey/mvvmc/scoped_subscription.hpp"

namespace gooey::mvvmc {
    using namespace ooey;

template <typename T>
class Property {
public:
    using Listener = std::function<void(const T&)>;

    Property() : alive_flag_(std::make_shared<bool>(true)) {}
    Property(T initial_value) : value_(std::move(initial_value)), alive_flag_(std::make_shared<bool>(true)) {}

    ~Property() {
        if (alive_flag_) {
            *alive_flag_ = false;
        }
    }

    // Properties cannot be moved or copied because their memory address 
    // is captured by active subscriptions.
    Property(const Property&) = delete;
    Property& operator=(const Property&) = delete;
    Property(Property&&) = delete;
    Property& operator=(Property&&) = delete;

    // Subscribes a listener and immediately calls it with the current value.
    // Returns an RAII ScopedSubscription that automatically unsubscribes when destroyed.
    ScopedSubscription subscribe(Listener listener) {
        uint32_t id = next_id_++;
        listeners_[id] = std::move(listener);
        listeners_[id](value_); // Initial sync

        std::weak_ptr<bool> weak_alive = alive_flag_;
        return ScopedSubscription([this, id, weak_alive]() {
            if (auto alive = weak_alive.lock()) {
                if (*alive) {
                    this->unsubscribe(id);
                }
            }
        });
    }

    void unsubscribe(uint32_t id) {
        listeners_.erase(id);
    }

    void set(T new_value) {
        value_ = std::move(new_value);
        notify();
    }

    const T& get() const {
        return value_;
    }

private:
    void notify() {
        for (auto& kv : listeners_) {
            kv.second(value_);
        }
    }

    T value_;
    std::map<uint32_t, Listener> listeners_;
    uint32_t next_id_{0};
    std::shared_ptr<bool> alive_flag_;
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::Property;
using gooey::mvvmc::ScopedSubscription;
}
