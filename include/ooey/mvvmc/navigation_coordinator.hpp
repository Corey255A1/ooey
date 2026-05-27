#pragma once

#include "ooey/mvvmc/property.hpp"
#include <vector>
#include <memory>
#include <string>

namespace ooey {

class PageViewModelBase {
public:
    virtual ~PageViewModelBase() = default;
    virtual std::string get_title() const = 0;
    virtual void update(float /*dt*/) {}
};

class NavigationCoordinator {
public:
    NavigationCoordinator() = default;

    // Properties for data binding
    Property<std::shared_ptr<PageViewModelBase>> current_viewmodel{nullptr};
    Property<bool> can_go_back{false};
    Property<bool> can_go_forward{false};

    // Navigation actions
    void navigate_to(std::shared_ptr<PageViewModelBase> vm);
    void go_back();
    void go_forward();

    const std::vector<std::shared_ptr<PageViewModelBase>>& get_history() const;

private:
    void update_states();

    std::vector<std::shared_ptr<PageViewModelBase>> history_;
    int current_index_{-1};
};

} // namespace ooey
