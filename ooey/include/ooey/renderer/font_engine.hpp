#pragma once

#include "ooey/types.hpp"
#include "ooey/renderer/font_backend.hpp"
#include "ooey/renderer/bitmap_font.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace ooey {

class FontEngine {
public:
    // Explicitly sets the platform-specific font backend.
    static void set_backend(std::unique_ptr<IFontBackend>&& backend);

    // Measures the text dimensions using the active backend or falls back to BitmapFont.
    static Size measure_text(const std::string& text, const Font& font);

    // Rasterizes text and calls the callback. Falls back to BitmapFont if needed.
    // The callback must accept: (int x, int y, int w, int h, uint8_t alpha).
    template <typename DrawCallback>
    static void draw_text(const std::string& text, const Font& font, const Point& position, DrawCallback&& callback) {
        IFontBackend* backend = get_backend();
        if (backend && backend->load_font(font)) {
            backend->draw_text(text, font, position, callback);
            return;
        }

        // Fallback to BitmapFont
        BitmapFont::draw_text(text, font.size, position, [&callback](int x, int y, int w, int h) {
            callback(x, y, w, h, 255);
        });
    }

    // Queries the OS for available font families.
    static std::vector<std::string> get_available_fonts();

private:
    static IFontBackend* get_backend();

    static std::unique_ptr<IFontBackend> backend_;
    static bool attempted_init_;
};

} // namespace ooey
