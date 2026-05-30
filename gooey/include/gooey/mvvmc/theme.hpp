#pragma once

#include "gooey/mvvmc/property.hpp"
#include "ooey/types.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace gooey::mvvmc {
    using namespace ooey;

struct Style {
    Color fill_color{0, 0, 0, 0};
    Color stroke_color{0, 0, 0, 0};
    float stroke_thickness{0.0f};
    Color text_color{255, 255, 255, 255};
    int corner_radius{0};
};

class Theme {
public:
    std::string name;

    void set_style(const std::string& style_name, const Style& style) {
        styles_[style_name] = style;
    }

    bool get_style(const std::string& style_name, Style& style) const {
        auto it = styles_.find(style_name);
        if (it != styles_.end()) {
            style = it->second;
            return true;
        }
        return false;
    }

private:
    std::unordered_map<std::string, Style> styles_;
};

class ThemeManager {
public:
    ThemeManager() = default;

    // Reactive active theme property
    Property<std::shared_ptr<Theme>> active_theme{nullptr};

    void add_theme(const std::string& theme_name, std::shared_ptr<Theme> theme) {
        themes_[theme_name] = theme;
        if (!active_theme.get()) {
            active_theme.set(theme);
        }
    }

    void set_active_theme(const std::string& theme_name) {
        auto it = themes_.find(theme_name);
        if (it != themes_.end()) {
            active_theme.set(it->second);
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Theme>> themes_;
};

} // namespace gooey::mvvmc
namespace gooey {
using gooey::mvvmc::Style;
using gooey::mvvmc::Theme;
using gooey::mvvmc::ThemeManager;
}
