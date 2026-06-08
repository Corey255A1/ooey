#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/gooey_node.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/menubar.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

// ---------------------------------------------------------
// 1. The ViewModel (Logic & State)
// ---------------------------------------------------------
class MenuViewModel {
private:
    std::shared_ptr<ThemeManager> theme_manager_;
    std::string active_theme_name_{"dark"};

public:
    explicit MenuViewModel(std::shared_ptr<ThemeManager> theme_manager)
        : theme_manager_(std::move(theme_manager)) {}

    // Observable properties
    Property<std::string> status_text{"Last Action: None"};
    Property<std::string> active_theme_prop{"dark"};

    void trigger_action(const std::string& action_name) {
        status_text.set("Last Action: Triggered " + action_name);
        std::cout << "ViewModel Action: " << action_name << std::endl;
    }

    void set_theme(const std::string& theme_name) {
        if (theme_manager_) {
            theme_manager_->set_active_theme(theme_name);
            active_theme_name_ = theme_name;
            active_theme_prop.set(theme_name);
            status_text.set("Theme changed to: " + theme_name);
            std::cout << "ViewModel: Changed active theme to " << theme_name << "\n";
        }
    }

    std::string get_active_theme() const {
        return active_theme_name_;
    }
};

// ---------------------------------------------------------
// 2. The View (UI Layout Composition & Bindings)
// ---------------------------------------------------------
class MenuView : public Column {
private:
    std::shared_ptr<MenuViewModel> view_model_;
    std::shared_ptr<MenuBar> menu_bar_;
    std::shared_ptr<Label> status_label_;

    // Helper to generate the dynamic categories menu list with correct checkmarks
    std::vector<MenuCategory> build_menu_categories() {
        std::string current_theme = view_model_->get_active_theme();

        // 1. File Category
        MenuCategory file_cat;
        file_cat.name = "File";
        file_cat.items = {
            MenuItem{.label = "New File", .shortcut = "Ctrl+N", .action = [this]() { view_model_->trigger_action("File -> New File"); }},
            MenuItem{.label = "Open File...", .shortcut = "Ctrl+O", .action = [this]() { view_model_->trigger_action("File -> Open File"); }},
            MenuItem{.label = "Save", .shortcut = "Ctrl+S", .action = [this]() { view_model_->trigger_action("File -> Save"); }},
            MenuItem{.is_separator = true},
            MenuItem{.label = "Exit", .shortcut = "Alt+F4", .action = []() { Application::get_instance()->quit(); }}
        };

        // 2. Edit Category
        MenuCategory edit_cat;
        edit_cat.name = "Edit";
        
        // Nested subitems for Edit -> Preferences
        std::vector<MenuItem> pref_subitems = {
            MenuItem{.label = "General Settings", .action = [this]() { view_model_->trigger_action("Edit -> Preferences -> General Settings"); }},
            MenuItem{.label = "Keymaps", .action = [this]() { view_model_->trigger_action("Edit -> Preferences -> Keymaps"); }}
        };

        edit_cat.items = {
            MenuItem{.label = "Cut", .shortcut = "Ctrl+X", .action = [this]() { view_model_->trigger_action("Edit -> Cut"); }},
            MenuItem{.label = "Copy", .shortcut = "Ctrl+C", .action = [this]() { view_model_->trigger_action("Edit -> Copy"); }},
            MenuItem{.label = "Paste", .shortcut = "Ctrl+V", .action = [this]() { view_model_->trigger_action("Edit -> Paste"); }},
            MenuItem{.is_separator = true},
            MenuItem{.label = "Preferences", .subitems = pref_subitems}
        };

        // 3. Theme Category with checkboxes
        MenuCategory theme_cat;
        theme_cat.name = "Theme";
        theme_cat.items = {
            MenuItem{
                .label = "Dark Mode",
                .is_checkbox = true,
                .checked = (current_theme == "dark"),
                .action = [this]() { view_model_->set_theme("dark"); }
            },
            MenuItem{
                .label = "Light Mode",
                .is_checkbox = true,
                .checked = (current_theme == "light"),
                .action = [this]() { view_model_->set_theme("light"); }
            },
            MenuItem{
                .label = "Cyberpunk",
                .is_checkbox = true,
                .checked = (current_theme == "cyberpunk"),
                .action = [this]() { view_model_->set_theme("cyberpunk"); }
            }
        };

        // 4. Help Category
        MenuCategory help_cat;
        help_cat.name = "Help";
        help_cat.items = {
            MenuItem{.label = "Welcome Guide", .action = [this]() { view_model_->trigger_action("Help -> Welcome Guide"); }},
            MenuItem{.label = "About Gooey Menu", .action = [this]() { view_model_->trigger_action("Help -> About Gooey Menu"); }}
        };

        return {file_cat, edit_cat, theme_cat, help_cat};
    }

public:
    explicit MenuView(std::shared_ptr<MenuViewModel> view_model)
        : view_model_(std::move(view_model)) {
        
        // Main container styling
        set_width(SizePolicy::MatchParent);
        set_height(SizePolicy::MatchParent);
        set_padding(0); // No padding for the root so MenuBar fits tightly at the top

        // 1. Create and add MenuBar
        menu_bar_ = std::make_shared<MenuBar>(build_menu_categories());
        menu_bar_->set_style_name("menubar");
        menu_bar_->set_breakpoint(600); // Collapse below 600px
        add_child(menu_bar_);

        // 2. Content Container Column
        auto content = std::make_shared<Column>();
        content->set_width(SizePolicy::MatchParent);
        content->set_height(SizePolicy::WrapContent);
        content->set_padding(25);
        content->set_margin(0);

        // Header Title Label
        auto title = std::make_shared<Label>(
            "Gooey Menu Bar Demo",
            Font{"sans-serif", 24, FontWeight::Bold},
            Point{0, 0},
            Color{240, 240, 245}
        );
        title->set_absolute(false);
        title->set_margin(0, 0, 0, 10);
        title->set_style_name("title-text");
        content->add_child(title);

        // Description
        auto desc = std::make_shared<Label>(
            "This demo showcases the new responsive Menu and MenuBar controls in action.\n\n"
            "Features:\n"
            "1. Classic desktop OS style menus with hover and selection.\n"
            "2. Nested submenus (Edit -> Preferences -> General Settings / Keymaps).\n"
            "3. Menu items with keyboard shortcuts and separator lines.\n"
            "4. Dynamic checkable MenuItems (check out the Theme dropdown options).\n"
            "5. Responsive auto-collapse to vertical hamburger view for screens <= 600px wide.",
            Font{"sans-serif", 13},
            Point{0, 0},
            Color{170, 170, 180}
        );
        desc->set_absolute(false);
        desc->set_margin(0, 0, 0, 30);
        desc->set_style_name("subtitle-text");
        content->add_child(desc);

        // Status Label showing selected menu choices
        status_label_ = std::make_shared<Label>(
            "Last Action: None",
            Font{"sans-serif", 14, FontWeight::Bold},
            Point{0, 0},
            Color{0, 180, 240}
        );
        status_label_->set_absolute(false);
        status_label_->set_margin(0, 0, 0, 20);
        status_label_->set_style_name("card-title-accent");
        content->add_child(status_label_);

        add_child(content);

        // Bindings
        view_model_->status_text.subscribe([this](const std::string& val) {
            status_label_->set_text(val);
        });

        // Update menu list checkboxes dynamically when theme changes
        view_model_->active_theme_prop.subscribe([this](const std::string&) {
            menu_bar_->set_categories(build_menu_categories());
        });
    }
};

