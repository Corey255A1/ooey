#pragma once

namespace ooey {}


namespace gooey::mvvmc {
    using namespace ooey;

class IController {
public:
    virtual ~IController() = default;
    virtual void process_events() = 0;
};

} // namespace gooey::mvvmc

namespace gooey {
    using namespace ooey;
using gooey::mvvmc::IController;
}
