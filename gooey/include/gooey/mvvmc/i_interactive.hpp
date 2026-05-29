#pragma once

namespace ooey {}


#include "ooey/types.hpp"
#include "ooey/input.hpp"

namespace gooey::mvvmc {
    using namespace ooey;

class IInteractive {
public:
    virtual ~IInteractive() = default;

    // Defines the interactive physical space
    virtual Rect bounds() const = 0;

    // Event Handlers (return true if consumed)
    virtual bool on_pointer_event(const Pointer& e) = 0;
    virtual bool on_key_event(const KeyEvent& e) = 0;
    virtual bool on_text_event(const TextEvent& e) { return false; }
};

} // namespace gooey::mvvmc
namespace gooey {
    using namespace ooey;
using gooey::mvvmc::IInteractive;
}
