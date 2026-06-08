#include <gtest/gtest.h>
#include "gooey/application.hpp"
#include "gooey/controls/menubar.hpp"
#include "gooey/controls/menu.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/theme.hpp"

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;
using namespace gooey::mvvmc;

TEST(GooeyControls, MenuBarLayoutAndResponsiveness) {
    Application app;
    auto root = std::make_shared<GooeyNode>();
    auto controller = std::make_unique<Controller>(app.get_input_manager(), root);
    app.set_controller(std::move(controller));

    std::vector<MenuCategory> categories = {
        {"File", {
            {"New File", "Ctrl+N", false, false, false, {}, [](){}},
            {"Exit", "", false, false, false, {}, [](){}}
        }},
        {"Edit", {
            {"Cut", "Ctrl+X", false, false, false, {}, [](){}}
        }}
    };

    auto menu_bar = std::make_shared<MenuBar>(categories);
    menu_bar->set_breakpoint(600);
    root->add_child(menu_bar);

    // 1. Test Horizontal Mode (width > breakpoint)
    menu_bar->measure(Size{800, 300});
    menu_bar->layout(Rect{0, 0, 800, 40});

    EXPECT_EQ(menu_bar->bounds().width, 800);
    EXPECT_EQ(menu_bar->bounds().height, 40);

    // 2. Test Collapsed Responsive Mode (width <= breakpoint, hamburger closed)
    menu_bar->measure(Size{500, 300});
    menu_bar->layout(Rect{0, 0, 500, 40});

    EXPECT_EQ(menu_bar->bounds().width, 500);
    EXPECT_EQ(menu_bar->bounds().height, 40);

    // 3. Simulating Hamburger button click to expand
    // Hamburger button is located at the right: width - 40 to width, y to y + 40
    // For 500px width, hamburger rect is x: 460, y: 0, w: 40, h: 40
    Pointer press_ham{.id = 0, .x = 480, .y = 20, .state = PointerState::Pressed};
    EXPECT_TRUE(menu_bar->on_pointer_event(press_ham));

    // When expanded, the height of the MenuBar increases to show categories vertically
    // 40 (closed height) + 40 * number of categories (2) = 120
    Size measured = menu_bar->measure(Size{500, 300});
    EXPECT_EQ(measured.height, 120);
    
    menu_bar->layout(Rect{0, 0, 500, measured.height});
    EXPECT_EQ(menu_bar->bounds().height, 120);

    // Simulating clicking hamburger button again to collapse
    EXPECT_TRUE(menu_bar->on_pointer_event(press_ham));
    measured = menu_bar->measure(Size{500, 300});
    EXPECT_EQ(measured.height, 40);
}

TEST(GooeyControls, MenuSpawningAndInteractions) {
    Application app;
    auto root = std::make_shared<GooeyNode>();
    auto controller = std::make_unique<Controller>(app.get_input_manager(), root);
    app.set_controller(std::move(controller));

    bool new_file_clicked = false;
    std::vector<MenuCategory> categories = {
        {"File", {
            {"New File", "Ctrl+N", false, false, false, {}, [&]() { new_file_clicked = true; }},
            {"Exit", "", false, false, false, {}, [](){}}
        }}
    };

    auto menu_bar = std::make_shared<MenuBar>(categories);
    root->add_child(menu_bar);
    root->measure(Size{800, 600});
    root->layout(Rect{0, 0, 800, 600});

    // Verify no menu is initially open
    EXPECT_EQ(root->get_children().size(), 1);

    // Press the "File" category header to open the dropdown menu
    // Category rect will be at bounds.x + 10 to bounds.x + 10 + text_width + 30
    // "File" size is around 40 + 30 = 70. So press at x=30, y=20
    Pointer press_file{.id = 0, .x = 30, .y = 20, .state = PointerState::Pressed};
    EXPECT_TRUE(menu_bar->on_pointer_event(press_file));

    // A child Menu dropdown should be spawned on the root node
    // Total children now should be 2 (MenuBar + Menu)
    EXPECT_EQ(root->get_children().size(), 2);

    auto menu_child = std::dynamic_pointer_cast<Menu>(root->get_children()[1]);
    ASSERT_NE(menu_child, nullptr);
    EXPECT_TRUE(menu_child->is_open());

    // Hover over the first menu item "New File" first (PointerState::Moved)
    int menu_y = menu_child->bounds().y;
    Pointer move_new_file{.id = 0, .x = menu_child->bounds().x + 20, .y = menu_y + 15, .state = PointerState::Moved};
    EXPECT_TRUE(menu_child->on_pointer_event(move_new_file));

    // Then click it (PointerState::Pressed)
    Pointer click_new_file{.id = 0, .x = menu_child->bounds().x + 20, .y = menu_y + 15, .state = PointerState::Pressed};
    EXPECT_TRUE(menu_child->on_pointer_event(click_new_file));

    // Action should be triggered
    EXPECT_TRUE(new_file_clicked);

    // Clicking a leaf menu item should automatically close the menu dropdown hierarchy
    EXPECT_FALSE(menu_child->is_open());
    EXPECT_EQ(root->get_children().size(), 1);
}

