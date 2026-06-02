#include <gtest/gtest.h>
#include "ooey/types.hpp"
#include "gooey/application.hpp"
#include "gooey/controls/text_box.hpp"
#include "code_editor.hpp"
#include "gooey/controls/list_control.hpp"
#include "gooey/mvvmc/navigation_coordinator.hpp"
#include "gooey/mvvmc/controller.hpp"
#include "ooey/input.hpp"

TEST(OoeyTypes, ColorInitialization) {
    ooey::Color c{255, 128, 64, 200};
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 128);
    EXPECT_EQ(c.b, 64);
    EXPECT_EQ(c.a, 200);

    ooey::Color default_alpha{10, 20, 30};
    EXPECT_EQ(default_alpha.a, 255);
}

TEST(OoeyTypes, RectInitialization) {
    ooey::Rect r{10, 20, 100, 200};
    EXPECT_EQ(r.x, 10);
    EXPECT_EQ(r.y, 20);
    EXPECT_EQ(r.width, 100);
    EXPECT_EQ(r.height, 200);
}

namespace {

class MockWindowBackend : public ooey::IWindowBackend {
public:
    int poll_count = 0;
    ooey::InputManager* input_manager = nullptr;

    bool create(const ooey::Size& size, const char* title) override {
        return true;
    }

    void destroy() override {
    }

    bool poll_events(int timeout_ms = 0) override {
        (void)timeout_ms;
        poll_count++;
        if (poll_count >= 3) {
            return false; // Exit after 3 polls
        }
        return true;
    }

    void set_input_manager(ooey::InputManager* manager) override {
        input_manager = manager;
    }

    void poll_input() override {
    }

    ooey::IRenderTarget* get_render_target() override {
        return nullptr;
    }

    void set_window_chrome(std::shared_ptr<ooey::WindowChrome> chrome) override { window_chrome_ = chrome; }
    std::shared_ptr<ooey::WindowChrome> get_window_chrome() const override { return window_chrome_; }
    void start_interactive_move() override {}
    void start_interactive_resize(ooey::WindowResizeEdge /*edge*/) override {}
    void request_close() override {}
    ooey::Size get_size() const override { return ooey::Size{800, 600}; }

private:
    std::shared_ptr<ooey::WindowChrome> window_chrome_;
};

} // namespace

TEST(OoeyApplication, MainLoopExecution) {
    gooey::Application app;
    auto mock_backend = std::make_unique<MockWindowBackend>();
    auto* mock_ptr = mock_backend.get();

    app.set_window_backend(std::move(mock_backend));
    app.run();

    EXPECT_EQ(mock_ptr->poll_count, 3);
}

TEST(OoeyApplication, HeadlessExit) {
    gooey::Application app;
    // Should exit immediately without an infinite loop if no backend
    app.run(); 
    SUCCEED();
}

TEST(OoeyControls, TextBoxKeyboardInput) {
    ooey::Font font{"sans-serif", 16};
    gooey::TextBox textBox{ooey::Rect{0, 0, 100, 100}, font, ooey::Color{0, 0, 0}, ooey::Color{255, 255, 255}};

    // Text box needs to be focused to process key/text events
    textBox.on_pointer_event({0, 10, 10, ooey::PointerState::Pressed});

    // Send character text events
    textBox.on_text_event({static_cast<char32_t>('H')});
    textBox.on_text_event({static_cast<char32_t>('e')});
    textBox.on_text_event({static_cast<char32_t>('l')});
    textBox.on_text_event({static_cast<char32_t>('l')});
    textBox.on_text_event({static_cast<char32_t>('o')});

    EXPECT_EQ(textBox.get_text(), "Hello");

    // Send backspace key event (using keysym 0xFF08 / XKB_KEY_BackSpace / XK_BackSpace)
    textBox.on_key_event({0xFF08, ooey::KeyState::Pressed});
    EXPECT_EQ(textBox.get_text(), "Hell");

    // Send backspace key event (using ASCII backspace 8)
    textBox.on_key_event({8, ooey::KeyState::Pressed});
    EXPECT_EQ(textBox.get_text(), "Hel");
}

