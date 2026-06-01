#pragma once

#include "gooey/mvvmc/theme.hpp"
#include <memory>

// Registers the standard Dark, Light, Hacker, and Lofi styles into the theme manager
void register_sysinfo_themes(const std::shared_ptr<gooey::ThemeManager>& theme_manager);
