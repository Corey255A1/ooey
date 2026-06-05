#pragma once

namespace ooey {}


#include "gooey/mvvmc/i_controller.hpp"
#include "ooey/input.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/i_interactive.hpp"
#include <memory>

namespace gooey::mvvmc {
    using namespace ooey;

class Controller : public IController {
public:
    Controller(InputManager& input_manager, std::shared_ptr<GooeyNode> root_view);

    void process_events();

    void set_focused_element(std::shared_ptr<IDrawable> element);
    std::shared_ptr<IDrawable> get_focused_element() const { return focused_element_; }

    void set_captured_element(std::shared_ptr<IDrawable> element) { captured_element_ = std::move(element); }
    std::shared_ptr<IDrawable> get_captured_element() const { return captured_element_; }

private:
    bool route_pointer_event(const Pointer& pointer, const std::shared_ptr<IDrawable>& node);

    InputManager& input_manager_;
    std::shared_ptr<GooeyNode> root_view_;
    std::shared_ptr<IDrawable> focused_element_{nullptr};
    std::shared_ptr<IDrawable> captured_element_{nullptr};
    int pointer_pressed_x_{0};
    int pointer_pressed_y_{0};
    bool captured_element_stolen_{false};
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::Controller;
}
