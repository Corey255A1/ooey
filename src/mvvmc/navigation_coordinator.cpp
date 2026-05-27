#include "ooey/mvvmc/navigation_coordinator.hpp"

namespace ooey {

void NavigationCoordinator::navigate_to(std::shared_ptr<PageViewModelBase> vm) {
    if (!vm) {
        return;
    }

    // Erase forward history if we are currently somewhere in the middle of history
    if (current_index_ >= 0 && current_index_ < static_cast<int>(history_.size()) - 1) {
        history_.erase(history_.begin() + current_index_ + 1, history_.end());
    }

    history_.push_back(vm);
    current_index_ = static_cast<int>(history_.size()) - 1;
    current_viewmodel.set(vm);
    update_states();
}

void NavigationCoordinator::go_back() {
    if (current_index_ > 0) {
        current_index_--;
        current_viewmodel.set(history_[current_index_]);
        update_states();
    }
}

void NavigationCoordinator::go_forward() {
    if (current_index_ < static_cast<int>(history_.size()) - 1) {
        current_index_++;
        current_viewmodel.set(history_[current_index_]);
        update_states();
    }
}

const std::vector<std::shared_ptr<PageViewModelBase>>& NavigationCoordinator::get_history() const {
    return history_;
}

void NavigationCoordinator::update_states() {
    can_go_back.set(current_index_ > 0);
    can_go_forward.set(current_index_ < static_cast<int>(history_.size()) - 1);
}

} // namespace ooey