TEST(GooeyControls, MenuKeyboardNavigation) {
    Application app;
    auto root = std::make_shared<GooeyNode>();
    auto controller = std::make_unique<Controller>(app.get_input_manager(), root);
    app.set_controller(std::move(controller));

    std::vector<MenuCategory> categories = {
        {"File", {
            {"New File", "", false, false, false, {}, [](){}},
            {"Open File", "", false, false, false, {}, [](){}}
        }},
        {"Edit", {
            {"Cut", "", false, false, false, {}, [](){}}
        }}
    };

    auto menu_bar = std::make_shared<MenuBar>(categories);
    root->add_child(menu_bar);
    root->measure(Size{800, 600});
    root->layout(Rect{0, 0, 800, 600});

    // Open "File" menu
    Pointer press_file{.id = 0, .x = 30, .y = 20, .state = PointerState::Pressed};
    menu_bar->on_pointer_event(press_file);

    auto menu_child = std::dynamic_pointer_cast<Menu>(root->get_children()[1]);
    ASSERT_NE(menu_child, nullptr);

    // Focus is on the opened menu
    auto* controller_impl = dynamic_cast<Controller*>(app.get_controller());
    ASSERT_NE(controller_impl, nullptr);
    EXPECT_EQ(controller_impl->get_focused_element(), menu_child);

    // Pressing Arrow-Right should cycle to the next category ("Edit") and open its menu
    KeyEvent right_key{.key_code = 0xFF53, .state = KeyState::Pressed};
    EXPECT_TRUE(menu_child->on_key_event(right_key));

    // The old menu should close, and the new menu ("Edit") should be spawned
    auto active_menu = std::dynamic_pointer_cast<Menu>(root->get_children()[1]);
    ASSERT_NE(active_menu, nullptr);
    EXPECT_NE(active_menu, menu_child);
}

TEST(GooeyControls, ViewSubmenusLanguageAndTheme) {
    Application app;
    auto root = std::make_shared<GooeyNode>();
    auto controller = std::make_unique<Controller>(app.get_input_manager(), root);
    app.set_controller(std::move(controller));

    auto theme_manager = std::make_shared<ThemeManager>();
    auto dark_theme = std::make_shared<Theme>();
    dark_theme->name = "dark";
    theme_manager->add_theme("dark", dark_theme);
    auto cyberpunk_theme = std::make_shared<Theme>();
    cyberpunk_theme->name = "cyberpunk";
    theme_manager->add_theme("cyberpunk", cyberpunk_theme);
    theme_manager->set_active_theme("dark");

    std::vector<MenuCategory> categories = {
        {"File", {
            {"Exit", "", false, false, false, {}, [](){}}
        }},
        {"View", {
            {"Language", "", false, false, false, {
                {"English", "", false, true, true, {}, [](){}},
                {"Spanish", "", false, true, false, {}, [](){}}
            }, [](){}},
            {"Theme", "", false, false, false, {
                {"Dark", "", false, true, true, {}, [theme_manager](){ theme_manager->set_active_theme("dark"); }},
                {"Cyberpunk", "", false, true, false, {}, [theme_manager](){ theme_manager->set_active_theme("cyberpunk"); }}
            }, [](){}}
        }}
    };

    auto menu_bar = std::make_shared<MenuBar>(categories);
    root->add_child(menu_bar);
    root->measure(Size{800, 600});
    root->layout(Rect{0, 0, 800, 600});

    // 1. Open "View" Category (which is index 1)
    // Press at x=90, y=20
    Pointer press_view{.id = 0, .x = 90, .y = 20, .state = PointerState::Pressed};
    EXPECT_TRUE(menu_bar->on_pointer_event(press_view));

    auto view_menu = std::dynamic_pointer_cast<Menu>(root->get_children()[1]);
    ASSERT_NE(view_menu, nullptr);

    // 2. Hover over "Theme" option (second item in View menu: y = menu_y + 30 to menu_y + 60)
    int menu_y = view_menu->bounds().y;
    Pointer hover_theme{.id = 0, .x = view_menu->bounds().x + 20, .y = menu_y + 45, .state = PointerState::Moved};
    EXPECT_TRUE(view_menu->on_pointer_event(hover_theme));

    // 3. Hover over "Cyberpunk" option inside Theme submenu
    int sub_x = view_menu->bounds().x + view_menu->bounds().width + 20;
    int sub_y = menu_y + 30 + 45; // Theme item starts at y=menu_y+30, Cyberpunk is second submenu item (midpoint +45)
    Pointer move_cyberpunk{.id = 0, .x = sub_x, .y = sub_y, .state = PointerState::Moved};
    EXPECT_TRUE(view_menu->on_pointer_event(move_cyberpunk));

    // 4. Click "Cyberpunk"
    Pointer click_cyberpunk{.id = 0, .x = sub_x, .y = sub_y, .state = PointerState::Pressed};
    EXPECT_TRUE(view_menu->on_pointer_event(click_cyberpunk));

    // The theme should be updated to "cyberpunk"!
    EXPECT_EQ(theme_manager->active_theme.get()->name, "cyberpunk");
}