// ---------------------------------------------------------
// 3. Application Main Entry Point
// ---------------------------------------------------------
int main(int, char**) {
    std::cout << "Starting Gooey Responsive Menu System Example..." << std::endl;

    Application app;

    auto backend = create_default_window_backend();
    if (!backend || !backend->create({700, 450}, "Gooey Responsive Menu Demo")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    // Setup Theme Manager
    auto theme_manager = std::make_shared<ThemeManager>();

    // 1. Dark Theme
    auto dark_theme = std::make_shared<Theme>();
    dark_theme->name = "dark";
    dark_theme->set_style("window", Style{.fill_color = Color{20, 20, 25}});
    dark_theme->set_style("menubar", Style{.fill_color = Color{30, 30, 35}, .stroke_color = Color{60, 60, 70}, .text_color = Color{220, 220, 225}});
    dark_theme->set_style("title-text", Style{.text_color = Color{0, 180, 240}});
    dark_theme->set_style("subtitle-text", Style{.text_color = Color{150, 150, 160}});
    dark_theme->set_style("card-title-accent", Style{.text_color = Color{0, 180, 240}});
    theme_manager->add_theme("dark", dark_theme);

    // 2. Light Theme
    auto light_theme = std::make_shared<Theme>();
    light_theme->name = "light";
    light_theme->set_style("window", Style{.fill_color = Color{240, 240, 245}});
    light_theme->set_style("menubar", Style{.fill_color = Color{225, 225, 230}, .stroke_color = Color{180, 180, 190}, .text_color = Color{40, 40, 45}});
    light_theme->set_style("title-text", Style{.text_color = Color{0, 100, 200}});
    light_theme->set_style("subtitle-text", Style{.text_color = Color{80, 80, 90}});
    light_theme->set_style("card-title-accent", Style{.text_color = Color{0, 100, 200}});
    theme_manager->add_theme("light", light_theme);

    // 3. Cyberpunk Theme
    auto cyberpunk_theme = std::make_shared<Theme>();
    cyberpunk_theme->name = "cyberpunk";
    cyberpunk_theme->set_style("window", Style{.fill_color = Color{10, 10, 15}});
    cyberpunk_theme->set_style("menubar", Style{.fill_color = Color{0, 0, 0}, .stroke_color = Color{255, 0, 128}, .text_color = Color{0, 255, 255}});
    cyberpunk_theme->set_style("title-text", Style{.text_color = Color{255, 0, 128}});
    cyberpunk_theme->set_style("subtitle-text", Style{.text_color = Color{0, 255, 255}});
    cyberpunk_theme->set_style("card-title-accent", Style{.text_color = Color{255, 255, 0}});
    theme_manager->add_theme("cyberpunk", cyberpunk_theme);

    theme_manager->set_active_theme("dark");
    app.set_theme_manager(theme_manager);

    // Bootstrap MVVM Components
    auto view_model = std::make_shared<MenuViewModel>(theme_manager);
    auto root_view = std::make_shared<MenuView>(view_model);

    app.set_root_view(std::move(root_view));
    app.run();

    return 0;
}
