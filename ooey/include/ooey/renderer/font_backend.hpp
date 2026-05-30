#pragma once

#include "ooey/types.hpp"
#include <string>
#include <vector>
#include <functional>

namespace ooey {

class IFontBackend {
public:
    virtual ~IFontBackend() = default;

    // Initializes the backend (e.g., loads dynamic library symbols).
    virtual bool initialize() = 0;

    // Checks if a font is available and preloads it.
    virtual bool load_font(const Font& font) = 0;

    // Measures the bounding box width and height of a string of text.
    virtual Size measure_text(const std::string& text, const Font& font) = 0;

    // Rasterizes text and calls a callback for each pixel/sub-rect block.
    // The callback signature: void(int x, int y, int w, int h, uint8_t alpha)
    using DrawCallback = std::function<void(int x, int y, int w, int h, uint8_t alpha)>;
    virtual void draw_text(const std::string& text, const Font& font, const Point& position, const DrawCallback& callback) = 0;

    // Returns a list of available font family names on the OS.
    virtual std::vector<std::string> get_available_fonts() = 0;
};

} // namespace ooey