TEST(GooeyControls, MenuSpawningWithControllerEvents) {
    Application app;
    auto root = std::make_shared<GooeyNode>();
    auto controller = std::make_unique<Controller>(app.get_input_manager(), root);
    auto* ctrl_ptr = controller.get();
    app.set_controller(std::move(controller));

    std::vector<MenuCategory> categories = {
        {"File", {
            {"New File", "", false, false, false, {}, [](){}},
            {"Exit", "", false, false, false, {}, [](){}}
        }}
    };

    auto menu_bar = std::make_shared<MenuBar>(categories);
    root->add_child(menu_bar);
    root->measure(Size{800, 600});
    root->layout(Rect{0, 0, 800, 600});

    // 1. Simulate mouse press on File category via InputManager
    // Press at x=30, y=20
    app.get_input_manager().push_pointer_event(Pointer{.id = 0, .x = 30, .y = 20, .state = PointerState::Pressed});
    
    // Process event
    ctrl_ptr->process_events();

    // Check if menu is open
    ASSERT_EQ(root->get_children().size(), 2);
    auto menu_child = std::dynamic_pointer_cast<Menu>(root->get_children()[1]);
    ASSERT_NE(menu_child, nullptr);
    EXPECT_TRUE(menu_child->is_open());

    // 2. Draw Menu to ensure focus validation doesn't dismiss it
    class DummyRenderTarget : public IRenderTarget {
    public:
        void clear(Color) override {}
        void draw_geometry(const Geometry&) override {}
        void draw_image(const Image&, const Rect&) override {}
        Size measure_text(const std::string&, const Font&) override { return Size{0, 0}; }
        void draw_text(const std::string&, const Font&, const Point&, Color) override {}
        void push_clip(const Rect&) override {}
        void pop_clip() override {}
        void present() override {}
    };
    DummyRenderTarget dummy_target;
    menu_child->draw(dummy_target);

    // The menu should still be open!
    EXPECT_TRUE(menu_child->is_open());
    EXPECT_EQ(root->get_children().size(), 2);

    // 3. Clear transient input events and send a release event
    app.get_input_manager().update();
    app.get_input_manager().push_pointer_event(Pointer{.id = 0, .x = 30, .y = 20, .state = PointerState::Released});
    ctrl_ptr->process_events();

    // The menu should still be open!
    EXPECT_TRUE(menu_child->is_open());

    // 4. Add another interactive element to the root and click it to change focus
    auto button = std::make_shared<Button>();
    button->set_absolute_bounds(Rect{100, 100, 50, 30});
    button->layout(Rect{100, 100, 50, 30});
    root->add_child(button);
    // Refresh layout
    root->measure(Size{800, 600});
    root->layout(Rect{0, 0, 800, 600});

    app.get_input_manager().update();
    app.get_input_manager().push_pointer_event(Pointer{.id = 0, .x = 120, .y = 115, .state = PointerState::Pressed});
    ctrl_ptr->process_events();

    // Trigger draw to process focus validation
    menu_child->draw(dummy_target);

    // Run dispatcher tasks
    app.run();

    // Now the menu child should be closed!
    EXPECT_FALSE(menu_child->is_open());
}

