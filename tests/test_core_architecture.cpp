#include <gtest/gtest.h>
#include "ooey/types.hpp"
#include "ooey/application.hpp"
#include "ooey/controls/text_box.hpp"
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

class MockWindowBackend : public ooey::IWindowBackend {
public:
    int poll_count = 0;
    ooey::InputManager* input_manager = nullptr;

    bool create(const ooey::Size& size, const char* title) override {
        return true;
    }

    void destroy() override {
    }

    bool poll_events() override {
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
};

TEST(OoeyApplication, MainLoopExecution) {
    ooey::Application app;
    auto mock_backend = std::make_unique<MockWindowBackend>();
    auto* mock_ptr = mock_backend.get();

    app.set_window_backend(std::move(mock_backend));
    app.run();

    EXPECT_EQ(mock_ptr->poll_count, 3);
}

TEST(OoeyApplication, HeadlessExit) {
    ooey::Application app;
    // Should exit immediately without an infinite loop if no backend
    app.run(); 
    SUCCEED();
}

TEST(OoeyControls, TextBoxKeyboardInput) {
    ooey::Font font{"sans-serif", 16};
    ooey::TextBox textBox{ooey::Rect{0, 0, 100, 100}, font, ooey::Color{0, 0, 0}, ooey::Color{255, 255, 255}};

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
