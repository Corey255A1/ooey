#pragma once

#include <cstdint>

namespace ooey {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}
};

struct Point {
    int x{0};
    int y{0};

    constexpr Point() = default;
    constexpr Point(int x, int y) : x(x), y(y) {}
};

struct Size {
    int width{0};
    int height{0};

    constexpr Size() = default;
    constexpr Size(int width, int height) : width(width), height(height) {}
};

struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};

    constexpr Rect() = default;
    constexpr Rect(int x, int y, int width, int height)
        : x(x), y(y), width(width), height(height) {}
};

enum class FontWeight {
    Normal,
    Bold
};

enum class FontStyle {
    Normal,
    Italic
};

struct Font {
    const char* family{"sans-serif"};
    int size{14};
    FontWeight weight{FontWeight::Normal};
    FontStyle style{FontStyle::Normal};

    constexpr Font() = default;
    constexpr Font(const char* family, int size, FontWeight weight = FontWeight::Normal, FontStyle style = FontStyle::Normal)
        : family(family), size(size), weight(weight), style(style) {}
};

} // namespace ooey
