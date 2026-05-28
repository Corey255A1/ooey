#pragma once

#include "ooey/mvvmc/i_controller.hpp"
#include "ooey/input.hpp"
#include "ooey/mvvmc/view.hpp"
#include "ooey/mvvmc/i_interactive.hpp"
#include <memory>

namespace ooey::mvvmc {

class Controller : public IController {
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

} // namespace ooey::mvvmc
namespace ooey {
using mvvmc::Controller;
}
