#pragma once

#include <memory>

namespace ooey {

class IWindowBackend;

// Dynamically creates the most appropriate window backend for the current environment.
std::unique_ptr<IWindowBackend> create_default_window_backend();

} // namespace ooey
