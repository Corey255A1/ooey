#pragma once

#include <functional>
#include <map>
#include <cstdint>

namespace ooey {

template <typename T>
class Property {
public:
    using Listener = std::function<void(const T&)>;

    Property() = default;
    Property(T initial_value) : value_(std::move(initial_value)) {}

    // Subscribes a listener and immediately calls it with the current value.
    // Returns a unique subscription ID that can be used to unsubscribe.
    uint32_t subscribe(Listener listener) {
        uint32_t id = next_id_++;
        listeners_[id] = std::move(listener);
        listeners_[id](value_); // Initial sync
        return id;
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
};

} // namespace ooey