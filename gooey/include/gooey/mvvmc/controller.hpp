#pragma once

namespace ooey {}


#include "gooey/mvvmc/i_controller.hpp"
#include "ooey/input.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include <memory>

namespace gooey::mvvmc {
    using namespace ooey;

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

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::Controller;
}