TEST(OoeyControls, ListControlNavigationAndScrolling) {
    ooey::Rect bounds{100, 100, 200, 250};
    ooey::Font font{"sans-serif", 16};
    ooey::Color text_color{255, 255, 255};
    ooey::Color bg_color{30, 30, 30};
    ooey::Color highlight_bg{0, 120, 215};
    ooey::Color highlight_text{255, 255, 255};

    gooey::ListControl listCtrl{bounds, 50, font, text_color, bg_color, highlight_bg, highlight_text};

    std::vector<std::string> items;
    for (int i = 1; i <= 200; ++i) {
        items.push_back("Element " + std::to_string(i));
    }
    listCtrl.set_items(items);

    // Initial state
    EXPECT_EQ(listCtrl.get_selected_index(), 0);

    // select_next
    listCtrl.select_next();
    EXPECT_EQ(listCtrl.get_selected_index(), 1);

    // select_previous
    listCtrl.select_previous();
    EXPECT_EQ(listCtrl.get_selected_index(), 0);

    // Scroll boundary test: select 6th item (index 5)
    for (int i = 0; i < 5; ++i) {
        listCtrl.select_next();
    }
    EXPECT_EQ(listCtrl.get_selected_index(), 5);

    // Clicking to select (3rd visible item, which corresponds to index scroll_offset + 2)
    // At selected_index_ = 5, the scroll_offset should adjust:
    // since selected_index (5) >= scroll_offset (0) + 5, scroll_offset_ becomes 5 - 4 = 1.
    // So visible items are index 1 to 5.
    // 3rd visible item is at index 1 + 2 = 3.
    // item_height is 250 / 5 = 50.
    // 3rd slot: y from 100 + 2 * 50 = 200 to 250.
    // Click at y = 225, x = 150.
    listCtrl.on_pointer_event({0, 150, 225, ooey::PointerState::Pressed});
    EXPECT_EQ(listCtrl.get_selected_index(), 3);

    // Key events (XK_Down = 0xFF54, XK_Up = 0xFF52)
    listCtrl.on_key_event({0xFF54, ooey::KeyState::Pressed}); // Down arrow
    EXPECT_EQ(listCtrl.get_selected_index(), 4);

    listCtrl.on_key_event({0xFF52, ooey::KeyState::Pressed}); // Up arrow
    EXPECT_EQ(listCtrl.get_selected_index(), 3);
}

class TestPageViewModel : public gooey::PageViewModelBase {
public:
    explicit TestPageViewModel(std::string name) : name_(std::move(name)) {}
    std::string get_title() const override { return name_; }
private:
    std::string name_;
};

TEST(OoeyMvvmc, NavigationCoordinatorTransitions) {
    auto coordinator = std::make_shared<gooey::NavigationCoordinator>();

    auto p1 = std::make_shared<TestPageViewModel>("Page 1");
    auto p2 = std::make_shared<TestPageViewModel>("Page 2");
    auto p3 = std::make_shared<TestPageViewModel>("Page 3");

    // Initial state
    EXPECT_FALSE(coordinator->can_go_back.get());
    EXPECT_FALSE(coordinator->can_go_forward.get());
    EXPECT_EQ(coordinator->current_viewmodel.get(), nullptr);

    // Navigate to Page 1
    coordinator->navigate_to(p1);
    EXPECT_EQ(coordinator->current_viewmodel.get(), p1);
    EXPECT_FALSE(coordinator->can_go_back.get());
    EXPECT_FALSE(coordinator->can_go_forward.get());

    // Navigate to Page 2
    coordinator->navigate_to(p2);
    EXPECT_EQ(coordinator->current_viewmodel.get(), p2);
    EXPECT_TRUE(coordinator->can_go_back.get());
    EXPECT_FALSE(coordinator->can_go_forward.get());

    // Go back
    coordinator->go_back();
    EXPECT_EQ(coordinator->current_viewmodel.get(), p1);
    EXPECT_FALSE(coordinator->can_go_back.get());
    EXPECT_TRUE(coordinator->can_go_forward.get());

    // Go forward
    coordinator->go_forward();
    EXPECT_EQ(coordinator->current_viewmodel.get(), p2);
    EXPECT_TRUE(coordinator->can_go_back.get());
    EXPECT_FALSE(coordinator->can_go_forward.get());

    // Branching: go back to Page 1, then navigate to Page 3 (should clear Page 2 from forward history)
    coordinator->go_back();
    coordinator->navigate_to(p3);
    EXPECT_EQ(coordinator->current_viewmodel.get(), p3);
    EXPECT_TRUE(coordinator->can_go_back.get());
    EXPECT_FALSE(coordinator->can_go_forward.get());

    // Try go forward (should fail, forward history cleared)
    coordinator->go_forward();
    EXPECT_EQ(coordinator->current_viewmodel.get(), p3);
}

