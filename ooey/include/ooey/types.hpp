#pragma once

#include <cstdint>
#include <cstring>

namespace ooey {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    constexpr Color() = default;
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a) {}

    bool operator==(const Color&) const = default;
};

struct Point {
    int x{0};
    int y{0};

    constexpr Point() = default;
    constexpr Point(int x, int y) : x(x), y(y) {}

    bool operator==(const Point&) const = default;
};

struct Size {
    int width{0};
    int height{0};

    constexpr Size() = default;
    constexpr Size(int width, int height) : width(width), height(height) {}

    bool operator==(const Size&) const = default;
};

struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};

    constexpr Rect() = default;
    constexpr Rect(int x, int y, int width, int height)
        : x(x), y(y), width(width), height(height) {}

    bool operator==(const Rect&) const = default;
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

    bool operator==(const Font& other) const {
        if (size != other.size || weight != other.weight || style != other.style) {
            return false;
        }
        if (family == other.family) {
            return true;
        }
        if (family == nullptr || other.family == nullptr) {
            return false;
        }
        return std::strcmp(family, other.family) == 0;
    }
};

enum class WindowResizeEdge {
    None = 0,
    Top = 1,
    Bottom = 2,
    Left = 3,
    TopLeft = 4,
    BottomLeft = 5,
    Right = 6,
    TopRight = 7,
    BottomRight = 8
};

} // namespace ooey

