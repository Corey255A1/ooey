#pragma once

namespace ooey::mvvmc {

class IController {
public:
    virtual ~IController() = default;
    virtual void process_events() = 0;
};

} // namespace ooey::mvvmc

namespace ooey {
using mvvmc::IController;
}
