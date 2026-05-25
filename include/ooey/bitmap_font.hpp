#pragma once

#include "ooey/types.hpp"
#include <string>
#include <cstdint>

namespace ooey {

class BitmapFont {
public:
    // Measures the width and height of a string given a font size.
    static Size measure_text(const std::string& text, int font_size);

    // Renders the characters into a callback that draws individual pixels or sub-rectangles.
    // Callback signature: void(int x, int y, int w, int h)
    template <typename DrawRectCallback>
    static void draw_text(const std::string& text, int font_size, const Point& position, DrawRectCallback&& draw_rect) {
        if (text.empty()) {
            return;
        }
        int scale = get_font_scale(font_size);
        int x = position.x;
        int y = position.y;
        for (char c : text) {
            const uint8_t* glyph = get_glyph_for_char(c);
            for (int row = 0; row < 7; ++row) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < 5; ++col) {
                    if (bits & (1 << (4 - col))) {
                        draw_rect(x + col * scale, y + row * scale, scale, scale);
                    }
                }
            }
            x += get_glyph_width(font_size);
        }
    }

private:
    static const uint8_t* get_glyph_for_char(char c);
    static int get_font_scale(int font_size);
    static int get_glyph_width(int font_size);
    static int get_glyph_height(int font_size);
};

} // namespace ooey
