#include <gtest/gtest.h>
#include "gooey/application.hpp"
#include "gooey/controls/menubar.hpp"
#include "gooey/controls/menu.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "ooey/platform.hpp"

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