class MockController : public gooey::IController {
public:
    int process_count = 0;
    void process_events() override {
        process_count++;
    }
};

TEST(OoeyApplication, CustomControllerExecution) {
    gooey::Application app;
    auto mock_backend = std::make_unique<MockWindowBackend>();

    auto mock_controller = std::make_unique<MockController>();
    auto* controller_ptr = mock_controller.get();

    app.set_window_backend(std::move(mock_backend));
    app.set_controller(std::move(mock_controller));

    app.run();

    EXPECT_EQ(controller_ptr->process_count, 2);
}

TEST(OoeyApplication, DispatchAndSingleton) {
    EXPECT_EQ(gooey::Application::get_instance(), nullptr);

    {
        gooey::Application app;
        EXPECT_EQ(gooey::Application::get_instance(), &app);

        auto mock_backend = std::make_unique<MockWindowBackend>();
        app.set_window_backend(std::move(mock_backend));

        bool task_executed = false;
        app.dispatch([&task_executed]() {
            task_executed = true;
        });

        EXPECT_FALSE(task_executed);

        app.run();

        EXPECT_TRUE(task_executed);
    }

    EXPECT_EQ(gooey::Application::get_instance(), nullptr);
}

TEST(OoeyApplication, PropertyChangeInvalidation) {
    EXPECT_EQ(gooey::Application::get_instance(), nullptr);

    {
        gooey::Application app;
        EXPECT_EQ(gooey::Application::get_instance(), &app);

        gooey::Property<int> prop{0};
        prop.set(42);
        EXPECT_EQ(prop.get(), 42);
    }

    EXPECT_EQ(gooey::Application::get_instance(), nullptr);
}

class DeletingView : public gooey::View, public gooey::mvvmc::IInteractive {
public:
    bool clicked = false;

    DeletingView() {
        is_absolute = true;
        absolute_bounds = ooey::Rect{0, 0, 100, 100};
    }

    ooey::Rect bounds() const override { return absolute_bounds; }

    bool on_pointer_event(const ooey::Pointer& e) override {
        if (e.state == ooey::PointerState::Pressed) {
            clicked = true;
            if (get_parent()) {
                get_parent()->clear_children();
            }
            return true;
        }
        return false;
    }

    bool on_key_event(const ooey::KeyEvent&) override { return false; }
};

TEST(OoeyMvvmc, SafeControllerElementDeletions) {
    ooey::InputManager input_manager;
    auto root_view = std::make_shared<gooey::View>();

    auto deleting_view = std::make_shared<DeletingView>();
    root_view->add_child(deleting_view);

    gooey::Controller controller(input_manager, root_view);

    input_manager.push_pointer_event(ooey::Pointer{1, 10, 10, ooey::PointerState::Pressed});
    controller.process_events();

    EXPECT_TRUE(deleting_view->clicked);
    
    // Send Released event; should not crash despite deleting_view being deleted
    input_manager.push_pointer_event(ooey::Pointer{1, 10, 10, ooey::PointerState::Released});
    controller.process_events();
}

TEST(OoeyMvvmc, RealRealElementDeletions) {
    ooey::InputManager input_manager;
    auto root_view = std::make_shared<gooey::View>();

    auto page_container = std::make_shared<gooey::View>();
    root_view->add_child(page_container);

    auto page_view = std::make_shared<gooey::View>();
    page_container->add_child(page_view);

    class NestedDeletingView : public gooey::View, public gooey::mvvmc::IInteractive {
    public:
        std::shared_ptr<gooey::View> container_to_clear;
        bool clicked = false;

        NestedDeletingView(std::shared_ptr<gooey::View> container)
            : container_to_clear(container) {
            is_absolute = true;
            absolute_bounds = ooey::Rect{0, 0, 100, 100};
        }

        ooey::Rect bounds() const override { return absolute_bounds; }

        bool on_pointer_event(const ooey::Pointer& e) override {
            if (e.state == ooey::PointerState::Pressed) {
                clicked = true;
                if (container_to_clear) {
                    container_to_clear->clear_children();
                }
                return true;
            }
            return false;
        }

        bool on_key_event(const ooey::KeyEvent&) override { return false; }
    };

    auto deleting_view = std::make_shared<NestedDeletingView>(page_container);
    page_view->add_child(std::move(deleting_view));

    gooey::Controller controller(input_manager, root_view);

    input_manager.push_pointer_event(ooey::Pointer{1, 10, 10, ooey::PointerState::Pressed});
    controller.process_events();

    // Send Released event
    input_manager.push_pointer_event(ooey::Pointer{1, 10, 10, ooey::PointerState::Released});
    controller.process_events();
}

