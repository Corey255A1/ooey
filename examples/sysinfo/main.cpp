#include <iostream>
#include <memory>
#include <chrono>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "view_model.hpp"
#include "view.hpp"
#include "themes.hpp"
#include "gooey/controls/scroll_container.hpp"

using namespace ooey;
using namespace gooey;

int main() {
    std::cout << "Starting OOEY Real-Time System Monitor Dashboard...\n";

    Application app;

    auto backend = create_default_window_backend();
    if (!backend || !backend->create({900, 700}, "OOEY Live System Monitor")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto theme_manager = std::make_shared<ThemeManager>();
    register_sysinfo_themes(theme_manager);
    theme_manager->set_active_theme("dark");
    app.set_theme_manager(theme_manager);

    auto view_model = std::make_shared<SystemMonitorViewModel>(theme_manager);
    auto view = std::make_shared<SystemMonitorView>(view_model);
    auto root_view = std::make_shared<ScrollContainer>();
    root_view->set_width(SizePolicy::MatchParent);
    root_view->set_height(SizePolicy::MatchParent);
    root_view->set_child(view);
    app.set_root_view(root_view);

    app.run();
    return 0;
}
