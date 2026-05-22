#include <gtest/gtest.h>
#include "ooey/types.hpp"
#include "ooey/application.hpp"

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