TEST(OoeyControls, CodeEditorEditingAndHighlighting) {
    ooey::Font font{"monospace", 14};
    gooey::CodeEditor editor{ooey::Rect{0, 0, 400, 300}, font, ooey::Color{255, 255, 255}, ooey::Color{30, 30, 30}};

    // Focus CodeEditor
    editor.on_pointer_event(ooey::Pointer{1, 10, 10, ooey::PointerState::Pressed});

    // Type text: "int x = 42;"
    editor.on_text_event({static_cast<char32_t>('i')});
    editor.on_text_event({static_cast<char32_t>('n')});
    editor.on_text_event({static_cast<char32_t>('t')});
    editor.on_text_event({static_cast<char32_t>(' ')});
    editor.on_text_event({static_cast<char32_t>('x')});
    editor.on_text_event({static_cast<char32_t>(' ')});
    editor.on_text_event({static_cast<char32_t>('=')});
    editor.on_text_event({static_cast<char32_t>(' ')});
    editor.on_text_event({static_cast<char32_t>('4')});
    editor.on_text_event({static_cast<char32_t>('2')});
    editor.on_text_event({static_cast<char32_t>(';')});

    EXPECT_EQ(editor.get_text(), "int x = 42;");

    // Press Backspace
    editor.on_key_event({0xFF08, ooey::KeyState::Pressed}); // XK_BackSpace
    EXPECT_EQ(editor.get_text(), "int x = 42");

    // Move cursor left twice to be before "42"
    editor.on_key_event({0xFF51, ooey::KeyState::Pressed}); // Left
    editor.on_key_event({0xFF51, ooey::KeyState::Pressed}); // Left

    // Press Return to split line
    editor.on_key_event({0xFF0D, ooey::KeyState::Pressed}); // XK_Return
    EXPECT_EQ(editor.get_text(), "int x = \n42");

    // Verify syntax highlighting works
    auto highlighter = std::make_shared<gooey::CppSyntaxHighlighter>();
    editor.set_syntax_highlighter(highlighter);
    
    int end_state = 0;
    auto tokens = highlighter->highlight("int main() {", 0, end_state);
    ASSERT_GE(tokens.size(), 1);
    EXPECT_EQ(tokens[0].text, "int");
    EXPECT_EQ(tokens[0].type, gooey::TokenType::Type);

    auto tokens_ret = highlighter->highlight("return 0;", 0, end_state);
    ASSERT_GE(tokens_ret.size(), 1);
    EXPECT_EQ(tokens_ret[0].text, "return");
    EXPECT_EQ(tokens_ret[0].type, gooey::TokenType::Keyword);
}

TEST(OoeyControls, RichTextBoxSelectionAndCopyPaste) {
    ooey::Font font{"monospace", 14};
    gooey::RichTextBox box{ooey::Rect{0, 0, 400, 300}, font, ooey::Color{255, 255, 255}, ooey::Color{30, 30, 30}};

    box.set_text("Hello World");
    box.on_pointer_event(ooey::Pointer{1, 10, 10, ooey::PointerState::Pressed});

    // Press Shift+Right five times to select "Hello"
    box.on_key_event({0xFFE1, ooey::KeyState::Pressed}); // Shift Pressed
    for (int i = 0; i < 5; ++i) {
        box.on_key_event({0xFF53, ooey::KeyState::Pressed}); // Right arrow
    }
    box.on_key_event({0xFFE1, ooey::KeyState::Released}); // Shift Released

    // Verify selection text
    EXPECT_TRUE(box.has_selection());
    EXPECT_EQ(box.get_selected_text(), "Hello");

    // Copy selected text to string clipboard (emulating high-level notepad logic)
    std::string clipboard = box.get_selected_text();
    EXPECT_EQ(clipboard, "Hello");

    // Clear selection by moving cursor to end
    box.on_key_event({0xFF57, ooey::KeyState::Pressed}); // End
    EXPECT_FALSE(box.has_selection());

    // Insert hyphen and paste "Hello" back at the end
    box.on_text_event({static_cast<char32_t>(' ')});
    box.on_text_event({static_cast<char32_t>('-')});
    box.on_text_event({static_cast<char32_t>(' ')});
    box.insert_text(clipboard);

    EXPECT_EQ(box.get_text(), "Hello World - Hello");
}






