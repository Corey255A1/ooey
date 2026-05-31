#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace ooey {

class IWindowBackend;

// Dynamically creates the most appropriate window backend for the current environment.
std::unique_ptr<IWindowBackend> create_default_window_backend();

// Cross-platform asset/file reader.
// On Android: attempts to read from APK assets first, then falls back to regular filesystem.
// On other platforms: reads from regular filesystem.
std::vector<uint8_t> read_asset(const std::string& path);

} // namespace ooey
