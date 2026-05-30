#include "win32_font_backend.hpp"
#include <iostream>

namespace ooey {

struct Win32FontBackend::Impl {
#ifdef _WIN32
    // Windows DirectWrite/GDI font structures would be defined here
#endif
};

Win32FontBackend::Win32FontBackend() : impl_(std::make_unique<Impl>()) {}
Win32FontBackend::~Win32FontBackend() = default;

bool Win32FontBackend::initialize() {
#ifdef _WIN32
    // Initialize DirectWrite or GDI factories
    return true;
#else
    return false; // DirectWrite is not available on non-Windows platforms
#endif
}

bool Win32FontBackend::load_font(const Font& /*font*/) {
#ifdef _WIN32
    // Match and cache the system font via GDI/DirectWrite
    return true;
#else
    return false;
#endif
}

Size Win32FontBackend::measure_text(const std::string& /*text*/, const Font& /*font*/) {
    return Size{0, 0};
}

void Win32FontBackend::draw_text(const std::string& /*text*/, const Font& /*font*/, const Point& /*position*/, const DrawCallback& /*callback*/) {
}

std::vector<std::string> Win32FontBackend::get_available_fonts() {
#ifdef _WIN32
    // Query system font collection (e.g. IDWriteFontCollection)
    return {"Arial", "Times New Roman", "Courier New", "Segoe UI", "Calibri", "Consolas"};
#else
    return {};
#endif
}

} // namespace ooey
