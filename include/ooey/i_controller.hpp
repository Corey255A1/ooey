#pragma once

namespace ooey {

class IController {
public:
    virtual ~IController() = default;
    virtual void process_events() = 0;
};

} // namespace ooey
