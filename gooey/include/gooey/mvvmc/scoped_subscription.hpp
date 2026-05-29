#pragma once

namespace ooey {}


#include <functional>

namespace gooey::mvvmc {
    using namespace ooey;

class ScopedSubscription {
public:
    ScopedSubscription() = default;
    explicit ScopedSubscription(std::function<void()> unsubscribe_func)
        : unsubscribe_func_(std::move(unsubscribe_func)) {}

    ~ScopedSubscription() {
        if (unsubscribe_func_) {
            unsubscribe_func_();
        }
    }

    // Move-only semantics
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;

    ScopedSubscription(ScopedSubscription&& other) noexcept
        : unsubscribe_func_(std::move(other.unsubscribe_func_)) {
        other.unsubscribe_func_ = nullptr;
    }

    ScopedSubscription& operator=(ScopedSubscription&& other) noexcept {
        if (this != &other) {
            if (unsubscribe_func_) unsubscribe_func_();
            unsubscribe_func_ = std::move(other.unsubscribe_func_);
            other.unsubscribe_func_ = nullptr;
        }
        return *this;
    }

private:
    std::function<void()> unsubscribe_func_;
};

} // namespace gooey::mvvmc