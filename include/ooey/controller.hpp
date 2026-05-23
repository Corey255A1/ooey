#pragma once

#include "ooey/input.hpp"
#include "ooey/view.hpp"
#include "ooey/i_interactive.hpp"
#include <memory>

namespace ooey {

class Controller {
public:
    Controller(InputManager& input_manager, std::shared_ptr<View> root_view);

    void process_events();

    void set_focused_element(IInteractive* element);

private:
    bool route_pointer_event(const Pointer& pointer, const std::shared_ptr<IDrawable>& node);

    InputManager& input_manager_;
    std::shared_ptr<View> root_view_;
    IInteractive* focused_element_{nullptr};
};

} // namespace ooey